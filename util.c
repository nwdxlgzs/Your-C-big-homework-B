#include "util.h"
#ifdef _WIN32
#include <process.h>
#include <windows.h>
#else
#include <pthread.h>
#endif
void openInBrowser(const char* url) {
    char cmd[1024];
#ifdef _WIN32
    //windows
    sprintf(cmd, "start %s", url);
#elif defined __APPLE__
    //mac
    sprintf(cmd, "open %s", url);
#else
    //linux 
    sprintf(cmd, "xdg-open %s", url);
#endif
    system(cmd);
}

void runOnThread(void* fptr, ...) {
    void* arg = NULL;
    if (fptr == NULL) {
        return;
    }
    //获取参数
    va_list args;
    va_start(args, fptr);
    arg = va_arg(args, void*);
    va_end(args);
    //转换函数指针
    void* (*func)(void*) = fptr;
#ifdef _WIN32
    // Windows
    _beginthreadex(NULL, 0, (unsigned int(__stdcall*)(void*))func, arg, 0, NULL);
#else
    // Linux or macOS
    pthread_t tid;
    pthread_create(&tid, NULL, func, arg);
#endif
}





#ifdef _WIN32
#ifndef inet_ntop
#define inet_ntop
#endif
void GetIPv4Address(char* destIP, char* ipv4) {
    //初始化变量
    DWORD dwRetVal = 0;
    ULONG outBufLen = 0;
    DWORD dwBestIfIndex = 0;
    PIP_ADAPTER_ADDRESSES pAddresses = NULL;
    PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
    PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;
    int i = 0;
    //获取最佳网卡序号
    dwRetVal = GetBestInterface(inet_addr(destIP), &dwBestIfIndex);
    if (dwRetVal != NO_ERROR) {
        printf("GetBestInterface failed with error: %d\n", dwRetVal);
        return;
    }
    //分配内存空间
    outBufLen = WORKING_BUFFER_SIZE;
    pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
    //获取所有网卡信息
    dwRetVal = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, pAddresses, &outBufLen);
    if (dwRetVal != NO_ERROR) {
        printf("GetAdaptersAddresses failed with error: %d\n", dwRetVal);
        free(pAddresses);
        return;
    }
    //遍历所有网卡，找到最佳网卡
    pCurrAddresses = pAddresses;
    while (pCurrAddresses) {
        if (pCurrAddresses->IfIndex == dwBestIfIndex) {
            //找到最佳网卡，遍历其单播地址，找到IPv4地址
            pUnicast = pCurrAddresses->FirstUnicastAddress;
            while (pUnicast) {
                if (pUnicast->Address.lpSockaddr->sa_family == AF_INET) {
                    //找到IPv4地址，转换为字符串并赋值给ipv4参数
                    struct sockaddr_in* sa_in = (struct sockaddr_in*)pUnicast->Address.lpSockaddr;
                    inet_ntop(AF_INET, &(sa_in->sin_addr), ipv4, INET_ADDRSTRLEN);
                    break;
                }
                pUnicast = pUnicast->Next;
            }
            break;
        }
        pCurrAddresses = pCurrAddresses->Next;
    }
    //释放内存空间
    free(pAddresses);
}
#else
void GetIPv4Address(char* destIP, char* ipv4) {
    //初始化变量
    int sockfd = 0;
    int ret = 0;
    struct ifreq ifr;
    struct sockaddr_in destAddr;
    struct sockaddr_in* sa_in = NULL;
    //创建套接字
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket failed");
        return;
    }
    //设置目标地址
    memset(&destAddr, 0, sizeof(destAddr));
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(53); //使用DNS端口
    ret = inet_pton(AF_INET, destIP, &destAddr.sin_addr.s_addr);
    if (ret != 1) {
        perror("inet_pton failed");
        close(sockfd);
        return;
    }
    //连接目标地址
    ret = connect(sockfd, (struct sockaddr*)&destAddr, sizeof(destAddr));
    if (ret == -1) {
        perror("connect failed");
        close(sockfd);
        return;
    }
    //获取本地地址
    memset(&ifr, 0, sizeof(ifr));
    ret = getsockname(sockfd, (struct sockaddr*)&ifr.ifr_addr, sizeof(ifr.ifr_addr));
    if (ret == -1) {
        perror("getsockname failed");
        close(sockfd);
        return;
    }
    //转换为字符串并赋值给ipv4参数
    sa_in = (struct sockaddr_in*)&ifr.ifr_addr;
    inet_ntop(AF_INET, &(sa_in->sin_addr), ipv4, INET_ADDRSTRLEN);
    //关闭套接字
    close(sockfd);
}
#endif

