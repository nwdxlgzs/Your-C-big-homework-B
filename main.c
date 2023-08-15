#include "proxyLib.h"
#include "liteiconv.h"
#include "util.h"
#include "msgObject.h"
#include "user.h"
SocketSetting* socketSetting;

int SEND_BUFF_SIZE = 4096;
// int RECV_BUFF_SIZE = 4096;
int RECV_BUFF_SIZE = 1024*1024*10;//10M(约定POST分片5MB（base64前），我总共给10MB，这样就可以了，虽然分片5MB依然很慢)
int main(int argc, char* argv[]) {
    srand(time(NULL));
    initAllMutex();//因为使用多线程的原因，所以要初始化锁
    UserList_load(DEFAULT_USER_STORAGE_PATH);
    XMsgObjects_load(DEFAULT_MSG_STORAGE_PATH);
    SocketSetting stackSocketSetting[64] = { 0 };//这个分配在栈空间的数组在main程序运行结束时才释放，正好不用malloc&free管理了
    socketSetting = stackSocketSetting;
    int sz;
    char** ips = getLocalIp(&sz);
    int port = WebServer_DefaultPort;
    for (int i = 0;i < sz;i++) {
        port += 100;
        printf("INFO: find local ip: %s, test port...\n", ips[i]);
        //寻找一个没有被占用的端口
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
    for (int i = 0;i < sz;i++) {//一定不是sz+1，因为sz+1是网站的端口不是工作的代理
        runOnThread(workproxy_launch, &socketSetting[i]);
    }
    VSCodePause;//等待用户按任意键退出，这样程序就可以停下来了(不要按任意键，否则会退出)
    mark_closeAllSocket();//标记关闭所有socket
    destroyAllMutex();//销毁锁
    return 0;
}


