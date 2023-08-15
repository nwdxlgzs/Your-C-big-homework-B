#ifndef UTIL_H
#define UTIL_H
#include "socketLib.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <stdio.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <iphlpapi.h> // for GetAdaptersAddresses ()
#pragma comment (lib, "Iphlpapi.lib")
#include <winerror.h>
#if (_WIN32_WINNT < 0x0501)
#define _WIN32_WINNT 0x0501
#endif
#include <iptypes.h>
//定义常量
#define WORKING_BUFFER_SIZE 15000
#define MAX_TRIES 3
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <ifaddrs.h> // for getifaddrs()
#endif
typedef struct TimeInfo {
    int year;//年
    int month;//月
    int day;//日
    int hour;//时
    int minute;//分
    int second;//秒
} TimeInfo;
typedef struct sBuffer {
    char* buff;//实际内容
    size_t size;//实际大小
    size_t cap;//内存容量
} sBuffer;
extern void sBuffer_init(sBuffer* buffer, int size);
extern void sBuffer_free(sBuffer* buffer);
extern void sBuffer_appendl(sBuffer* buffer, const char* data, int size);
#define sBuffer_append(buffer, data) sBuffer_appendl(buffer, data, strlen(data))
extern void sBuffer_addchar(sBuffer* buffer, char c);
extern void sBuffer_fmt(sBuffer* buffer, const char* fmt, ...);
extern void sBuffer_utf8jsonstr(sBuffer* buffer, char* text, int len);
//线程锁
#ifdef _WIN32
typedef CRITICAL_SECTION TLmutex_t;
#else
typedef pthread_mutex_t TLmutex_t;
#endif
//看定义的哪个锁，方便加解锁
typedef enum TypeMutex {
    TypeMutex_API,
}TypeMutex;
extern void initAllMutex();
extern void destroyAllMutex();
extern void lockMutex(TypeMutex type);
extern void unlockMutex(TypeMutex type);

extern void openInBrowser(const char* url);
extern void runOnThread(void* fptr, ...);
extern char** getLocalIp(int* len);

extern char* encHex(const char* data, int size);
extern char* decHex(const char* data, int* size);

extern char* fmtYmdHMS(const char* fmt, long long timestamp);
extern long long getTimestamp();
extern long long getTimestampByStr(const char* str);
extern TimeInfo getTimeInfo(long long timestamp);

extern int createDir(const char* path);
extern int isFileExist(const char* path);
extern int removeFile(const char* path);
extern char* base64_encode(const unsigned char* in, int* len, int urlsafe);
extern char* base64_decode(const char* in, int* len);
extern int TimeInfo_compare(TimeInfo* start, TimeInfo* end);

extern const unsigned char X_ctype_[UCHAR_MAX + 2];


#define ALPHABIT	0
#define DIGITBIT	1
#define PRINTBIT	2
#define SPACEBIT	3
#define XDIGITBIT	4
#define MASK(B)		(1 << (B))
#define testprop(c,p)	(X_ctype_[(c)+1] & (p))
#define lislalpha(c)	testprop(c, MASK(ALPHABIT))
#define lislalnum(c)	testprop(c, (MASK(ALPHABIT) | MASK(DIGITBIT)))
#define lisdigit(c)	testprop(c, MASK(DIGITBIT))
#define lisspace(c)	testprop(c, MASK(SPACEBIT))
#define lisprint(c)	testprop(c, MASK(PRINTBIT))
#define lisxdigit(c)	testprop(c, MASK(XDIGITBIT))
#define ltolower(c)	((c) | ('A' ^ 'a'))



#define VSCodePause system("pause>nul")
#endif // UTIL_H