char** getLocalIp(int* len) {
    *len = 0;
    int i;
    char** ip_list = malloc(32 * sizeof(char*));
    for (i = 0; i < 32; i++) {
        ip_list[i] = malloc(129);
    }
    i = 0;
    if (socket_init() == -1) {
        perror("Error: failed to initialize socket library\n");
        return NULL;
    }
#ifdef _WIN32
    PIP_ADAPTER_INFO pAdapterInfo;
    PIP_ADAPTER_INFO pAdapter = NULL;
    DWORD dwRetVal = 0;
    ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
    pAdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
    if (pAdapterInfo == NULL) {
        perror("Error: failed to allocate memory for adapter info\n");
        return NULL;
    }
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
        if (pAdapterInfo == NULL) {
            perror("Error: failed to allocate memory for adapter info\n");
            return NULL;
        }
    }
    if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
        pAdapter = pAdapterInfo;
        while (pAdapter) {
            if (strcmp(pAdapter->IpAddressList.IpAddress.String, "0.0.0.0") != 0 && i < 32) {
                sprintf(ip_list[i++], "%s", pAdapter->IpAddressList.IpAddress.String);
                *len = i;
                // printf("%s: %s\n", pAdapter->AdapterName, pAdapter->IpAddressList.IpAddress.String);
            }
            pAdapter = pAdapter->Next;
        }
    } else {
        perror("Error: failed to get adapter info\n");
        return NULL;
    }
    if (pAdapterInfo)
        free(pAdapterInfo);
#else
    struct ifaddrs* ifaddr, * ifa;
    if (getifaddrs(&ifaddr) == -1) {
        perror("Error: failed to get network interfaces\n");
        return NULL;
    }
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;
        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in* pAddr = (struct sockaddr_in*)ifa->ifa_addr;
            snprintf(ip, 129, "%s", inet_ntoa(pAddr->sin_addr));
            // printf("%s: %s\n", ifa->ifa_name, ip);
            if (strcmp(ip, "0.0.0.0") != 0 && i < 32) {
                sprintf(ip_list[i++], "%s", ip);
                *len = i;
            }
        }
    }
    freeifaddrs(ifaddr);
#endif
    socket_cleanup();
    return ip_list;
}
static TLmutex_t API_mutex_t;

// 初始化互斥锁
void initAllMutex() {
#ifdef _WIN32
    InitializeCriticalSection(&API_mutex_t);
#else
    pthread_mutex_init(&API_mutex_t, NULL);
#endif
}

void destroyAllMutex() {
#ifdef _WIN32
    DeleteCriticalSection(&API_mutex_t);
#else
    pthread_mutex_destroy(&API_mutex_t);
#endif
}
static inline void impl_switchMutex(TypeMutex type, int is_lock) {
    TLmutex_t* mutex = NULL;
    switch (type) {
        case TypeMutex_API:
            {
                mutex = &API_mutex_t;
                break;
            }
        default:
            return;
    }
    if (mutex == NULL)return;
    if (is_lock) {
#ifdef _WIN32
        EnterCriticalSection(mutex);
#else
        pthread_mutex_lock(mutex);
#endif
    } else {
#ifdef _WIN32
        LeaveCriticalSection(mutex);
#else
        pthread_mutex_unlock(mutex);
#endif
    }
}
void lockMutex(TypeMutex type) {
    impl_switchMutex(type, 1);
}
void unlockMutex(TypeMutex type) {
    impl_switchMutex(type, 0);
}


