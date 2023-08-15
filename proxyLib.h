#ifndef PROXYLIB_H
#define PROXYLIB_H
typedef enum HDStatus {
    HDStatus_normal,
    HDStatus_continue,
    HDStatus_send,
    HDStatus_sendAsTxt,
    HDStatus_sendAsJson,
    HDStatus_sendAsStream,
}HDStatus;
#include "socketLib.h"
#include "util.h"
#include "user.h"
#include "msgObject.h"
#include "webServerRes.h"
#define WebServer_Host "127.0.0.1"//"localhost"
#define WebServer_DefaultPort 18081
#define workproxy_Timeout 604800//工作代理设置接收一周的timeout时间总够了吧
extern int website_port;
extern void webServer_launch(int* isOpenBrowser);
extern void workproxy_launch(SocketSetting* ss);
extern HDStatus globalHandle(const char* host, int port, BufferBox* box, int client_sockfd, SocketData* data, int is_localhost);
#endif // PROXYLIB_H