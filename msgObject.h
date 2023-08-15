#ifndef MSGOBJECT_H
#define MSGOBJECT_H
#define DEFAULT_MSG_STORAGE_PATH "msg.dat"
/*
    消息实体:
    存储消息的通用容器
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "util.h"
#include "liteiconv.h"
#include "user.h"
//把消息看成QQ、微信聊天中的内容即可
typedef enum XMsgObjectType {
    //textMsg类
    XMsgObjectType_TEXT,//文本消息（一段内容，包括网址）
    XMsgObjectType_MARKDOWN,//Markdown消息（本质还是文本，但是显示时按MD绘制）
    //fileMsg类
    XMsgObjectType_FILE,//文件消息（一个文件）
    XMsgObjectType_IMAGE,//图片消息（一张图片）
    XMsgObjectType_AUDIO,//语音消息（一段音频）
    XMsgObjectType_VIDEO,//视频消息（一段视频）
    //badMsg类
    XMsgObjectType_BAD,//坏消息(消息过期、被清理等都可以归类这里)
    //永远也不用，用于表达最大值而已
    XMsgObjectType_MAXENUMSIZE
} XMsgObjectType;
//为了不去折腾GC，这里尽量直接使用定长数组
#define DEFAULT_MSGNAME "未命名消息"
typedef struct XMsgObject {
    XMsgObjectType type;//类型
    int ownerID;//所有者ID(发出者ID)
    TimeInfo time_info;//时间
    char msgId[65];//消息ID
    char msgName[65];//消息名称(用于显示的名字)
    union {//匿名联合体，直接称呼内部定义的struct名字就行，他们共用一片内存
        struct {
            size_t len;//文本长度a
            char* text;//文本内容
        } textMsg;
        struct {
            size_t size;//文件大小
            char fileName[513];//文件名
            char filePath[4097];//文件路径(发送者发送到服务器后服务器存储的路径，这样别人下载就可以读取这里路径找文件发送了)
            unsigned char isCleared;//是否被清理(通知这个东西是否还能被下载)
        } fileMsg;
        struct {//坏消息
        } badMsg;
    };
} XMsgObject;
typedef struct XMsgObjectList {
    XMsgObject** msgList;//消息列表
    size_t msgListSize;//消息列表大小
    size_t msgListCapacity;//消息列表容量
} XMsgObjectList;
typedef struct XMsgObjectsResult {
    XMsgObject** list;//消息列表
    size_t size;//消息列表大小
} XMsgObjectsResult;
typedef struct SplitMergeHolder {
    XMsgObjectType type;
    int ownerID;//所有者ID(发出者ID)
    char msgName[65];//消息名称(用于显示的名字)
    char fileName[513];//文件名
    int splitCount;//分割数量
    int splitSize;//分割大小
    int fileSize;//文件大小
    FILE* tmpFile;//临时文件句柄
} SplitMergeHolder;
extern XMsgObjectList* XMsgObjects;//消息列表(全局变量)
extern XMsgObject* XMsgObject_new();
extern void XMsgObject_addMsgId(XMsgObject* msgObject);
extern void XMsgObject_addTimeInfo(XMsgObject* msgObject);
extern void XMsgObject_free(XMsgObject* msgObject);
extern void XMsgObjects_freeItem(XMsgObject* msgObject);
extern XMsgObject* XMsgObject_newclass_TextMsg(XMsgObjectType type, int ownerID, const char* msgId, const char* msgName, TimeInfo timeI, const char* text, size_t len);
extern XMsgObject* XMsgObject_newclass_FileMsg(XMsgObjectType type, int ownerID, const char* msgId, const char* msgName, TimeInfo timeI, const char* fileName, const char* filePath, size_t size, unsigned char isCleared);
extern XMsgObject* XMsgObject_newclass_BadMsg(int ownerID, const char* msgId, const char* msgName, TimeInfo timeI);
extern XMsgObject* XMsgObject_newTextMsg(int ownerID, const char* msgId, const char* msgName, TimeInfo timeI, const char* text, size_t len);
extern XMsgObject* XMsgObject_newMarkdownMsg(int ownerID, const char* msgId, const char* msgName, TimeInfo timeI, char* text, size_t len);
extern XMsgObject* XMsgObject_newFileMsg(int ownerID, const char* msgId, const char* msgName, TimeInfo timeI, const char* fileName, const char* filePath, size_t size, unsigned char isCleared);
extern XMsgObject* XMsgObject_newImageMsg(int ownerID, const char* msgId, const char* msgName, TimeInfo timeI, const char* fileName, const char* filePath, size_t size, unsigned char isCleared);
extern XMsgObject* XMsgObject_newAudioMsg(int ownerID, const char* msgId, const char* msgName, TimeInfo timeI, const char* fileName, const char* filePath, size_t size, unsigned char isCleared);
extern XMsgObject* XMsgObject_newVideoMsg(int ownerID, const char* msgId, const char* msgName, TimeInfo timeI, const char* fileName, const char* filePath, size_t size, unsigned char isCleared);
extern XMsgObject* XMsgObject_newBadMsg(int ownerID, const char* msgId, const char* msgName, TimeInfo timeI);

extern int XMsgObjects_dump(const char* path);
extern int XMsgObjects_load(const char* path);

extern XMsgObject* XMsgObject_findMsgById(const char* msgId);
extern int XMsgObject_withdraw(XMsgObject* msg);
extern XMsgObjectsResult XMsgObject_selectMsgByTime(TimeInfo* start, TimeInfo* end);
extern XMsgObjectsResult XMsgObject_selectMsgByTypes(XMsgObjectType* types, int typesSize);
extern XMsgObjectsResult XMsgObject_selectMsgByType(XMsgObjectType type);
extern XMsgObjectsResult XMsgObject_selectMsgByOwnerID(int ownerID);
extern XMsgObjectsResult XMsgObject_selectMsgByOwner(const char* owner);
extern XMsgObjectsResult XMsgObject_selectMsgBySearch(const char* search);
extern XMsgObjectsResult XMsgObject_selectAllMsg();
extern XMsgObjectsResult XMsgObjectsResult_operator_intersection(XMsgObjectsResult r1, XMsgObjectsResult r2, int freeInput);
extern void XMsgObjectsResult_free(XMsgObjectsResult r);
extern void XMsgObjectsResult_add(XMsgObjectsResult* r, XMsgObject* msg);

extern char* XMsgObjectsResult_toJson(XMsgObjectsResult r);

#define DEFAULT_STORE_DIR "./files/"
extern void storeTempFile(XMsgObject* msgObject, FILE* tmp);
extern char* getStoreFile(XMsgObject* msgObject, int* len);
extern void storeFile(XMsgObject* msgObject, const char* content);
extern void cleanStoreFile(XMsgObject* msgObject);
#endif // MSGOBJECT_H