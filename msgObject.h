#ifndef MSGOBJECT_H
#define MSGOBJECT_H
#define DEFAULT_MSG_STORAGE_PATH "msg.dat"
/*
    ��Ϣʵ��:
    �洢��Ϣ��ͨ������
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "util.h"
#include "liteiconv.h"
#include "user.h"
//����Ϣ����QQ��΢�������е����ݼ���
typedef enum XMsgObjectType {
    //textMsg��
    XMsgObjectType_TEXT,//�ı���Ϣ��һ�����ݣ�������ַ��
    XMsgObjectType_MARKDOWN,//Markdown��Ϣ�����ʻ����ı���������ʾʱ��MD���ƣ�
    //fileMsg��
    XMsgObjectType_FILE,//�ļ���Ϣ��һ���ļ���
    XMsgObjectType_IMAGE,//ͼƬ��Ϣ��һ��ͼƬ��
    XMsgObjectType_AUDIO,//������Ϣ��һ����Ƶ��
    XMsgObjectType_VIDEO,//��Ƶ��Ϣ��һ����Ƶ��
    //badMsg��
    XMsgObjectType_BAD,//����Ϣ(��Ϣ���ڡ�������ȶ����Թ�������)
    //��ԶҲ���ã����ڱ�����ֵ����
    XMsgObjectType_MAXENUMSIZE
} XMsgObjectType;
//Ϊ�˲�ȥ����GC�����ﾡ��ֱ��ʹ�ö�������
#define DEFAULT_MSGNAME "δ������Ϣ"
typedef struct XMsgObject {
    XMsgObjectType type;//����
    int ownerID;//������ID(������ID)
    TimeInfo time_info;//ʱ��
    char msgId[65];//��ϢID
    char msgName[65];//��Ϣ����(������ʾ������)
    union {//���������壬ֱ�ӳƺ��ڲ������struct���־��У����ǹ���һƬ�ڴ�
        struct {
            size_t len;//�ı�����a
            char* text;//�ı�����
        } textMsg;
        struct {
            size_t size;//�ļ���С
            char fileName[513];//�ļ���
            char filePath[4097];//�ļ�·��(�����߷��͵���������������洢��·���������������ؾͿ��Զ�ȡ����·�����ļ�������)
            unsigned char isCleared;//�Ƿ�����(֪ͨ��������Ƿ��ܱ�����)
        } fileMsg;
        struct {//����Ϣ
        } badMsg;
    };
} XMsgObject;
typedef struct XMsgObjectList {
    XMsgObject** msgList;//��Ϣ�б�
    size_t msgListSize;//��Ϣ�б��С
    size_t msgListCapacity;//��Ϣ�б�����
} XMsgObjectList;
typedef struct XMsgObjectsResult {
    XMsgObject** list;//��Ϣ�б�
    size_t size;//��Ϣ�б��С
} XMsgObjectsResult;
typedef struct SplitMergeHolder {
    XMsgObjectType type;
    int ownerID;//������ID(������ID)
    char msgName[65];//��Ϣ����(������ʾ������)
    char fileName[513];//�ļ���
    int splitCount;//�ָ�����
    int splitSize;//�ָ��С
    int fileSize;//�ļ���С
    FILE* tmpFile;//��ʱ�ļ����
} SplitMergeHolder;
extern XMsgObjectList* XMsgObjects;//��Ϣ�б�(ȫ�ֱ���)
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