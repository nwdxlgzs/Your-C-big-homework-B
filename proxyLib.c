#include "proxyLib.h"
#include "liteiconv.h"
SocketSetting* socketSetting;
static const char* website_root = "webui";
int website_port = WebServer_DefaultPort;//Ĭ��18081�����ռ���Ǿͼ���������
typedef struct mimetype {
    const char* ext;
    const char* type;
} mimetype;
static mimetype _MimeType[] = {
    {"html","text/html" },
    {"htm","text/html"},
    {"css","text/css"},
    {"xml","text/xml"},
    {"gif","image/gif"},
    {"jpeg","image/jpeg"},
    {"jpg","image/jpeg"},
    {"js","text/javascript"},
    {"txt","text/plain"},
    {"png","image/png"},
    {"ico","image/x-icon"},
    {"jng","image/x-jng"},
    {"bmp","image/x-ms-bmp"},
    {"svg","image/svg+xml"},
    {"webp","image/webp"},
    {"woff","application/font-woff"},
    {"jar","application/java-archive"},
    {"json","application/json"},
    {"doc","application/msword"},
    {"pdf","application/pdf"},
    {"m3u8","application/vnd.apple.mpegurl"},
    {"xls","application/vnd.ms-excel"},
    {"ppt","application/vnd.ms-powerpoint"},
    {"7z","application/x-7z-compressed"},
    {"rar","application/x-rar-compressed"},
};
void webServer_launch(int* isOpenBrowser) {
    //Ѱ��һ��û�б�ռ�õĶ˿�
    website_port = WebServer_DefaultPort;
    while (1) {
        if (socket_init() == -1) {
            perror("Error: failed to initialize socket library\n");
            return;
        }
        int server_sockfd = socket_create();
        if (server_sockfd == -1) {
            perror("Error: failed to create server socket\n");
            return;
        }
        if (socket_bind(server_sockfd, WebServer_Host, website_port, 0) == -1) {
            website_port++;
            continue;
        }
        socket_close(server_sockfd);
        socket_cleanup();
        break;
    }
    for (int i = 0;i < 64;i++) {
        if (socketSetting[i].is_localhost) {
            socketSetting[i].port = website_port;
            break;
        }
    }
    if (*isOpenBrowser) {
        //�������������ҳ(�������������Զ�������ҳ)
        char url[100];
        sprintf(url, "http://%s:%d/index.html", WebServer_Host, website_port);
        runOnThread(openInBrowser, url);//�߳��������������ȴ����н��
        *isOpenBrowser = 0;
    }
redo_webServer_launch:
    if (socket_init() == -1) {
        perror("Error: failed to initialize socket library\n");
        return;
    }
    int server_sockfd = socket_create();
    if (server_sockfd == -1) {
        perror("Error: failed to create server socket\n");
        return;
    }
    if (set_socket_timeout(server_sockfd, 60000, 60000) == -1) {
        perror("Error: failed to set socket timeout\n");
        return;
    }
    if (socket_bind(server_sockfd, WebServer_Host, website_port, 1) == -1) {
        char err[100];
        sprintf(err, "Error: failed to bind server socket to  %s:%d\n", WebServer_Host, website_port);
        perror(err);
        return;
    }
    if (socket_listen(server_sockfd) == -1) {
        perror("Error: failed to listen on server socket\n");
        return;
    }
    printf("WebServer: Server is listening on %s:%d\n", WebServer_Host, website_port);
    char full_url_fpath[4096];
    while (1) {
        int client_sockfd = socket_accept(server_sockfd);
        if (client_sockfd == -1) {
            fprintf(stderr, "Error: failed to accept connection on server socket\n");
            return;
        }
        int bufsize = RECV_BUFF_SIZE;
        if (setsockopt(client_sockfd, SOL_SOCKET, SO_RCVBUF, (const char*)&bufsize, sizeof(bufsize)) == -1) {
            perror("setsockopt: failed to set SO_RCVBUF option\n");
            return;
        }
        bufsize = SEND_BUFF_SIZE;
        if (setsockopt(client_sockfd, SOL_SOCKET, SO_SNDBUF, (const char*)&bufsize, sizeof(bufsize)) == -1) {
            perror("setsockopt: failed to set SO_SNDBUF option\n");
            return;
        }
        char* buf = NULL;
        int r = recv_msg(client_sockfd, &buf, 0);
        if (r == 0 || r == -1) {/*what's up*/
            free(buf);
            printf("Info: HTTP connection closed\n");
            socket_close(client_sockfd);
            socket_close(server_sockfd);
            printf("Info: Server closed client socket %d & server socket %d\n", client_sockfd, server_sockfd);
            socket_cleanup();
            printf("Info: restart...\n");
            goto redo_webServer_launch;
        }
        SocketData* data = parseSocketData(buf, r);
        BufferBox buffer_box = {
            .buffer = NULL,
            .size = -1
        };
        if (strncmp(data->url, "/api.cgi?", 9) == 0 || strcmp(data->url, "/getPort.json") == 0) {//C��˽ӿ�
            HDStatus status = globalHandle(WebServer_Host, website_port, &buffer_box, client_sockfd, data, 1);
            if (status == HDStatus_continue) continue;
            else if (status == HDStatus_send) goto send_buffer;
            else if (status == HDStatus_sendAsTxt || status == HDStatus_sendAsJson) {
                char* tbuf = malloc(1024 + buffer_box.size);
                if (status == HDStatus_sendAsTxt) {
                    sprintf(tbuf, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/plain\r\nAccess-Control-Allow-Origin: *\r\n\r\n",
                                           buffer_box.size);
                } else if (status == HDStatus_sendAsJson) {
                    sprintf(tbuf, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\n\r\n",
                        buffer_box.size);
                } else {
                    sprintf(tbuf, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: application/octet-stream\r\nAccess-Control-Allow-Origin: *\r\n\r\n",
                        buffer_box.size);
                }
                int hlen = strlen(tbuf);
                memcpy(tbuf + hlen, buffer_box.buffer, buffer_box.size);
                free(buffer_box.buffer);
                buffer_box.buffer = tbuf;
                buffer_box.size += hlen;
                goto send_buffer;
            }
        } else {
            strcpy(full_url_fpath, website_root);
            int len = strlen(data->url);
            char* u = urlDecode(data->url, &len);
            strncat(full_url_fpath, u, len);
            free(u);
            if (data->url[strlen(data->url) - 1] == '/') {
                strcat(full_url_fpath, "index.html");
            }
            //Ȼ��ɾ��url�еĲ�����#����?����ģ�
            {
                char* p = strchr(data->url, '?');
                if (p != NULL) {
                    p = strchr(full_url_fpath, '?');
                    if (p != NULL) *p = '\0';
                }
                p = strchr(data->url, '#');
                if (p != NULL) {
                    p = strchr(full_url_fpath, '#');
                    if (p != NULL) *p = '\0';
                }
            }
            printf("WebServer: Website File Index %s\n", full_url_fpath);
            HDStatus status = getWebServerRes(full_url_fpath, &buffer_box, client_sockfd, data);
            if (status == HDStatus_continue) continue;
        }
        if (buffer_box.buffer != NULL) {
            if (buffer_box.size == -1) buffer_box.size = strlen(buffer_box.buffer);
            char* ext = strrchr(full_url_fpath, '.');
            int find = 0;
            if (ext != NULL) {
                ext++;
                for (int i = 0; i < sizeof(_MimeType) / sizeof(mimetype); i++) {
                    if (strcasecmp(ext, _MimeType[i].ext) == 0) {
                        char* tbuf = malloc(buffer_box.size + 256);
                        sprintf(tbuf, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: %s\r\nAccess-Control-Allow-Origin: *\r\n\r\n",
                         buffer_box.size, _MimeType[i].type);
                        int hlen = strlen(tbuf);
                        memcpy(tbuf + hlen, buffer_box.buffer, buffer_box.size);
                        free(buffer_box.buffer);
                        buffer_box.buffer = tbuf;
                        buffer_box.size += hlen;
                        find = 1;
                        break;
                    }
                }
            }
            if (!find) {
                char* tbuf = malloc(buffer_box.size + 256);
                sprintf(tbuf, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: %s\r\nAccess-Control-Allow-Origin: *\r\n\r\n",
                 buffer_box.size, "*/*");
                int hlen = strlen(tbuf);
                memcpy(tbuf + hlen, buffer_box.buffer, buffer_box.size);
                free(buffer_box.buffer);
                buffer_box.buffer = tbuf;
                buffer_box.size += hlen;
            }
        } else {
            char* tbuf = malloc(256);
            strcpy(tbuf, "HTTP/1.1 200 OK\r\nContent-Length: 0\r\nAccess-Control-Allow-Origin: *\r\n\r\n");
            buffer_box.buffer = tbuf;
            buffer_box.size = strlen(tbuf);
        }
    send_buffer:
        send_msg(client_sockfd, buffer_box.buffer, buffer_box.size, 0);
        freeSocketData(data);
        if (buffer_box.buffer)free(buffer_box.buffer);
        socket_close(client_sockfd);
        if (closeAllSocket)break;
    }
    socket_close(server_sockfd);
    printf("Info: Server closed server socket %d\n", server_sockfd);
    socket_cleanup();
    printf("Info: Server cleaned up socket library\n");
}
//���ع����Ĵ��������ļ����ݣ�
void workproxy_launch(SocketSetting* ss) {
    if (ss == NULL)return;
redo_workproxy_launch:
    if (socket_init() == -1) {
        perror("Error: failed to initialize socket library\n");
        return;
    }
    int server_sockfd = socket_create();
    if (server_sockfd == -1) {
        perror("Error: failed to create server socket\n");
        return;
    }
    if (set_socket_timeout(server_sockfd, workproxy_Timeout, workproxy_Timeout) == -1) {
        perror("Error: failed to set socket timeout\n");
        return;
    }
    if (socket_bind(server_sockfd, ss->host, ss->port, 1) == -1) {
        char err[100];
        sprintf(err, "Error: failed to bind server socket to  %s:%d\n", ss->host, ss->port);
        perror(err);
        return;
    }
    if (socket_listen(server_sockfd) == -1) {
        perror("Error: failed to listen on server socket\n");
        return;
    }
    printf("WorkProxxy: worker is listening on %s:%d\n", ss->host, ss->port);
    char full_url_fpath[4096];
    while (1) {
        int client_sockfd = socket_accept(server_sockfd);
        if (client_sockfd == -1) {
            fprintf(stderr, "Error: failed to accept connection on server socket\n");
            return;
        }
        int bufsize = RECV_BUFF_SIZE;
        if (setsockopt(client_sockfd, SOL_SOCKET, SO_RCVBUF, (const char*)&bufsize, sizeof(bufsize)) == -1) {
            perror("setsockopt: failed to set SO_RCVBUF option\n");
            return;
        }
        bufsize = SEND_BUFF_SIZE;
        if (setsockopt(client_sockfd, SOL_SOCKET, SO_SNDBUF, (const char*)&bufsize, sizeof(bufsize)) == -1) {
            perror("setsockopt: failed to set SO_SNDBUF option\n");
            return;
        }
        char* buf = NULL;
        int r = recv_msg(client_sockfd, &buf, 0);
        if (r == 0 || r == -1) {/*what's up*/
            free(buf);
            printf("Info: HTTP connection closed\n");
            socket_close(client_sockfd);
            socket_close(server_sockfd);
            printf("Info: Server closed client socket %d & server socket %d\n", client_sockfd, server_sockfd);
            socket_cleanup();
            printf("Info: restart...\n");
            goto redo_workproxy_launch;
        }
        SocketData* data = parseSocketData(buf, r);
        BufferBox buffer_box = {
            .buffer = NULL,
            .size = -1
        };
        if (strncmp(data->url, "/api.cgi?", 9) == 0 || strcmp(data->url, "/getPort.json") == 0) {//C��˽ӿ�
            HDStatus status = globalHandle(WebServer_Host, website_port, &buffer_box, client_sockfd, data, 0);
            if (status == HDStatus_continue) continue;
            else if (status == HDStatus_send) goto send_buffer;
            else if (status == HDStatus_sendAsTxt || status == HDStatus_sendAsJson) {
                char* tbuf = malloc(1024 + buffer_box.size);
                if (status == HDStatus_sendAsTxt) {
                    sprintf(tbuf, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/plain\r\nAccess-Control-Allow-Origin: *\r\n\r\n",
                                           buffer_box.size);
                } else if (status == HDStatus_sendAsJson) {
                    sprintf(tbuf, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\n\r\n",
                        buffer_box.size);
                } else {
                    sprintf(tbuf, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: application/octet-stream\r\nAccess-Control-Allow-Origin: *\r\n\r\n",
                        buffer_box.size);
                }
                int hlen = strlen(tbuf);
                memcpy(tbuf + hlen, buffer_box.buffer, buffer_box.size);
                free(buffer_box.buffer);
                buffer_box.buffer = tbuf;
                buffer_box.size += hlen;
                goto send_buffer;
            }
        } else {
            strcpy(full_url_fpath, website_root);
            int len = strlen(data->url);
            char* u = urlDecode(data->url, &len);
            strncat(full_url_fpath, u, len);
            free(u);
            if (data->url[strlen(data->url) - 1] == '/') {
                strcat(full_url_fpath, "index.html");
            }
            //Ȼ��ɾ��url�еĲ�����#����?����ģ�
            {
                char* p = strchr(data->url, '?');
                if (p != NULL) {
                    p = strchr(full_url_fpath, '?');
                    if (p != NULL) *p = '\0';
                }
                p = strchr(data->url, '#');
                if (p != NULL) {
                    p = strchr(full_url_fpath, '#');
                    if (p != NULL) *p = '\0';
                }
            }
            HDStatus status = getWebServerRes(full_url_fpath, &buffer_box, client_sockfd, data);
            if (status == HDStatus_continue) continue;
        }
        if (buffer_box.buffer != NULL) {
            if (buffer_box.size == -1) buffer_box.size = strlen(buffer_box.buffer);
            char* ext = strrchr(full_url_fpath, '.');
            int find = 0;
            if (ext != NULL) {
                ext++;
                for (int i = 0; i < sizeof(_MimeType) / sizeof(mimetype); i++) {
                    if (strcasecmp(ext, _MimeType[i].ext) == 0) {
                        char* tbuf = malloc(buffer_box.size + 256);
                        sprintf(tbuf, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: %s\r\nAccess-Control-Allow-Origin: *\r\n\r\n",
                         buffer_box.size, _MimeType[i].type);
                        int hlen = strlen(tbuf);
                        memcpy(tbuf + hlen, buffer_box.buffer, buffer_box.size);
                        free(buffer_box.buffer);
                        buffer_box.buffer = tbuf;
                        buffer_box.size += hlen;
                        find = 1;
                        break;
                    }
                }
            }
            if (!find) {
                char* tbuf = malloc(buffer_box.size + 256);
                sprintf(tbuf, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: %s\r\nAccess-Control-Allow-Origin: *\r\n\r\n",
                 buffer_box.size, "*/*");
                int hlen = strlen(tbuf);
                memcpy(tbuf + hlen, buffer_box.buffer, buffer_box.size);
                free(buffer_box.buffer);
                buffer_box.buffer = tbuf;
                buffer_box.size += hlen;
            }
        } else {
            char* tbuf = malloc(256);
            strcpy(tbuf, "HTTP/1.1 200 OK\r\nContent-Length: 0\r\nAccess-Control-Allow-Origin: *\r\n\r\n");
            buffer_box.buffer = tbuf;
            buffer_box.size = strlen(tbuf);
        }
    send_buffer:
        send_msg(client_sockfd, buffer_box.buffer, buffer_box.size, 0);
        freeSocketData(data);
        if (buffer_box.buffer)free(buffer_box.buffer);
        socket_close(client_sockfd);
        if (closeAllSocket)break;
    }
    socket_close(server_sockfd);
    printf("Info: Server closed server socket %d\n", server_sockfd);
    socket_cleanup();
    printf("Info: Server cleaned up socket library\n");
}

