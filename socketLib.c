/**
 * @file socketLib.c
 * @warning If you are not a developer, please do not modify this file unless you know what you are doing.
 */
#include "socketLib.h"
int closeAllSocket = 0;
void mark_closeAllSocket() {
    closeAllSocket = 1;
}
#ifdef _WIN32

int socket_init() {
    WSADATA wsa_data;
    return WSAStartup(MAKEWORD(2, 2), &wsa_data);
}

void socket_cleanup() {
    WSACleanup();
}
#else
int socket_init() {
    return 0;
}

void socket_cleanup() {
    return;
}
#endif

int socket_create() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket_create: failed to create socket\n");
        return -1;
    }
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) == -1) {
        perror("socket_create: failed to set SO_REUSEADDR option\n");
        return -1;
    }
    return sockfd;
}

int socket_bind(int sockfd, const char* host, int port, int log) {
    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (host != NULL) {
        if (strcmp(host, "localhost") == 0) {
            addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        } else {
            //判断是否是ip地址
            int isip = 1;
            for (int i = 0; i < strlen(host); i++) {
                if (host[i] != '.' && (host[i] < '0' || host[i] > '9')) {
                    isip = 0;
                    break;
                }
            }
            if (isip) {
                addr.sin_addr.s_addr = inet_addr(host);
            } else {
                struct hostent* hostinfo = gethostbyname(host);
                if (hostinfo == NULL) {
                    perror("socket_connect: failed to get host by name\n");
                    return -1;
                }
                addr.sin_addr = *(struct in_addr*)hostinfo->h_addr;
            }
        }
    }
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        if (log) {
            perror("socket_bind: failed to bind socket\n");
            printf("Error: try to check your port %d is not used by other application.\n"
            "cmd command: netstat -ano | findstr %d\n"
            "find that PID and kill it.\n", port, port);
        }
        return -1;
    }
    return 0;
}

int socket_listen(int sockfd) {
    if (listen(sockfd, 5) == -1) {
        perror("socket_listen: failed to listen on socket\n");
        return -1;
    }
    return 0;
}

int socket_accept(int sockfd) {
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    int connfd = accept(sockfd, (struct sockaddr*)&addr, &addrlen);
    if (connfd == -1) {
        perror("socket_accept: failed to accept connection\n");
        return -1;
    }
    return connfd;
}

int socket_connect(int sockfd, const char* host, int port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (host != NULL) {
        if (strcmp(host, "localhost") == 0) {
            addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        } else {
            //判断是否是ip地址
            int isip = 1;
            for (int i = 0; i < strlen(host); i++) {
                if (host[i] != '.' && (host[i] < '0' || host[i] > '9')) {
                    isip = 0;
                    break;
                }
            }
            if (isip) {
                addr.sin_addr.s_addr = inet_addr(host);
            } else {
                struct hostent* hostinfo = gethostbyname(host);
                if (hostinfo == NULL) {
                    perror("socket_connect: failed to get host by name\n");
                    return -1;
                }
                addr.sin_addr = *(struct in_addr*)hostinfo->h_addr;
            }
        }
    }
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("socket_connect: failed to connect to remote host\n");
        return -1;
    }
    return 0;
}

int socket_send(int sockfd, const char* data, int len) {
    int sent = send(sockfd, data, len, 0);
    if (sent == -1) {
        perror("socket_send: failed to send data\n");
        return -1;
    }
    return sent;
}
int socket_recv(int sockfd, char** buffer) {
    int received = 0;
    int buffSize = RECV_BUFF_SIZE;
    char* buff = (char*)malloc(buffSize);
    while (1) {
        int n = recv(sockfd, buff + received, RECV_BUFF_SIZE, 0);
        if (n == -1) {
            printf("Info: socket_recv: failed to receive data, maybe connection timeout.\n");
            if (buff)free(buff);
            return -1;
        }
        received += n;
        if (n == 0 || n < RECV_BUFF_SIZE) {
            // Connection closed
            break;
        }
        if (received == buffSize) {
            buffSize += RECV_BUFF_SIZE;
            buff = (char*)realloc(buff, buffSize);
        }
    }
    *buffer = buff;
    return received;
}