char* encHex(const char* data, int size) {
    if (data == NULL) return NULL;
    if (size == -1) size = strlen(data);
    char* hex = malloc(size * 2 + 1);
    for (int i = 0; i < size; i++) {
        sprintf(hex + i * 2, "%02x", data[i]);
    }
    hex[size * 2] = '\0';
    return hex;
}
#define __HEX__(x)  (((x) >= '0' && (x) <= '9') ? ((x) - '0') : (((x) >= 'a' && (x) <= 'f') ? ((x) - 'a' + 10) : (((x) >= 'A' && (x) <= 'F') ? ((x) - 'A' + 10) : 0)))
char* decHex(const char* hex, int* size) {
    if (hex == NULL) return NULL;
    int len;
    if (size != NULL) {
        len = *size;
        *size = 0;
    } else {
        len = strlen(hex);
    }
    char* data = malloc(len / 2 + 1);
    for (int i = 0; i < len; i += 2) {
        data[i / 2] = (__HEX__(hex[i]) << 4) | (__HEX__(hex[i + 1]));
    }
    data[len / 2] = '\0';
    if (size != NULL)
        *size = len / 2;
    return data;
}

const unsigned char X_ctype_[UCHAR_MAX + 2] = {
  0x00,  /* EOZ */
  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,	/* 0. */
  0x00,  0x08,  0x08,  0x08,  0x08,  0x08,  0x00,  0x00,
  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,	/* 1. */
  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,
  0x0c,  0x04,  0x04,  0x04,  0x04,  0x04,  0x04,  0x04,	/* 2. */
  0x04,  0x04,  0x04,  0x04,  0x04,  0x04,  0x04,  0x04,
  0x16,  0x16,  0x16,  0x16,  0x16,  0x16,  0x16,  0x16,	/* 3. */
  0x16,  0x16,  0x04,  0x04,  0x04,  0x04,  0x04,  0x04,
  0x04,  0x15,  0x15,  0x15,  0x15,  0x15,  0x15,  0x05,	/* 4. */
  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,
  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,	/* 5. */
  0x05,  0x05,  0x05,  0x04,  0x04,  0x04,  0x04,  0x05,
  0x04,  0x15,  0x15,  0x15,  0x15,  0x15,  0x15,  0x05,	/* 6. */
  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,
  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,	/* 7. */
  0x05,  0x05,  0x05,  0x04,  0x04,  0x04,  0x04,  0x00,
  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,	/* e. */
  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,
  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,	/* e. */
  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,
  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,	/* e. */
  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,
  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,	/* e. */
  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,
  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,	/* c. */
  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,
  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,	/* d. */
  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,
  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,	/* e. */
  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,  0x05,
  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,	/* f. */
  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,
};

void sBuffer_init(sBuffer* buffer, int size) {
    buffer->size = 0;
    buffer->cap = size;
    buffer->buff = (char*)malloc(size);
}

void sBuffer_free(sBuffer* buffer) {
    if (buffer->buff)free(buffer->buff);
    buffer->buff = NULL;
    buffer->size = 0;
    buffer->cap = 0;
}
void sBuffer_appendl(sBuffer* buffer, const char* data, int size) {
    if (buffer->size + size + 1 >= buffer->cap) {
        buffer->cap = buffer->size + size + 1024;//额外预留1024字节
        buffer->buff = (char*)realloc(buffer->buff, buffer->cap);
    }
    memcpy(buffer->buff + buffer->size, data, size);
    buffer->size += size;
    buffer->buff[buffer->size] = 0;
}

void sBuffer_addchar(sBuffer* buffer, char c) {
    if (buffer->size + 2 >= buffer->cap) {
        buffer->cap = buffer->size + 1024;//额外预留1024字节
        buffer->buff = (char*)realloc(buffer->buff, buffer->cap);
    }
    buffer->buff[buffer->size] = c;
    buffer->size++;
    buffer->buff[buffer->size] = 0;
}

void sBuffer_fmt(sBuffer* buffer, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int size = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (buffer->size + size + 1 >= buffer->cap) {
        buffer->cap = buffer->size + size + 1024;//额外预留1024字节
        buffer->buff = (char*)realloc(buffer->buff, buffer->cap);
    }
    va_start(ap, fmt);
    vsnprintf(buffer->buff + buffer->size, size + 1, fmt, ap);
    va_end(ap);
    buffer->size += size;
    buffer->buff[buffer->size] = 0;
}