/*
    ȫ�ִ����������ڴ������е�����
    is_localhost�����⣬���ǹ���Աҳ�������ע�����û��˺ŵ�¼���ǹ���Ա����Ϊ���˹���Աû���ܽӴ��豸
*/
HDStatus globalHandle(const char* host, int port, BufferBox* box, int client_sockfd, SocketData* data, int is_localhost) {
    if (box == NULL || data == NULL)return 0;//host���Բ�֪��������box��dataһ��Ҫ��
    if (strcmp(data->url, "/getPort.json") == 0) {//������Ϣ�õ�
        char* getPort = malloc(4096);
        sprintf(getPort, "[");
        char tmp[256];
        for (int i = 0;i < 64;i++) {
            if (socketSetting[i].port == 0)break;
            sprintf(tmp, "{\"%s\":%d},", socketSetting[i].host, socketSetting[i].port);
            strcat(getPort, tmp);
        }
        getPort[strlen(getPort) - 1] = ']';
        box->buffer = getPort;
        box->size = strlen(getPort);
        return HDStatus_sendAsJson;
    }
    SocketPair* api = socketPairGET(data, "api", 1);
    if (api == NULL || api->value == NULL) {//û�ҵ�api�����ǾͲ��ṩ����
        do503ServiceUnavailable(data, client_sockfd);
        return HDStatus_continue;
    }
    if (strcmp(api->value, "AreYouAlive") == 0) {//һ��Լ���õ�API����ȷ������
        /*
            ˵����
                ����/api.cgi?api=AreYouAlive
                ���أ�ʱ���ַ�����%Y-%m-%d %H:%M:%S��
                ���壺���ڿͻ��˲��Է������Ƿ���
        */
        box->buffer = fmtYmdHMS(NULL, getTimestamp());
        box->size = strlen(box->buffer);
        return HDStatus_sendAsTxt;
    } else if (strcmp(api->value, "login") == 0) {//��¼
        /*
            ˵����
                ����/api.cgi?api=login&acc=XXX&pwd=XXX
                ���أ���¼�ɹ���{"status":1,","token":"token�ַ���","ownerID":�û�ID}
                      ��¼ʧ�ܣ�{"status":0}
                ���壺���ڿͻ��˵�¼����ȡtoken
        */
        SocketPair* SPacc = socketPairGET(data, "acc", 1);
        SocketPair* APpwd = socketPairGET(data, "pwd", 1);
        int isFail = 0;
        if (SPacc == NULL || SPacc->value == NULL ||
            APpwd == NULL || APpwd->value == NULL) isFail = 1;
        else {
            int len = strlen(SPacc->value);
            char* acc = urlDecode(SPacc->value, &len);
            len = strlen(APpwd->value);
            char* pwd = urlDecode(APpwd->value, &len);
            User* user = User_getUser(acc);
            if (user == NULL || !User_login(user, pwd))isFail = 1;
            free(acc);
            free(pwd);
            if (!isFail) {
                sBuffer sb;
                sBuffer_init(&sb, 1024);
                sBuffer_fmt(&sb, "{\"status\":1,\"token\":\"%s\",\"ownerID\":%d,\"account\":\"", user->token, user->ownerID);
                char* text = user->account;
                int len = strlen(text);
                int isGBKsys = isGBKSystem();
                if (text && isGBKsys && !isUTF8(text, len)) {
                    text = gbk_to_utf8(text, &len);
                }
                sBuffer_utf8jsonstr(&sb, text, len);
                if (text && isGBKsys && text != user->account)
                    free(text);
                sBuffer_appendl(&sb, "\"}", 2);
                box->buffer = (char*)malloc(sb.size + 1);
                memcpy(box->buffer, sb.buff, sb.size);
                box->buffer[sb.size] = '\0';
                box->size = sb.size;
                sBuffer_free(&sb);
                return HDStatus_sendAsJson;
            }
        }
        box->buffer = malloc(16);
        strcpy(box->buffer, "{\"status\":0}");
        box->size = strlen(box->buffer);
        return HDStatus_sendAsJson;
    } else if (strcmp(api->value, "logout") == 0) {//��¼
        /*
            ˵����
                ����/api.cgi?api=logout&token=XXX
                ���أ���¼�ɹ���{"status":1}
                      ��¼ʧ�ܣ�{"status":0}
                ���壺���ڿͻ��˵ǳ�������token
        */
        SocketPair* SPtoken = socketPairGET(data, "token", 1);
        int isFail = 0;
        if (SPtoken == NULL || SPtoken->value == NULL) isFail = 1;
        else {
            int len = strlen(SPtoken->value);
            char* token = SPtoken->value;
            User* user = User_getUserByToken(token);
            if (user == NULL)isFail = 1;
            else {
                User_logout(user);
            }
            if (!isFail) {
                box->buffer = malloc(16);
                strcpy(box->buffer, "{\"status\":1}");
                box->size = strlen(box->buffer);
                return HDStatus_sendAsJson;
            }
        }
        box->buffer = malloc(16);
        strcpy(box->buffer, "{\"status\":0}");
        box->size = strlen(box->buffer);
        return HDStatus_sendAsJson;
    } else if (strcmp(api->value, "register") == 0) {//ע��
        /*
        ˵����
            ����/api.cgi?api=register&acc=XXX&pwd=XXX
            ���أ���¼�ɹ���{"status":1,","token":"token�ַ���"}
                  ��¼ʧ�ܣ�{"status":0}
            ���壺���ڿͻ��˵�¼����ȡtoken
        */
        SocketPair* SPacc = socketPairGET(data, "acc", 1);
        SocketPair* APpwd = socketPairGET(data, "pwd", 1);
        int isFail = 0;
        if (SPacc == NULL || SPacc->value == NULL ||
            APpwd == NULL || APpwd->value == NULL) isFail = 1;
        else {
            int len = strlen(SPacc->value);
            char* acc = urlDecode(SPacc->value, &len);
            if (strcmp(acc, "δ֪") == 0)isFail = 1;//������ע��admin�˺�
            if (strcmp(acc, "*") == 0)isFail = 1;//������ע��admin�˺�
            len = strlen(APpwd->value);
            char* pwd = urlDecode(APpwd->value, &len);
            if (!isFail && User_create(acc, pwd) == NULL)isFail = 1;
            free(acc);
            free(pwd);
            if (!isFail) {
                box->buffer = malloc(512);
                strcpy(box->buffer, "{\"status\":1}");
                box->size = strlen(box->buffer);
                return HDStatus_sendAsJson;
            }
        }
        box->buffer = malloc(16);
        strcpy(box->buffer, "{\"status\":0}");
        box->size = strlen(box->buffer);
        return HDStatus_sendAsJson;
    } else if (strcmp(api->value, "getStudentID") == 0) {//��ȡѧ��
        /*
            ˵����
                ����/api.cgi?api=getStudentID
                ���أ�XXXX
                ���壺������ҳ��ʾѧ����Ϣ
         */
        box->buffer = malloc(64);
        strcpy(box->buffer, "1234567890");
        box->size = strlen(box->buffer);
        return HDStatus_sendAsTxt;
    } else if (strcmp(api->value, "canTrust") == 0) {//�ж��Ƿ��ǿ��ŵ�
        /*
            ˵����
                ����/api.cgi?api=canTrust
                ���أ�true/false
                ���壺������ҳ�ж��Ƿ��ǿ�������
         */
        box->buffer = malloc(64);
        if (is_localhost) {
            strcpy(box->buffer, "true");
        } else {
            strcpy(box->buffer, "false");
        }
        box->size = strlen(box->buffer);
        return HDStatus_sendAsTxt;
    } else if (strcmp(api->value, "infosafeusers") == 0) {//��ȡUserList����(infosafeJsonֻ���˺ź�ID����Ϣ)
        /*
            ˵����
                ����/api.cgi?api=infosafeusers&token=XXX
                ���أ�UserList��infosafeJson������
                ���壺������ʾ����
         */
        SocketPair* SPtoken = socketPairGET(data, "token", 1);
        int isFail = 0;
        if (SPtoken == NULL || SPtoken->value == NULL)isFail = 1;
        else {
            User* user = User_getUserByToken(SPtoken->value);
            if (user == NULL) {//ûע������ã������ڵ�
                do400BadRequest(data, client_sockfd);
                return HDStatus_continue;
            }
            box->buffer = UserList_infosafeJson();
            box->size = strlen(box->buffer);
            return HDStatus_sendAsJson;
        }
    } else if (strcmp(api->value, "data") == 0) {//��ȡ����
        /*
            ˵����
                ����/api.cgi?api=data&token=XXX&time_filter=XXX&type_filter=XXX&owner_filter=XXX&sort=XXX&search=XXX
                ���أ�XMsgObjectsResult��json������
                ���壺������ʾ����
                ����˵����
                    sort������ asc �� desc ��Ĭ��asc����
                    time_filter������ HEX(start)-HEX(end) �� *-HEX(end) �� HEX(start)-* �� *-*��Ĭ��*-*��
                    type_filter������ XXX �� *��Ĭ��*�����⣬����XXX_XXX�����Ķ�ѡ����(text/markdown/file/image/audio/video)
                    owner_filter������ Url(XXX) �� *��Ĭ��*��
                    search������ msgName�����ؼ���(����)������Ҳ������ƥ��msgId��ͬʱ�ռ��жϣ��������Ǿ��ǲ�����ȫ��չʾ��
                    �������ʱ��
         */
        SocketPair* SPtoken = socketPairGET(data, "token", 1);
        SocketPair* SPtime_filter = socketPairGET(data, "time_filter", 1);
        SocketPair* SPtype_filter = socketPairGET(data, "type_filter", 1);
        SocketPair* SPowner_filter = socketPairGET(data, "owner_filter", 1);
        SocketPair* SPsort = socketPairGET(data, "sort", 1);
        int isFail = 0;
        if ((SPtoken == NULL || SPtoken->value == NULL) ||
            (SPtime_filter != NULL && SPtime_filter->value == NULL && strstr(SPtime_filter->value, "-") != NULL) ||
            (SPtype_filter != NULL && SPtype_filter->value == NULL) ||
            (SPowner_filter != NULL && SPowner_filter->value == NULL) ||
            (SPsort != NULL && SPsort->value == NULL))  isFail = 1;
        else {
            User* user = User_getUserByToken(SPtoken->value);
            if (user == NULL) {//ûע������ã������ڵ�
                do400BadRequest(data, client_sockfd);
                return HDStatus_continue;
            }
            /*
                ʱ�����������
            */
            int isASC = 1;
            if (SPsort != NULL && strcmp(SPsort->value, "desc") == 0)isASC = -1;
            /*
                ʱ����˽���
            */
            char* time_start = NULL;
            char* time_end = NULL;
            char* time_filter = SPtime_filter->value;
            int time_filter_len = strlen(time_filter);
            if (strcmp(time_filter, "*-*") == 0) {
                time_start = NULL;
                time_end = NULL;
            } else if (time_filter[time_filter_len - 1] == '*') {//HEX(start)-*
                time_end = NULL;
                int len = strlen(time_filter);
                char buff[len + 1];
                strcpy(buff, time_filter);
                buff[len] = '\0';
                buff[len - 1] = '\0';
                time_start = decHex(buff, NULL);
            } else if (time_filter[0] == '*') {//*-HEX(end)
                time_start = NULL;
                time_end = decHex(time_filter + 2, NULL);
            } else {
                char* p = strstr(time_filter, "-");
                *p = '\0';
                time_start = decHex(time_filter, NULL);
                time_end = decHex(p + 1, NULL);
            }
            TimeInfo tibakup[2] = { 0 };
            TimeInfo* time_start_info = &tibakup[0];
            TimeInfo* time_end_info = &tibakup[1];
            if (time_start != NULL) {
                tibakup[0] = getTimeInfo(getTimestampByStr(time_start));
                free(time_start);
            } else {
                time_start_info = NULL;//��ʾ������
            }
            if (time_end != NULL) {
                tibakup[1] = getTimeInfo(getTimestampByStr(time_end));
                free(time_end);
            } else {
                time_end_info = NULL;//��ʾ������
            }
            /*
                ���͹��˽���
            */
            XMsgObjectType type_filter[XMsgObjectType_MAXENUMSIZE] = { 0 };
            int type_filter_size = 0;
            if (strstr(SPtype_filter->value, "text") != NULL) type_filter[type_filter_size++] = XMsgObjectType_TEXT;
            if (strstr(SPtype_filter->value, "markdown") != NULL) type_filter[type_filter_size++] = XMsgObjectType_MARKDOWN;
            if (strstr(SPtype_filter->value, "file") != NULL) type_filter[type_filter_size++] = XMsgObjectType_FILE;
            if (strstr(SPtype_filter->value, "image") != NULL) type_filter[type_filter_size++] = XMsgObjectType_IMAGE;
            if (strstr(SPtype_filter->value, "audio") != NULL) type_filter[type_filter_size++] = XMsgObjectType_AUDIO;
            if (strstr(SPtype_filter->value, "video") != NULL) type_filter[type_filter_size++] = XMsgObjectType_VIDEO;
            /*
                �����߹��˽���
            */
            char* owner_filter = SPowner_filter->value;
            if (strcmp(owner_filter, "*") == 0)owner_filter = NULL;
            else {
                int len = strlen(owner_filter);
                owner_filter = urlDecode(owner_filter, &len);
            }
            /*
                ��������
                search��ǿ���������￪ʼ�����ж�
            */
            SocketPair* SPsearch = socketPairGET(data, "search", 1);
            char* search = NULL;
            if (SPsearch != NULL && SPsearch->value != NULL) {
                int len = strlen(SPsearch->value);
                search = urlDecode(SPsearch->value, &len);
            }
            XMsgObjectsResult r_time = XMsgObject_selectMsgByTime(time_start_info, time_end_info);
            XMsgObjectsResult r_type = XMsgObject_selectMsgByTypes(type_filter, type_filter_size);
            XMsgObjectsResult r_owner = XMsgObject_selectMsgByOwner(owner_filter);
            XMsgObjectsResult r_search = XMsgObject_selectMsgBySearch(search);
            //��ʼ�󽻼�
            XMsgObjectsResult r = XMsgObjectsResult_operator_intersection(r_time, r_type, 1);
            r = XMsgObjectsResult_operator_intersection(r, r_owner, 1);
            r = XMsgObjectsResult_operator_intersection(r, r_search, 1);
            /*
                ����ʱ�䣩
            */
            if (r.size > 1) {//����1����Ҫ����
                XMsgObject** objs = r.list;
                for (int i = 0; i < r.size - 1; i++) {
                    for (int j = 0; j < r.size - 1 - i; j++) {
                        if (isASC * TimeInfo_compare(&objs[j]->time_info, &objs[j + 1]->time_info) > 0) {
                            XMsgObject* temp = objs[j];
                            objs[j] = objs[j + 1];
                            objs[j + 1] = temp;
                        }
                    }
                }
            }
            char* json = XMsgObjectsResult_toJson(r);
            XMsgObjectsResult_free(r);
            box->buffer = json;
            box->size = strlen(json);
            if (owner_filter != NULL && owner_filter != SPowner_filter->value)free(owner_filter);
            if (search != NULL && search != SPsearch->value)free(search);
            return HDStatus_sendAsJson;
        }
        box->buffer = malloc(16);
        strcpy(box->buffer, "[]");
        box->size = strlen(box->buffer);
        return HDStatus_sendAsJson;
    } else if (strcmp(api->value, "send") == 0) {//�ϴ���������
        /*
            ˵����
                ����/api.cgi?api=send&token=XXX&type=XXX&data=Base64_urlsafe(data)&msgName=url_encode(XXX) {&fileName=url_encode(XXX)}
                ���أ�{"status":0} �� {"status":1,"msgId":XXX}
                ���壺�����ϴ�����
                ����˵����
                    type������ text/markdown/file/image/audio/video
         */
        SocketPair* SPtoken = socketPairGET(data, "token", 1);
        SocketPair* SPtype = socketPairGET(data, "type", 1);
        SocketPair* SPdata = socketPairGET(data, "data", 1);
        SocketPair* SPmsgName = socketPairGET(data, "msgName", 1);
        int isFail = 0;
        if ((SPtoken == NULL || SPtoken->value == NULL) ||
            (SPtype == NULL || SPtype->value == NULL) ||
            (SPdata == NULL || SPdata->value == NULL) ||
            (SPmsgName == NULL || SPmsgName->value == NULL))  isFail = 1;
        else {
            User* user = User_getUserByToken(SPtoken->value);
            if (user == NULL) {//ûע������ã������ڵ�
                do400BadRequest(data, client_sockfd);
                return HDStatus_continue;
            }
            XMsgObject* msg = NULL;
            if ((strcmp(SPtype->value, "text") == 0) ||
            (strcmp(SPtype->value, "markdown") == 0)) {
                int len = strlen(SPdata->value);
                char* data = base64_decode(SPdata->value, &len);
                if (data == NULL)isFail = 1;
                else {
                    int msgNamelen = strlen(SPmsgName->value);
                    char* msgName = urlDecode(SPmsgName->value, &msgNamelen);
                    if (strcmp(SPtype->value, "text") == 0)
                        msg = XMsgObject_newTextMsg(user->ownerID, NULL, msgName, getTimeInfo(getTimestamp()), data, (size_t)len);
                    else if (strcmp(SPtype->value, "markdown") == 0)
                        msg = XMsgObject_newMarkdownMsg(user->ownerID, NULL, msgName, getTimeInfo(getTimestamp()), data, (size_t)len);
                    free(msgName);
                    free(data);
                }
            } else if ((strcmp(SPtype->value, "file") == 0) ||
            (strcmp(SPtype->value, "image") == 0) ||
            (strcmp(SPtype->value, "audio") == 0) ||
            (strcmp(SPtype->value, "video") == 0)) {
                SocketPair* SPfileName = socketPairGET(data, "fileName", 1);
                if (SPfileName == NULL || SPfileName->value == NULL)isFail = 1;
                else {
                    int fileNameLen = strlen(SPfileName->value);
                    char* fileName = urlDecode(SPfileName->value, &fileNameLen);
                    int len = strlen(SPdata->value);
                    char* data = base64_decode(SPdata->value, &len);
                    if (data == NULL)isFail = 1;
                    else {
                        int msgNamelen = strlen(SPmsgName->value);
                        char* msgName = urlDecode(SPmsgName->value, &msgNamelen);
                        if (strcmp(SPtype->value, "file") == 0)
                            msg = XMsgObject_newFileMsg(user->ownerID, NULL, msgName, getTimeInfo(getTimestamp()), fileName, NULL, (size_t)len, 1);
                        else if (strcmp(SPtype->value, "image") == 0)
                            msg = XMsgObject_newImageMsg(user->ownerID, NULL, msgName, getTimeInfo(getTimestamp()), fileName, NULL, (size_t)len, 1);
                        else if (strcmp(SPtype->value, "audio") == 0)
                            msg = XMsgObject_newAudioMsg(user->ownerID, NULL, msgName, getTimeInfo(getTimestamp()), fileName, NULL, (size_t)len, 1);
                        else if (strcmp(SPtype->value, "video") == 0)
                            msg = XMsgObject_newVideoMsg(user->ownerID, NULL, msgName, getTimeInfo(getTimestamp()), fileName, NULL, (size_t)len, 1);
                        storeFile(msg, data);
                        free(msgName);
                        free(data);
                    }
                    free(fileName);
                }
            } else {
                isFail = 1;
            }
            if (isFail == 0 && msg) {
                XMsgObjects_dump(DEFAULT_MSG_STORAGE_PATH);
                box->buffer = malloc(128);
                sprintf(box->buffer, "{\"status\":1,\"msgId\":\"%s\"}", msg->msgId);
            } else {
                box->buffer = malloc(32);
                strcpy(box->buffer, "{\"status\":0}");
            }
            box->size = strlen(box->buffer);
            return HDStatus_sendAsJson;
        }
    } else if (strcmp(api->value, "getMsgById") == 0) {//��ȡ��Ϣ����
        /*
            ˵����
                ����/api.cgi?api=getMsgById&msgId=XXX
                ���أ�XXXX
                ���壺���ڻ�ȡ����Ϣ��Ϣ
         */
        SocketPair* SPmsgId = socketPairGET(data, "msgId", 1);
        if (SPmsgId == NULL || SPmsgId->value == NULL) {
            do400BadRequest(data, client_sockfd);
            return HDStatus_continue;
        }
        XMsgObject* msg = XMsgObject_findMsgById(SPmsgId->value);
        if (msg == NULL) {
            do404NotFound(data, client_sockfd);
            return HDStatus_continue;
        }
        XMsgObjectsResult r = { NULL,0 };
        XMsgObjectsResult_add(&r, msg);
        char* json = XMsgObjectsResult_toJson(r);
        XMsgObjectsResult_free(r);
        box->buffer = json;
        box->size = strlen(json);
        return HDStatus_sendAsJson;
    } else if (strcmp(api->value, "shareDownloadName") == 0) {//��ȡ�Ƽ�����������
        /*
            ˵����
                ����/api.cgi?api=shareDownloadName&msgId=XXX
                ���أ�{"status":1,"msgName":XXX}
                ���壺���ڻ�ȡ�Ƽ�����������
         */
        SocketPair* SPmsgId = socketPairGET(data, "msgId", 1);
        if (SPmsgId == NULL || SPmsgId->value == NULL) {
            do400BadRequest(data, client_sockfd);
            return HDStatus_continue;
        }
        XMsgObject* msg = XMsgObject_findMsgById(SPmsgId->value);
        if (msg == NULL) {
            do404NotFound(data, client_sockfd);
            return HDStatus_continue;
        }
        if (msg->type == XMsgObjectType_TEXT || msg->type == XMsgObjectType_MARKDOWN) {
            int len = strlen(msg->msgName);
            char* text = msg->msgName;
            int isGBKsys = isGBKSystem();
            if (text && isGBKsys && !isUTF8(text, len)) {
                text = gbk_to_utf8(text, &len);
            }
            sBuffer sb;
            sBuffer_init(&sb, 256);
            sBuffer_utf8jsonstr(&sb, text, len);
            if (text && isGBKsys && text != msg->msgName)
                free(text);
            box->buffer = malloc(sb.size + 32);
            sprintf(box->buffer, "{\"status\":1,\"name\":\"%s.%s\"}", sb.buff, msg->type == XMsgObjectType_TEXT ? "txt" : "md");
            sBuffer_free(&sb);
            box->size = strlen(box->buffer);
            return HDStatus_sendAsJson;
        } else if (msg->type == XMsgObjectType_FILE || msg->type == XMsgObjectType_IMAGE || msg->type == XMsgObjectType_AUDIO || msg->type == XMsgObjectType_VIDEO) {
            int len = strlen(msg->fileMsg.fileName);
            char* text = msg->fileMsg.fileName;
            int isGBKsys = isGBKSystem();
            if (text && isGBKsys && !isUTF8(text, len)) {
                text = gbk_to_utf8(text, &len);
            }
            sBuffer sb;
            sBuffer_init(&sb, 1024);
            sBuffer_utf8jsonstr(&sb, text, len);
            if (text && isGBKsys && text != msg->fileMsg.fileName)
                free(text);
            box->buffer = malloc(sb.size + 32);
            sprintf(box->buffer, "{\"status\":1,\"name\":\"%s\"}", sb.buff);
            sBuffer_free(&sb);
            box->size = strlen(box->buffer);
            return HDStatus_sendAsJson;
        } else if (msg->type == XMsgObjectType_BAD) {
            do503ServiceUnavailable(data, client_sockfd);
            return HDStatus_continue;
        } else {
            do403Forbidden(data, client_sockfd);
            return HDStatus_continue;
        }
    } else if (strcmp(api->value, "download") == 0) {//�ϴ���������
        /*
            ˵����
                ����/api.cgi?api=download&msgId=XXX
                ���أ�XXX��404/400
                ���壺������������
                ����˵����
                    ����������token����
         */
        SocketPair* SPmsgId = socketPairGET(data, "msgId", 1);
        if (SPmsgId == NULL || SPmsgId->value == NULL) {
            do400BadRequest(data, client_sockfd);
            return HDStatus_continue;
        }
        XMsgObject* msg = XMsgObject_findMsgById(SPmsgId->value);
        if (msg == NULL) {
            do404NotFound(data, client_sockfd);
            return HDStatus_continue;
        }
        if (msg->type == XMsgObjectType_TEXT || msg->type == XMsgObjectType_MARKDOWN) {
            box->buffer = malloc(msg->textMsg.len + 1);
            memcpy(box->buffer, msg->textMsg.text, msg->textMsg.len);
            box->buffer[msg->textMsg.len] = '\0';
            box->size = msg->textMsg.len;
            return HDStatus_sendAsTxt;
        } else if (msg->type == XMsgObjectType_FILE || msg->type == XMsgObjectType_IMAGE || msg->type == XMsgObjectType_AUDIO || msg->type == XMsgObjectType_VIDEO) {
            box->buffer = getStoreFile(msg, &box->size);
            return HDStatus_sendAsStream;
        } else if (msg->type == XMsgObjectType_BAD) {
            box->buffer = malloc(1);
            box->buffer[0] = '\0';
            box->size = 0;
            return HDStatus_sendAsTxt;
        } else {
            do404NotFound(data, client_sockfd);
            return HDStatus_continue;
        }
    } else if (strcmp(api->value, "withdraw") == 0) {//�غɳ���
        /*
            ˵����
                ����/api.cgi?api=withdraw&msgId=XXX&token=XXX
                ���أ�{"status":1}/{"status":0}
                ���壺���ڳ����غ�
         */
        SocketPair* SPmsgId = socketPairGET(data, "msgId", 1);
        SocketPair* SPtoken = socketPairGET(data, "token", 1);
        if (SPmsgId == NULL || SPmsgId->value == NULL || SPtoken == NULL || SPtoken->value == NULL) {
            do400BadRequest(data, client_sockfd);
            return HDStatus_continue;
        }
        XMsgObject* msg = XMsgObject_findMsgById(SPmsgId->value);
        if (msg == NULL) {
            do404NotFound(data, client_sockfd);
            return HDStatus_continue;
        }
        User* user = User_getUserByToken(SPtoken->value);
        if (user == NULL) {
            do403Forbidden(data, client_sockfd);
            return HDStatus_continue;
        }
        if (user->ownerID != msg->ownerID && !is_localhost) {
            do503ServiceUnavailable(data, client_sockfd);
            return HDStatus_continue;
        }
        int ret = XMsgObject_withdraw(msg);
        box->buffer = malloc(32);
        if (ret == 0) {
            sprintf(box->buffer, "{\"status\":1}");
        } else {
            sprintf(box->buffer, "{\"status\":0}");
        }
        box->size = strlen(box->buffer);
        XMsgObjects_dump(DEFAULT_MSG_STORAGE_PATH);
        return HDStatus_sendAsJson;
    } else if (strcmp(api->value, "delete") == 0) {//ɾ��
        /*
            ˵����
                ����/api.cgi?api=delete&msgId=XXX&token=XXX
                ���أ�{"status":1}/{"status":0}
                ���壺���ڳ����غ�
         */
        if (!is_localhost) {
            do503ServiceUnavailable(data, client_sockfd);
            return HDStatus_continue;
        }
        SocketPair* SPmsgId = socketPairGET(data, "msgId", 1);
        SocketPair* SPtoken = socketPairGET(data, "token", 1);
        if (SPmsgId == NULL || SPmsgId->value == NULL || SPtoken == NULL || SPtoken->value == NULL) {
            do400BadRequest(data, client_sockfd);
            return HDStatus_continue;
        }
        User* user = User_getUserByToken(SPtoken->value);
        if (user == NULL) {
            do403Forbidden(data, client_sockfd);
            return HDStatus_continue;
        }
        XMsgObject* msg = XMsgObject_findMsgById(SPmsgId->value);
        if (msg == NULL) {
            do404NotFound(data, client_sockfd);
            return HDStatus_continue;
        }
        if (user->ownerID != msg->ownerID) {
            box->buffer = malloc(32);
            sprintf(box->buffer, "{\"status\":0}");
            box->size = strlen(box->buffer);
            XMsgObjects_dump(DEFAULT_MSG_STORAGE_PATH);
            return HDStatus_sendAsJson;
        }
        XMsgObjects_freeItem(msg);
        box->buffer = malloc(32);
        sprintf(box->buffer, "{\"status\":1}");
        box->size = strlen(box->buffer);
        XMsgObjects_dump(DEFAULT_MSG_STORAGE_PATH);
        return HDStatus_sendAsJson;
        /*
            �ļ��������������������⣬ֻ����Ƭ���ͣ��ı����ܳ���Ȼ����������Ҳ�����ˣ���ô�����ݸ��ﲻ���ļ��أ���
            ���ļ�ֻ�ָܷ�ͣ������֧�ֵĻ���Ŀǰ����С�����Ŀ�������ˣ����ļ������Ժ���˵���߲�֧�֣�
            ��Ƭ����˼·��
            ����������һ���ļ���Ϣ�Ͷ�Ӧ�ļ������֪ͨ��һ������Ƭ��ÿƬ����ܴ�С����Ϣ��
            Ȼ������Ƭд�����󣬸��߶��ĸ���Ƭд�룬д����٣�д���������ʲô
            �黹�ļ�������������չ�����Ϣ�б���Ŀ���ӽ��б��У�����ϴ�
            ������̣�
            filesplitsend->�õ��ϲ����ƣ���ʱĿ¼�д����ļ����ϲ���������д��
            filesplitwrite->Я������д����Ƭ
            filesplitmerge->Я������֪ͨ��ɺϲ���������Ϣ��¼����ʱ�ļ��ƶ�����ʽĿ¼���ϲ���ɣ�
            ���ʧ�ܣ���ʱ�ļ���ϵͳ�Զ�����Ͳ�����
        */
    } else if (strcmp(api->value, "filesplitsend") == 0) {
        /*
        ˵����
            ����/api.cgi?api=filesplitsend&token=XXX&type=XXX&msgName=XXX&fileName=XXX&fileSize=XXX&splitCount=XXX&splitSize=XXX
            ���أ�{"mergeId":XXX}
        */
        SocketPair* SPtoken = socketPairGET(data, "token", 1);
        SocketPair* SPtype = socketPairGET(data, "type", 1);
        SocketPair* SPmsgName = socketPairGET(data, "msgName", 1);
        SocketPair* SPfileName = socketPairGET(data, "fileName", 1);
        SocketPair* SPfileSize = socketPairGET(data, "fileSize", 1);
        SocketPair* SPsplitCount = socketPairGET(data, "splitCount", 1);
        SocketPair* SPsplitSize = socketPairGET(data, "splitSize", 1);
        if (SPtoken == NULL || SPtoken->value == NULL || SPtype == NULL || SPtype->value == NULL || SPmsgName == NULL || SPmsgName->value == NULL || SPfileName == NULL || SPfileName->value == NULL || SPfileSize == NULL || SPfileSize->value == NULL || SPsplitCount == NULL || SPsplitCount->value == NULL || SPsplitSize == NULL || SPsplitSize->value == NULL) {
            do400BadRequest(data, client_sockfd);
            return HDStatus_continue;
        }
        User* user = User_getUserByToken(SPtoken->value);
        if (user == NULL) {
            do403Forbidden(data, client_sockfd);
            return HDStatus_continue;
        }
        XMsgObjectType type = XMsgObjectType_BAD;
        if (strcmp(SPtype->value, "file") == 0) {
            type = XMsgObjectType_FILE;
        } else if (strcmp(SPtype->value, "image") == 0) {
            type = XMsgObjectType_IMAGE;
        } else if (strcmp(SPtype->value, "audio") == 0) {
            type = XMsgObjectType_AUDIO;
        } else if (strcmp(SPtype->value, "video") == 0) {
            type = XMsgObjectType_VIDEO;
        } else {
            do400BadRequest(data, client_sockfd);
            return HDStatus_continue;
        }
        int msgNamelen = strlen(SPmsgName->value);
        char* msgName = urlDecode(SPmsgName->value, &msgNamelen);
        if (msgNamelen > 64) {
            free(msgName);
            do400BadRequest(data, client_sockfd);
            return HDStatus_continue;
        }
        int fileNameLen = strlen(SPfileName->value);
        char* fileName = urlDecode(SPfileName->value, &fileNameLen);
        if (fileNameLen > 512) {
            free(msgName);
            free(fileName);
            do400BadRequest(data, client_sockfd);
            return HDStatus_continue;
        }
        int fileSize = atoi(SPfileSize->value);
        int splitCount = atoi(SPsplitCount->value);
        int splitSize = atoi(SPsplitSize->value);
        SplitMergeHolder* holder = malloc(sizeof(SplitMergeHolder));
        holder->type = type;
        holder->ownerID = user->ownerID;
        strcpy(holder->msgName, msgName);
        strcpy(holder->fileName, fileName);
        holder->fileSize = fileSize;
        holder->splitCount = splitCount;
        holder->splitSize = splitSize;
        holder->tmpFile = NULL;
        free(msgName);
        free(fileName);
        FILE* tmpFile = tmpfile();
        if (tmpFile == NULL) {
            free(holder);
            do500InternalServerError(data, client_sockfd);
            return HDStatus_continue;
        }
        holder->tmpFile = tmpFile;
        //���ƾ���holder���ڴ��ַ
        box->buffer = malloc(64);
        sprintf(box->buffer, "{\"mergeId\":\"%p\"}", holder);
        box->size = strlen(box->buffer);
        return HDStatus_sendAsTxt;
    } else if (strcmp(api->value, "filesplitwrite") == 0) {
        /*
        ˵����
            ����/api.cgi?api=filesplitwrite&mergeId=XXX&dataLen=XXX&data=XXX
            ���أ�
        */
        SocketPair* SPmergeId = socketPairGET(data, "mergeId", 1);
        SocketPair* SPdataLen = socketPairGET(data, "dataLen", 1);
        SocketPair* SPdata = socketPairGET(data, "data", 1);
        if (SPmergeId == NULL || SPmergeId->value == NULL || SPdataLen == NULL || SPdataLen->value == NULL || SPdata == NULL || SPdata->value == NULL) {
            do400BadRequest(data, client_sockfd);
            return HDStatus_continue;
        }
        SplitMergeHolder* holder = (SplitMergeHolder*)strtoll(SPmergeId->value, NULL, 16);
        if (holder == NULL) {
            do400BadRequest(data, client_sockfd);
            return HDStatus_continue;
        }
        if (holder->tmpFile == NULL) {
            do400BadRequest(data, client_sockfd);
            return HDStatus_continue;
        }
        int dataLen = atoi(SPdataLen->value);
        //data��base64���������
        int len = strlen(SPdata->value);
        char* data = base64_decode(SPdata->value, &len);
        if (len > 0) {
            fwrite(data, 1, dataLen, holder->tmpFile);
        }
        free(data);
        return HDStatus_normal;
    } else if (strcmp(api->value, "filesplitmerge") == 0) {
        /*
        ˵����
            ����/api.cgi?api=filesplitmerge&mergeId=XXX
            ���أ�{"status":0} �� {"status":1,"msgId":XXX}
        */
        SocketPair* SPmergeId = socketPairGET(data, "mergeId", 1);
        if (SPmergeId == NULL || SPmergeId->value == NULL) {
            do400BadRequest(data, client_sockfd);
            return HDStatus_continue;
        }
        SplitMergeHolder* holder = (SplitMergeHolder*)strtoll(SPmergeId->value, NULL, 16);
        if (holder == NULL) {
            do400BadRequest(data, client_sockfd);
            return HDStatus_continue;
        }
        if (holder->tmpFile == NULL) {
            free(holder);
            do400BadRequest(data, client_sockfd);
            return HDStatus_continue;
        }
        if (ftell(holder->tmpFile) != holder->fileSize) {
            free(holder);
            do500InternalServerError(data, client_sockfd);
            return HDStatus_continue;
        }
        XMsgObject* msg = XMsgObject_newclass_FileMsg(holder->type, holder->ownerID, NULL, holder->msgName, getTimeInfo(getTimestamp()), holder->fileName, NULL, holder->fileSize, 1);
        if (msg == NULL) {
            do500InternalServerError(data, client_sockfd);
            return HDStatus_continue;
        }
        storeTempFile(msg, holder->tmpFile);
        free(holder);
        XMsgObjects_dump(DEFAULT_MSG_STORAGE_PATH);
        box->buffer = malloc(512);
        sprintf(box->buffer, "{\"status\":1,\"msgId\":\"%s\"}", msg->msgId);
        box->size = strlen(box->buffer);
        return HDStatus_sendAsJson;
    }
    return HDStatus_normal;
}