void socket_close(int sockfd) {
#ifdef _WIN32
    closesocket(sockfd);
#else
    close(sockfd);
#endif
}


int set_socket_timeout(int fd, int read_sec, int write_sec) {
    struct timeval send_timeval;
    struct timeval recv_timeval;
    if (fd <= 0) return -1;
    send_timeval.tv_sec = write_sec < 0 ? 0 : write_sec;
    send_timeval.tv_usec = 0;
    recv_timeval.tv_sec = read_sec < 0 ? 0 : read_sec;;
    recv_timeval.tv_usec = 0;
    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&send_timeval, sizeof(struct timeval)) == -1) {
        return -1;
    }
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&recv_timeval, sizeof(struct timeval)) == -1) {
        return -1;
    }
    return 0;
}

int recv_msg(int sockfd, char** buffer, int log) {
    int n = socket_recv(sockfd, buffer);
    if (n == -1) {
        printf("Info: failed to receive message from socket %d\n", sockfd);
        return -1;
    }
    if (n == 0) {
        printf("Info: connection closed by socket %d\n", sockfd);
        return 0;
    }
    (*buffer)[n] = '\0';
    if (log)
        printf(
            "\n===============>>>>>[Received message from socket %d]>>>>>===============\n"
            "%s"
            "\n===============<<<<<[Received message from socket %d]<<<<<===============\n", sockfd, *buffer, sockfd);
    return n;
}

void send_msg(int sockfd, const char* msg, int len, int log) {
    if (len == 0)len = strlen(msg);
    if (socket_send(sockfd, msg, len) == -1) {
        char err[100];
        sprintf(err, "Error: failed to send message to socket %d\n", sockfd);
        perror(err);
        return;
    }
    if (log)
        printf(
            "\n===============>>>>>[Sent message to socket %d]>>>>>================\n"
            "%s"
            "\n===============<<<<<[Sent message to socket %d]<<<<<===============\n", sockfd, msg, sockfd);
}