void sBuffer_utf8jsonstr(sBuffer* buffer, char* text, int len) {
    for (int j = 0; j < len; j++) {
        unsigned char c = text[j];
        if (c == '\"' || c == '\\') {
            sBuffer_addchar(buffer, '\\');
            sBuffer_addchar(buffer, c);
        } else if (c == '\n') {
            sBuffer_addchar(buffer, '\\');
            sBuffer_addchar(buffer, 'n');
        } else if (c == '\r') {
            sBuffer_addchar(buffer, '\\');
            sBuffer_addchar(buffer, 'r');
        } else if (c == '\t') {
            sBuffer_addchar(buffer, '\\');
            sBuffer_addchar(buffer, 't');
        } else if (lisprint(c)) {
            sBuffer_addchar(buffer, c);
        } else {
            sBuffer_fmt(buffer, "\\u00%02X", c);
        }
    }
}

char* fmtYmdHMS(const char* fmt, long long timestamp) {
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    if (timestamp != 0) {
        t = timestamp / 1000;
        tm = localtime(&t);
    }
    char* buf = malloc(64);
    if (fmt == NULL) {
        strftime(buf, 64, "%Y-%m-%d %H:%M:%S", tm);
    } else {
        strftime(buf, 64, fmt, tm);
    }
    return buf;
}

