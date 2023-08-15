#include "proxyLib.h"
#include "liteiconv.h"
#include "util.h"
#include "msgObject.h"
#include "user.h"
SocketSetting* socketSetting;

int SEND_BUFF_SIZE = 4096;
// int RECV_BUFF_SIZE = 4096;
int RECV_BUFF_SIZE = 1024*1024*10;//10M(Լ��POST��Ƭ5MB��base64ǰ�������ܹ���10MB�������Ϳ����ˣ���Ȼ��Ƭ5MB��Ȼ����)
int main(int argc, char* argv[]) {
    srand(time(NULL));
    initAllMutex();//��Ϊʹ�ö��̵߳�ԭ������Ҫ��ʼ����
    UserList_load(DEFAULT_USER_STORAGE_PATH);
    XMsgObjects_load(DEFAULT_MSG_STORAGE_PATH);
    SocketSetting stackSocketSetting[64] = { 0 };//���������ջ�ռ��������main�������н���ʱ���ͷţ����ò���malloc&free������
    socketSetting = stackSocketSetting;
    int sz;
    char** ips = getLocalIp(&sz);
    int port = WebServer_DefaultPort;
    for (int i = 0;i < sz;i++) {
        port += 100;
        printf("INFO: find local ip: %s, test port...\n", ips[i]);
        //Ѱ��һ��û�б�ռ�õĶ˿�
        while (1) {
            if (socket_init() == -1) {
                perror("Error: failed to initialize socket library\n");
                return 1;
            }
            int server_sockfd = socket_create();
            if (server_sockfd == -1) {
                perror("Error: failed to create server socket\n");
                return 1;
            }
            if (socket_bind(server_sockfd, ips[i], port, 0) == -1) {
                port++;
                continue;
            }
            socket_close(server_sockfd);
            socket_cleanup();
            break;
        }
        printf("INFO: %s use port: %d\n", ips[i], port);
        socketSetting[i].port = port;
        strcpy(socketSetting[i].host, ips[i]);
        socketSetting[i].is_localhost = 0;
        socketSetting[i].timeout = workproxy_Timeout;
    }
    socketSetting[sz].port = website_port;
    strcpy(socketSetting[sz].host, WebServer_Host);
    socketSetting[sz].is_localhost = 1;
    socketSetting[sz].timeout = 60;
    int isOpenBrowser = 1;
    runOnThread(webServer_launch, &isOpenBrowser);
    for (int i = 0;i < sz;i++) {//һ������sz+1����Ϊsz+1����վ�Ķ˿ڲ��ǹ����Ĵ���
        runOnThread(workproxy_launch, &socketSetting[i]);
    }
    VSCodePause;//�ȴ��û���������˳�����������Ϳ���ͣ������(��Ҫ���������������˳�)
    mark_closeAllSocket();//��ǹر�����socket
    destroyAllMutex();//������
    return 0;
}