void freeSocketData(SocketData* data) {
    if (data == NULL)return;
    if (data->buff) free(data->buff);//很多数据都是指向buff的，所以只需要释放buff就可以了
    if (data->headers) free(data->headers);
    if (data->body) free(data->body);
    if (data->params) free(data->params);
    free(data);
}
SocketData* parseSocketData(char* buff, int buffSize) {
    // FILE* fp = fopen("rawSocketData.txt", "wb");
    // fwrite(buff, 1, buffSize, fp);
    // fclose(fp);
    SocketData* data = (SocketData*)malloc(sizeof(SocketData));
    data->buff = buff;
    data->buffSize = buffSize;
    data->method = SM_UNKNOWN;
    data->url = NULL;
    data->version = NULL;
    data->headers = NULL;
    data->headerSize = 0;
    data->body = NULL;
    data->bodySize = 0;
    data->params = NULL;
    data->paramSize = 0;
    //解析请求头
    char* p = buff;
    char* end = buff + buffSize;
    char* lineEnd = NULL;
    char* lineStart = p;
    while (p < end) {
        if (*p == '\r' && *(p + 1) == '\n') {
            lineEnd = p;
            *p = '\0';
            p += 2;
            if (lineStart == lineEnd) {
                //请求头结束
                break;
            }
            if (data->method == SM_UNKNOWN) {
                //解析请求方法
                if (strncmp(lineStart, "GET", 3) == 0) {
                    data->method = SM_GET;
                } else if (strncmp(lineStart, "POST", 4) == 0) {
                    data->method = SM_POST;
                } else if (strncmp(lineStart, "PUT", 3) == 0) {
                    data->method = SM_PUT;
                } else if (strncmp(lineStart, "DELETE", 6) == 0) {
                    data->method = SM_DELETE;
                } else if (strncmp(lineStart, "HEAD", 4) == 0) {
                    data->method = SM_HEAD;
                } else if (strncmp(lineStart, "OPTIONS", 7) == 0) {
                    data->method = SM_OPTIONS;
                } else if (strncmp(lineStart, "TRACE", 5) == 0) {
                    data->method = SM_TRACE;
                } else if (strncmp(lineStart, "CONNECT", 7) == 0) {
                    data->method = SM_CONNECT;
                } else if (strncmp(lineStart, "PATCH", 5) == 0) {
                    data->method = SM_PATCH;
                }
                //解析请求URL
                char* urlStart = lineStart;
                while (*urlStart != ' ') {
                    urlStart++;
                }
                *urlStart = '\0';
                urlStart++;
                data->url = urlStart;
                //解析请求版本
                char* versionStart = urlStart;
                while (*versionStart != ' ') {
                    versionStart++;
                }
                *versionStart = '\0';
                versionStart++;
                data->version = versionStart;
            } else {
                //解析请求头
                char* keyStart = lineStart;
                char* keyEnd = keyStart;
                while (*keyEnd != ':') {
                    keyEnd++;
                }
                *keyEnd = '\0';
                keyEnd++;
                char* valueStart = keyEnd;
                while (*valueStart == ' ') {
                    valueStart++;
                }
                char* valueEnd = valueStart;
                while (*valueEnd != '\0') {
                    valueEnd++;
                }
                valueEnd--;
                while (*valueEnd == ' ') {
                    valueEnd--;
                }
                valueEnd++;
                *valueEnd = '\0';
                data->headerSize++;
                data->headers = (SocketPair*)realloc(data->headers, sizeof(SocketPair) * data->headerSize);
                data->headers[data->headerSize - 1].key = keyStart;
                data->headers[data->headerSize - 1].value = valueStart;
            }
            lineStart = p;
        } else {
            p++;
        }
    }
    //解析请求体
    switch (data->method) {
        case SM_POST:
            {//这种后头是数据
                for (int i = 0; i < data->headerSize; i++) {
                    if (strcasecmp(data->headers[i].key, "Content-Length") == 0) {
                        data->bodySize = atoi(data->headers[i].value);
                        data->body = malloc(data->bodySize + 1);
                        memcpy(data->body, p, data->bodySize);
                        data->body[data->bodySize] = '\0';
                        break;
                    }
                }
                break;
            }
        case SM_GET:
            {//这种在url后面
                char* paramStart = data->url;
                while (*paramStart != '?' && *paramStart != '\0') {
                    paramStart++;
                }
                if (*paramStart == '?') {
                    char* paramEnd = paramStart;
                    while (*paramEnd != '\0') {
                        paramEnd++;
                    }
                    paramEnd--;
                    while (*paramEnd == ' ') {
                        paramEnd--;
                    }
                    paramEnd++;
                    *paramEnd = '\0';
                    data->bodySize = paramEnd - paramStart;
                    data->body = malloc(data->bodySize + 1);
                    memcpy(data->body, paramStart + 1, data->bodySize);
                    data->body[data->bodySize] = '\0';
                }
                break;
            }
    }
    if (data->body != NULL) {
        //解析请求体参数
        char* p = data->body;
        char* end = data->body + data->bodySize;
        char* lineStart = p;
        int early_stop = 0;//提前结束
        while (p < end) {
            if (*p == '&' || p == end - 1) {
                if (p == end - 1) {
                    p++;
                }
                *p = '\0';
                p++;
                char* keyStart = lineStart;
                char* keyEnd = keyStart;
                while (*keyEnd != '=') {
                    keyEnd++;
                    if (keyEnd == end) {
                        early_stop = 1;
                        break;
                    }
                }
                if (early_stop)break;
                *keyEnd = '\0';
                keyEnd++;
                char* valueStart = keyEnd;
                char* valueEnd = valueStart;
                while (*valueEnd != '\0') {
                    valueEnd++;
                }
                valueEnd--;
                while (*valueEnd == ' ') {
                    valueEnd--;
                }
                valueEnd++;
                *valueEnd = '\0';
                data->paramSize++;
                data->params = (SocketPair*)realloc(data->params, sizeof(SocketPair) * data->paramSize);
                data->params[data->paramSize - 1].key = keyStart;
                data->params[data->paramSize - 1].value = valueStart;
                lineStart = p;
            } else {
                p++;
            }
        }
    }
    return data;
}
SocketPair* socketPairGET(SocketData* data, char* key, int ignoreCase) {
    for (int i = 0; i < data->paramSize; i++) {
        if (ignoreCase) {
            if (strcasecmp(data->params[i].key, key) == 0) {
                return &data->params[i];
            }
        } else {
            if (strcmp(data->params[i].key, key) == 0) {
                return &data->params[i];
            }
        }
    }
    for (int i = 0; i < data->headerSize; i++) {
        if (ignoreCase) {
            if (strcasecmp(data->headers[i].key, key) == 0) {
                return &data->headers[i];
            }
        } else {
            if (strcmp(data->headers[i].key, key) == 0) {
                return &data->headers[i];
            }
        }
    }
    return NULL;
}
void do400BadRequest(SocketData* data, SOCKET connfd) {
    freeSocketData(data);
    send_msg(connfd, "HTTP/1.1 400 Bad Request\r\nAccess-Control-Allow-Origin: *\r\n\r\n", 0, 1);
    socket_close(connfd);
}
void do401Unauthorized(SocketData* data, SOCKET connfd) {
    freeSocketData(data);
    send_msg(connfd, "HTTP/1.1 401 Unauthorized\r\nAccess-Control-Allow-Origin: *\r\n\r\n", 0, 1);
    socket_close(connfd);
}
void do403Forbidden(SocketData* data, SOCKET connfd) {
    freeSocketData(data);
    send_msg(connfd, "HTTP/1.1 403 Forbidden\r\nAccess-Control-Allow-Origin: *\r\n\r\n", 0, 1);
    socket_close(connfd);
}
void do404NotFound(SocketData* data, SOCKET connfd) {
    freeSocketData(data);
    send_msg(connfd, "HTTP/1.1 404 Not Found\r\nAccess-Control-Allow-Origin: *\r\n\r\n", 0, 1);
    socket_close(connfd);
}
void do405MethodNotAllowed(SocketData* data, SOCKET connfd) {
    freeSocketData(data);
    send_msg(connfd, "HTTP/1.1 405 Method Not Allowed\r\nAccess-Control-Allow-Origin: *\r\n\r\n", 0, 1);
    socket_close(connfd);
}
void do500InternalServerError(SocketData* data, SOCKET connfd) {
    freeSocketData(data);
    send_msg(connfd, "HTTP/1.1 500 Internal Server Error\r\nAccess-Control-Allow-Origin: *\r\n\r\n", 0, 1);
    socket_close(connfd);
}
void do501NotImplemented(SocketData* data, SOCKET connfd) {
    freeSocketData(data);
    send_msg(connfd, "HTTP/1.1 501 Not Implemented\r\nAccess-Control-Allow-Origin: *\r\n\r\n", 0, 1);
    socket_close(connfd);
}
void do502BadGateway(SocketData* data, SOCKET connfd) {
    freeSocketData(data);
    send_msg(connfd, "HTTP/1.1 502 Bad Gateway\r\nAccess-Control-Allow-Origin: *\r\n\r\n", 0, 1);
    socket_close(connfd);
}
void do503ServiceUnavailable(SocketData* data, SOCKET connfd) {
    freeSocketData(data);
    send_msg(connfd, "HTTP/1.1 503 Service Unavailable\r\nAccess-Control-Allow-Origin: *\r\n\r\n", 0, 1);
    socket_close(connfd);
}
void do504GatewayTimeout(SocketData* data, SOCKET connfd) {
    freeSocketData(data);
    send_msg(connfd, "HTTP/1.1 504 Gateway Timeout\r\nAccess-Control-Allow-Origin: *\r\n\r\n", 0, 1);
    socket_close(connfd);
}