//Unix时间戳（省略毫秒）
long long getTimestamp() {
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    time_t timep;
    timep = mktime(tm);
    return (timep * 1000) + 0;
}
//Unix时间戳（省略毫秒）
long long getTimestampByStr(const char* str) {
    struct tm tm;
    time_t timep;
    sscanf(str, "%d-%d-%d %d:%d:%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
    tm.tm_year -= 1900;
    tm.tm_mon -= 1;
    timep = mktime(&tm);
    //FMT已经不支持毫秒了
    return timep * 1000 + 0;
}

TimeInfo getTimeInfo(long long timestamp) {
    time_t t = timestamp / 1000;
    struct tm* tm = localtime(&t);
    TimeInfo info;
    info.year = tm->tm_year + 1900;
    info.month = tm->tm_mon + 1;
    info.day = tm->tm_mday;
    info.hour = tm->tm_hour;
    info.minute = tm->tm_min;
    info.second = tm->tm_sec;
    return info;
}

// 创建目录
int createDir(const char* path) {
    if (path == NULL)   return 0;
    char buf[4096];
    int len = strlen(path);
    if (len > 4096) return 0;
    strcpy(buf, path);
#ifdef _WIN32
    //windows
    if (buf[len - 1] != '\\') { // use backslash for Windows
        strcat(buf, "\\");
    }
    len = strlen(buf);
    for (int i = 1; i < len; i++) {
        if (buf[i] == '\\') { // use backslash for Windows
            buf[i] = '\0';
            if (_access(buf, 0) != 0) { // use _access for Windows
                if (mkdir(buf) == -1) { // use _mkdir for Windows
                    return 0;
                }
            }
            buf[i] = '\\';
        }
    }
#else
    //linux/mac
    if (buf[len - 1] != '/') {
        strcat(buf, "/");
    }
    len = strlen(buf);
    for (int i = 1; i < len; i++) {
        if (buf[i] == '/') {
            buf[i] = '\0';
            if (access(buf, NULL) != 0) {
                if (mkdir(buf, 0777) == -1) {
                    return 0;
                }
            }
            buf[i] = '/';
        }
    }
#endif
    return 1;
}

int isFileExist(const char* path) {
    if (path == NULL)   return 0;
    FILE* fp = fopen(path, "r");
    if (fp == NULL) {
        return 0;
    }
    fclose(fp);
    return 1;
}

int removeFile(const char* path) {
    if (path == NULL)   return 0;
    if (isFileExist(path)) {
        return remove(path);
    }
    return 0;
}


static const char normal_base64en[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '+', '/',
};
static const char urlsafe_base64en[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '-', '_',
};
// 调整了+-/_对应值，这样可以同时支持urlsafe和normal自动识别处理
static const unsigned char base64de[] = {
    /* nul, soh, stx, etx, eot, enq, ack, bel, */
    255, 255, 255, 255, 255, 255, 255, 255,
    /*  bs,  ht,  nl,  vt,  np,  cr,  so,  si, */
    255, 255, 255, 255, 255, 255, 255, 255,
    /* dle, dc1, dc2, dc3, dc4, nak, syn, etb, */
    255, 255, 255, 255, 255, 255, 255, 255,
    /* can,  em, sub, esc,  fs,  gs,  rs,  us, */
    255, 255, 255, 255, 255, 255, 255, 255,
    /*  sp, '!', '"', '#', '$', '%', '&', ''', */
    255, 255, 255, 255, 255, 255, 255, 255,
    /* '(', ')', '*', '+', ',', '-', '.', '/', */
    255, 255, 255,  62, 255, 62, 255,  63,
    /* '0', '1', '2', '3', '4', '5', '6', '7', */
    52,  53,  54,  55,  56,  57,  58,  59,
    /* '8', '9', ':', ';', '<', '=', '>', '?', */
    60,  61, 255, 255, 255, 255, 255, 255,
    /* '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', */
    255,   0,   1,  2,   3,   4,   5,    6,
    /* 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', */
    7,   8,   9,  10,  11,  12,  13,  14,
    /* 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', */
    15,  16,  17,  18,  19,  20,  21,  22,
    /* 'X', 'Y', 'Z', '[', '\', ']', '^', '_', */
    23,  24,  25, 255, 255, 255, 255, 63,
    /* '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', */
    255,  26,  27,  28,  29,  30,  31,  32,
    /* 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', */
    33,  34,  35,  36,  37,  38,  39,  40,
    /* 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', */
    41,  42,  43,  44,  45,  46,  47,  48,
    /* 'x', 'y', 'z', '{', '|', '}', '~', del, */
    49,  50,  51, 255, 255, 255, 255, 255
};

char* base64_encode(const unsigned char* in, int* len, int urlsafe) {
    if (in == NULL || len == NULL || *len <= 0)return NULL;
    const char* base64en = urlsafe ? urlsafe_base64en : base64en;
    int s = 0;
    unsigned int i, j, inlen = *len;
    unsigned char c, l = 0;
    char* out = (char*)malloc(((inlen + 128) + 3) / 3 * 4 + 1);
    for (i = j = 0; i < inlen; i++) {
        c = in[i];
        switch (s) {
            case 0:
                s = 1;
                out[j++] = base64en[(c >> 2) & 0x3F];
                break;
            case 1:
                s = 2;
                out[j++] = base64en[((l & 0x3) << 4) | ((c >> 4) & 0xF)];
                break;
            case 2:
                s = 0;
                out[j++] = base64en[((l & 0xF) << 2) | ((c >> 6) & 0x3)];
                out[j++] = base64en[c & 0x3F];
                break;
        }
        l = c;
    }
    switch (s) {
        case 1:
            out[j++] = base64en[(l & 0x3) << 4];
            out[j++] = '=';
            out[j++] = '=';
            break;
        case 2:
            out[j++] = base64en[(l & 0xF) << 2];
            out[j++] = '=';
            break;
    }
    out[j] = 0;
    *len = j;
    return out;
}

char* base64_decode(const char* in, int* len) {
    if (in == NULL || len == NULL || *len <= 0)return NULL;
    unsigned int i, j, inlen = *len;
    unsigned char c;
    *len = 0;
    char* out = (char*)malloc((inlen + 128) / 4 * 3 + 1);
    for (i = j = 0; i < inlen; i++) {
        if (in[i] == '=') break;
        c = base64de[(unsigned char)in[i]];
        if (c == 255) {
            *len = 0;
            free(out);
            return NULL;
        }
        switch (i & 0x3) {
            case 0:
                out[j] = (c << 2) & 0xFF;
                break;
            case 1:
                out[j++] |= (c >> 4) & 0x3;
                out[j] = (c & 0xF) << 4;
                break;
            case 2:
                out[j++] |= (c >> 2) & 0xF;
                out[j] = (c & 0x3) << 6;
                break;
            case 3:
                out[j++] |= c;
                break;
        }
    }
    *len = j;
    return out;
}

int TimeInfo_compare(TimeInfo* start, TimeInfo* end) {
    if (start->year != end->year)  return start->year - end->year;
    if (start->month != end->month)  return start->month - end->month;
    if (start->day != end->day)  return start->day - end->day;
    if (start->hour != end->hour)  return start->hour - end->hour;
    if (start->minute != end->minute)  return start->minute - end->minute;
    if (start->second != end->second)  return start->second - end->second;
    return 0;
}