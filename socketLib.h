/**
 * @file socketLib.h
 * @warning If you are not a developer, please do not modify this file unless you know what you are doing.
 */
#ifndef SOCKETLIB_H
#define SOCKETLIB_H

 //�շ������С��̫�������⣬̫С��Ӱ�����ܣ�ֻ�ǽ����С�������������ϵͳ���䣩
extern int SEND_BUFF_SIZE;
extern int RECV_BUFF_SIZE;
typedef struct SocketSetting {
    char host[129];//����
    int port;//�˿�
    int timeout;//��ʱʱ��
    int is_localhost;//�Ƿ���localhost
    struct SocketSetting* next;
} SocketSetting;
extern SocketSetting* socketSetting;
typedef struct BufferBox {
    char* buffer;
    int size;
} BufferBox;
extern int closeAllSocket;//�ر�����socket�ı�־

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#endif


typedef struct SocketPair {
    char* key;
    char* value;
} SocketPair;
typedef enum SocketMethod {
    SM_GET,
    SM_POST,
    SM_PUT,
    SM_DELETE,
    SM_HEAD,
    SM_OPTIONS,
    SM_TRACE,
    SM_CONNECT,
    SM_PATCH,
    SM_UNKNOWN,
} SocketMethod;
typedef struct SocketData {
    char* buff;int buffSize;//ʵ������
    SocketMethod method;
    char* url;
    char* version;
    SocketPair* headers;
    int headerSize;
    char* body;
    int bodySize;
    SocketPair* params;
    int paramSize;
} SocketData;
extern int socket_init();
extern void socket_cleanup();
extern int socket_create();
extern int socket_bind(int sockfd, const char* host, int port, int log);
extern int socket_listen(int sockfd);
extern int socket_accept(int sockfd);
extern int socket_connect(int sockfd, const char* host, int port);
extern int socket_send(int sockfd, const char* data, int len);
extern int socket_recv(int sockfd, char** buffer);
extern void socket_close(int sockfd);
extern int set_socket_timeout(int fd, int read_sec, int write_sec);
extern int recv_msg(int sockfd, char** buffer, int log);
extern void send_msg(int sockfd, const char* msg, int len, int log);
extern void freeSocketData(SocketData* data);
extern SocketData* parseSocketData(char* buff, int buffSize);
extern SocketPair* socketPairGET(SocketData* data, char* key, int ignoreCase);
extern void mark_closeAllSocket();
//һЩԤ����Ĵ�����Ӧ�룬ֱ�ӵ��ü���
extern void do400BadRequest(SocketData* data, SOCKET connfd);
extern void do401Unauthorized(SocketData* data, SOCKET connfd);
extern void do403Forbidden(SocketData* data, SOCKET connfd);
extern void do404NotFound(SocketData* data, SOCKET connfd);
extern void do405MethodNotAllowed(SocketData* data, SOCKET connfd);
extern void do500InternalServerError(SocketData* data, SOCKET connfd);
extern void do501NotImplemented(SocketData* data, SOCKET connfd);
extern void do502BadGateway(SocketData* data, SOCKET connfd);
extern void do503ServiceUnavailable(SocketData* data, SOCKET connfd);
extern void do504GatewayTimeout(SocketData* data, SOCKET connfd);

#endif