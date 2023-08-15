#include "msgObject.h"
XMsgObjectList* XMsgObjects = NULL;
#define XMsgObjectListCapacity 100

/*
    XMsgObjects_dump(DEFAULT_MSG_STORAGE_PATH);
    问题在于构建过程只是提供基础方法，后续一定有存取，所以没有类似User的全自动保存
*/
static void initXMsgObjectsIfNeed() {
    if (XMsgObjects == NULL) {
        XMsgObjects = (XMsgObjectList*)malloc(sizeof(XMsgObjectList));//这个伴随生命周期就不释放了
        XMsgObjects->msgList = (XMsgObject**)malloc(sizeof(XMsgObject*) * XMsgObjectListCapacity);
        XMsgObjects->msgListSize = 0;
        XMsgObjects->msgListCapacity = XMsgObjectListCapacity;
    }
}
XMsgObject* XMsgObject_new() {
    lockMutex(TypeMutex_API);//互斥锁-上锁
    initXMsgObjectsIfNeed();
    XMsgObjectList* XMsgObjectss = XMsgObjects;
    if (XMsgObjects->msgListSize >= XMsgObjects->msgListCapacity) {
        XMsgObjects->msgListCapacity += XMsgObjectListCapacity;
        XMsgObjects->msgList = (XMsgObject**)realloc(XMsgObjects->msgList,
         sizeof(XMsgObject*) * XMsgObjects->msgListCapacity);
    }
    XMsgObject* msgObject = (XMsgObject*)malloc(sizeof(XMsgObject));
    memset(msgObject, 0, sizeof(XMsgObject));
    msgObject->type = XMsgObjectType_BAD;
    msgObject->ownerID = -1;
    XMsgObjects->msgList[XMsgObjects->msgListSize++] = msgObject;
    XMsgObject_addMsgId(msgObject);
    XMsgObject_addTimeInfo(msgObject);
    unlockMutex(TypeMutex_API);//互斥锁-解锁
    return msgObject;
}

void XMsgObject_addMsgId(XMsgObject* msgObject) {
    if (msgObject == NULL)return;
    initXMsgObjectsIfNeed();
    XMsgObjects->msgListCapacity = XMsgObjectListCapacity;
    memset(msgObject->msgId, 0, 65);//64+1
retry:
    for (int i = 0; i < 64; i++) {
        int r = rand() % 62;
        if (r < 10) {
            msgObject->msgId[i] = '0' + r;
        } else if (r < 36) {
            msgObject->msgId[i] = 'a' + r - 10;
        } else {
            msgObject->msgId[i] = 'A' + r - 36;
        }
    }
    msgObject->msgId[64] = '\0';
    for (size_t i = 0; i < XMsgObjects->msgListSize; i++) {
        if (XMsgObjects->msgList[i] == msgObject)continue;
        if (strcmp(XMsgObjects->msgList[i]->msgId, msgObject->msgId) == 0) {
            goto retry;
        }
    }
}
void XMsgObject_addTimeInfo(XMsgObject* msgObject) {
    if (msgObject == NULL)return;
    initXMsgObjectsIfNeed();
    msgObject->time_info = getTimeInfo(getTimestamp());
}

void XMsgObject_free(XMsgObject* msgObject) {
    if (msgObject == NULL) return;
    initXMsgObjectsIfNeed();
    if (msgObject->type == XMsgObjectType_TEXT ||
        msgObject->type == XMsgObjectType_MARKDOWN) {
        if (msgObject->textMsg.text)free(msgObject->textMsg.text);
    }
    free(msgObject);
}

void XMsgObjects_freeItem(XMsgObject* msgObject) {
    if (msgObject == NULL) return;
    initXMsgObjectsIfNeed();
    lockMutex(TypeMutex_API);//互斥锁-上锁
    for (size_t i = 0; i < XMsgObjects->msgListSize; i++) {
        if (XMsgObjects->msgList[i] == msgObject) {
            for (size_t j = i; j < XMsgObjects->msgListSize - 1; j++) {
                XMsgObjects->msgList[j] = XMsgObjects->msgList[j + 1];
            }
            XMsgObjects->msgListSize--;
            XMsgObject_free(msgObject);
            break;
        }
    }
    unlockMutex(TypeMutex_API);//互斥锁-解锁
}

XMsgObject* XMsgObject_newclass_TextMsg(XMsgObjectType type, int ownerID, const char* msgId, const char* msgName, TimeInfo timeI, const char* text, size_t len) {
    XMsgObject* msgObject = XMsgObject_new();
    msgObject->time_info = timeI;
    if (msgId && strlen(msgId) <= 64)strcpy(msgObject->msgId, msgId);
    if (msgName && strlen(msgName) <= 64)strcpy(msgObject->msgName, msgName);
    msgObject->type = type;
    msgObject->ownerID = ownerID;
    msgObject->textMsg.len = len;
    msgObject->textMsg.text = (char*)malloc(sizeof(char) * (msgObject->textMsg.len + 1));
    memcpy(msgObject->textMsg.text, text, msgObject->textMsg.len);
    msgObject->textMsg.text[msgObject->textMsg.len] = '\0';
    return msgObject;
}
XMsgObject* XMsgObject_newclass_FileMsg(XMsgObjectType type, int ownerID, const char* msgId, const char* msgName, TimeInfo timeI, const char* fileName, const char* filePath, size_t size, unsigned char isCleared) {
    XMsgObject* msgObject = XMsgObject_new();
    msgObject->time_info = timeI;
    if (msgId && strlen(msgId) <= 64)strcpy(msgObject->msgId, msgId);
    if (msgName && strlen(msgName) <= 64)strcpy(msgObject->msgName, msgName);
    msgObject->type = type;
    msgObject->ownerID = ownerID;
    msgObject->fileMsg.size = size;
    if (fileName && strlen(fileName) <= 512) strcpy(msgObject->fileMsg.fileName, fileName);
    if (filePath && strlen(filePath) <= 4096) strcpy(msgObject->fileMsg.filePath, filePath);
    msgObject->fileMsg.isCleared = isCleared;
    return msgObject;
}

XMsgObject* XMsgObject_newclass_BadMsg(int ownerID, const char* msgId, const char* msgName, TimeInfo timeI) {
    XMsgObject* msgObject = XMsgObject_new();
    msgObject->time_info = timeI;
    if (msgId && strlen(msgId) <= 64)strcpy(msgObject->msgId, msgId);
    if (msgName && strlen(msgName) <= 64)strcpy(msgObject->msgName, msgName);
    msgObject->type = XMsgObjectType_BAD;
    msgObject->ownerID = ownerID;
    return msgObject;
}

XMsgObject* XMsgObject_newTextMsg(int ownerID, const char* msgId, const char* msgName, TimeInfo timeI, const char* text, size_t len) {
    XMsgObject* o = XMsgObject_newclass_TextMsg(XMsgObjectType_TEXT, ownerID, msgId, msgName, timeI, text, len);
}

XMsgObject* XMsgObject_newMarkdownMsg(int ownerID, const char* msgId, const char* msgName, TimeInfo timeI, char* text, size_t len) {
    return XMsgObject_newclass_TextMsg(XMsgObjectType_MARKDOWN, ownerID, msgId, msgName, timeI, text, len);
}

XMsgObject* XMsgObject_newFileMsg(int ownerID, const char* msgId, const char* msgName, TimeInfo timeI, const char* fileName, const char* filePath, size_t size, unsigned char isCleared) {
    return XMsgObject_newclass_FileMsg(XMsgObjectType_FILE, ownerID, msgId, msgName, timeI, fileName, filePath, size, isCleared);
}

XMsgObject* XMsgObject_newImageMsg(int ownerID, const char* msgId, const char* msgName, TimeInfo timeI, const char* fileName, const char* filePath, size_t size, unsigned char isCleared) {
    return XMsgObject_newclass_FileMsg(XMsgObjectType_IMAGE, ownerID, msgId, msgName, timeI, fileName, filePath, size, isCleared);
}

XMsgObject* XMsgObject_newAudioMsg(int ownerID, const char* msgId, const char* msgName, TimeInfo timeI, const char* fileName, const char* filePath, size_t size, unsigned char isCleared) {
    return XMsgObject_newclass_FileMsg(XMsgObjectType_AUDIO, ownerID, msgId, msgName, timeI, fileName, filePath, size, isCleared);
}

XMsgObject* XMsgObject_newVideoMsg(int ownerID, const char* msgId, const char* msgName, TimeInfo timeI, const char* fileName, const char* filePath, size_t size, unsigned char isCleared) {
    return XMsgObject_newclass_FileMsg(XMsgObjectType_VIDEO, ownerID, msgId, msgName, timeI, fileName, filePath, size, isCleared);
}

XMsgObject* XMsgObject_newBadMsg(int ownerID, const char* msgId, const char* msgName, TimeInfo timeI) {
    return XMsgObject_newclass_BadMsg(ownerID, msgId, msgName, timeI);
}

#define XMsgObjectsMagic "MSGs"
/*
    XMsgObjects保存结构定义
    char[4] MSGs
    size_t size
    [0]{
        unsigned char type
        int ownerID
        unsigned char msgIdLen
        char* msgId
        unsigned char msgNameLen
        char* msgName
        int year
        unsigned char month
        unsigned char day
        unsigned char hour
        unsigned char minute
        unsigned char second
        union({
            textMsg:{
                size_t len
                char* text
            }
            fileMsg:{
                size_t size
                size_t fileNameLen
                char* fileName
                size_t filePathLen
                char* filePath
                unsigned char isCleared
            }
            badMsg:{
            }
        })
    }
    [1]{...}
    [2]{...}
    ...
*/

int XMsgObjects_dump(const char* path) {
    FILE* fp = fopen(path, "wb");
    if (fp == NULL) return -1;
    lockMutex(TypeMutex_API);//互斥锁-上锁
    fwrite(XMsgObjectsMagic, 1, 4, fp);
    initXMsgObjectsIfNeed();
    fwrite(&XMsgObjects->msgListSize, 1, sizeof(size_t), fp);
    unsigned char ucharType;
    int intType;
    size_t sizetType;
    for (int i = 0; i < XMsgObjects->msgListSize; i++) {
        ucharType = XMsgObjects->msgList[i]->type;
        fwrite(&ucharType, 1, sizeof(unsigned char), fp);
        intType = XMsgObjects->msgList[i]->ownerID;
        fwrite(&intType, 1, sizeof(int), fp);
        ucharType = strlen(XMsgObjects->msgList[i]->msgId);
        fwrite(&ucharType, 1, sizeof(unsigned char), fp);
        fwrite(XMsgObjects->msgList[i]->msgId, 1, ucharType, fp);
        ucharType = strlen(XMsgObjects->msgList[i]->msgName);
        fwrite(&ucharType, 1, sizeof(unsigned char), fp);
        fwrite(XMsgObjects->msgList[i]->msgName, 1, ucharType, fp);
        intType = XMsgObjects->msgList[i]->time_info.year;
        fwrite(&intType, 1, sizeof(int), fp);
        ucharType = XMsgObjects->msgList[i]->time_info.month;
        fwrite(&ucharType, 1, sizeof(unsigned char), fp);
        ucharType = XMsgObjects->msgList[i]->time_info.day;
        fwrite(&ucharType, 1, sizeof(unsigned char), fp);
        ucharType = XMsgObjects->msgList[i]->time_info.hour;
        fwrite(&ucharType, 1, sizeof(unsigned char), fp);
        ucharType = XMsgObjects->msgList[i]->time_info.minute;
        fwrite(&ucharType, 1, sizeof(unsigned char), fp);
        ucharType = XMsgObjects->msgList[i]->time_info.second;
        fwrite(&ucharType, 1, sizeof(unsigned char), fp);
        switch (XMsgObjects->msgList[i]->type) {
            case XMsgObjectType_TEXT:
            case XMsgObjectType_MARKDOWN:
                {
                    sizetType = XMsgObjects->msgList[i]->textMsg.len;
                    fwrite(&sizetType, 1, sizeof(size_t), fp);
                    fwrite(XMsgObjects->msgList[i]->textMsg.text, 1, XMsgObjects->msgList[i]->textMsg.len, fp);
                    break;
                }
            case XMsgObjectType_FILE:
            case XMsgObjectType_IMAGE:
            case XMsgObjectType_AUDIO:
            case XMsgObjectType_VIDEO:
                {
                    sizetType = XMsgObjects->msgList[i]->fileMsg.size;
                    fwrite(&sizetType, 1, sizeof(size_t), fp);
                    sizetType = strlen(XMsgObjects->msgList[i]->fileMsg.fileName);
                    fwrite(&sizetType, 1, sizeof(size_t), fp);
                    fwrite(XMsgObjects->msgList[i]->fileMsg.fileName, 1, sizetType, fp);
                    sizetType = strlen(XMsgObjects->msgList[i]->fileMsg.filePath);
                    fwrite(&sizetType, 1, sizeof(size_t), fp);
                    fwrite(XMsgObjects->msgList[i]->fileMsg.filePath, 1, sizetType, fp);
                    ucharType = XMsgObjects->msgList[i]->fileMsg.isCleared;
                    fwrite(&ucharType, 1, sizeof(unsigned char), fp);
                    break;
                }
            default:
                {
                    break;
                }
        }
    }
    unlockMutex(TypeMutex_API);//互斥锁-解锁
    fclose(fp);
    return 0;
}

int XMsgObjects_load(const char* path) {
    FILE* fp = fopen(path, "rb");
    if (fp == NULL) return -1;
    char magic[4];
    fread(magic, 1, 4, fp);
    if (strncmp(magic, XMsgObjectsMagic, 4) != 0) {
        fclose(fp);
        return -1;
    }
    if (XMsgObjects == NULL) {
        XMsgObjects = (XMsgObjectList*)malloc(sizeof(XMsgObjectList));//这个伴随生命周期就不释放了
        XMsgObjects->msgList = NULL;
        XMsgObjects->msgListSize = 0;
        XMsgObjects->msgListCapacity = 0;
    } else {
        if (XMsgObjects->msgList) {
            for (int i = 0; i < XMsgObjects->msgListSize; i++) {
                XMsgObject_free(XMsgObjects->msgList[i]);
            }
            free(XMsgObjects->msgList);
        }
        XMsgObjects->msgList = NULL;
        XMsgObjects->msgListSize = 0;
        XMsgObjects->msgListCapacity = 0;

    }
    size_t msgListSize = 0;
    fread(&msgListSize, 1, sizeof(size_t), fp);
    unsigned char ucharType;
    int intType;
    size_t sizetType;
    XMsgObjectType otype;
    int ownerID;
    int time_year, time_month, time_day, time_hour, time_minute, time_second;
    for (int i = 0; i < msgListSize; i++) {
        fread(&ucharType, 1, sizeof(unsigned char), fp);
        otype = ucharType;
        fread(&intType, 1, sizeof(int), fp);
        ownerID = intType;
        fread(&ucharType, 1, sizeof(unsigned char), fp);
        if (ucharType > 64) return -1;//溢出
        char msgId[ucharType + 1];
        fread(msgId, 1, ucharType, fp);
        msgId[ucharType] = '\0';
        fread(&ucharType, 1, sizeof(unsigned char), fp);
        if (ucharType > 64) return -1;//溢出
        char msgName[ucharType + 1];
        fread(msgName, 1, ucharType, fp);
        msgName[ucharType] = '\0';
        fread(&intType, 1, sizeof(int), fp);
        time_year = intType;
        fread(&ucharType, 1, sizeof(unsigned char), fp);
        time_month = ucharType;
        fread(&ucharType, 1, sizeof(unsigned char), fp);
        time_day = ucharType;
        fread(&ucharType, 1, sizeof(unsigned char), fp);
        time_hour = ucharType;
        fread(&ucharType, 1, sizeof(unsigned char), fp);
        time_minute = ucharType;
        fread(&ucharType, 1, sizeof(unsigned char), fp);
        time_second = ucharType;
        TimeInfo timeInfo = { time_year, time_month, time_day, time_hour, time_minute, time_second };
        switch (otype) {
            case XMsgObjectType_TEXT:
            case XMsgObjectType_MARKDOWN:
                {
                    fread(&sizetType, 1, sizeof(size_t), fp);
                    char text[sizetType + 1];
                    fread(text, 1, sizetType, fp);
                    text[sizetType] = '\0';
                    if (otype == XMsgObjectType_TEXT)
                        XMsgObject_newTextMsg(ownerID, msgId, msgName, timeInfo, text, sizetType);
                    else
                        XMsgObject_newMarkdownMsg(ownerID, msgId, msgName, timeInfo, text, sizetType);
                    break;
                }
            case XMsgObjectType_FILE:
            case XMsgObjectType_IMAGE:
            case XMsgObjectType_AUDIO:
            case XMsgObjectType_VIDEO:
                {
                    size_t fsize;
                    fread(&fsize, 1, sizeof(size_t), fp);
                    fread(&sizetType, 1, sizeof(size_t), fp);
                    if (sizetType > 512) return -1;//溢出
                    char fileName[sizetType + 1];
                    fread(fileName, 1, sizetType, fp);
                    fileName[sizetType] = '\0';
                    fread(&sizetType, 1, sizeof(size_t), fp);
                    if (sizetType > 4096) return -1;//溢出
                    char filePath[sizetType + 1];
                    fread(filePath, 1, sizetType, fp);
                    filePath[sizetType] = '\0';
                    fread(&ucharType, 1, sizeof(unsigned char), fp);
                    if (otype == XMsgObjectType_FILE)
                        XMsgObject_newFileMsg(ownerID, msgId, msgName, timeInfo, fileName, filePath, fsize, ucharType);
                    else if (otype == XMsgObjectType_IMAGE)
                        XMsgObject_newImageMsg(ownerID, msgId, msgName, timeInfo, fileName, filePath, fsize, ucharType);
                    else if (otype == XMsgObjectType_AUDIO)
                        XMsgObject_newAudioMsg(ownerID, msgId, msgName, timeInfo, fileName, filePath, fsize, ucharType);
                    else
                        XMsgObject_newVideoMsg(ownerID, msgId, msgName, timeInfo, fileName, filePath, fsize, ucharType);
                    break;
                }
            case XMsgObjectType_BAD:
            default:
                {
                    XMsgObject_newBadMsg(ownerID, msgId, msgName, timeInfo);
                    break;
                }
        }
    }
}

XMsgObject* XMsgObject_findMsgById(const char* msgId) {
    initXMsgObjectsIfNeed();
    for (int i = 0; i < XMsgObjects->msgListSize; i++) {
        if (strcmp(XMsgObjects->msgList[i]->msgId, msgId) == 0) {
            return XMsgObjects->msgList[i];
        }
    }
    return NULL;
}
static void inline __witchdrawTextMsg__(XMsgObject* msg) {
    if (msg == NULL) return;
    if (msg->type == XMsgObjectType_TEXT || msg->type == XMsgObjectType_MARKDOWN) {
        if (msg->textMsg.text != NULL)
            free(msg->textMsg.text);
        msg->textMsg.text = NULL;
        msg->textMsg.len = 0;
        msg->type = XMsgObjectType_BAD;
    }
}
int XMsgObject_withdraw(XMsgObject* msg) {
    if (msg == NULL) return -1;
    lockMutex(TypeMutex_API);//互斥锁-上锁
    switch (msg->type) {
        case XMsgObjectType_TEXT:
        case XMsgObjectType_MARKDOWN:
            {
                __witchdrawTextMsg__(msg);
                break;
            }
        case XMsgObjectType_FILE:
        case XMsgObjectType_IMAGE:
        case XMsgObjectType_AUDIO:
        case XMsgObjectType_VIDEO:
            {
                cleanStoreFile(msg);
                break;
            }
        default:
            break;
    }
    unlockMutex(TypeMutex_API);//互斥锁-解锁
    return 0;
}
XMsgObjectsResult XMsgObject_selectMsgByTime(TimeInfo* start, TimeInfo* end) {
    //约定NULL为不限制
    if (start == NULL && end == NULL) {
        return XMsgObject_selectAllMsg();
    }
    initXMsgObjectsIfNeed();
    XMsgObjectsResult msgObjects = { NULL, 0 };
    msgObjects.list = (XMsgObject**)malloc(sizeof(XMsgObject*) * XMsgObjects->msgListSize);
    msgObjects.size = 0;
    for (int i = 0; i < XMsgObjects->msgListSize; i++) {
        if (start != NULL && end != NULL) {
            if (TimeInfo_compare(start, &XMsgObjects->msgList[i]->time_info) <= 0 &&
                TimeInfo_compare(end, &XMsgObjects->msgList[i]->time_info) >= 0) {
                msgObjects.list[msgObjects.size++] = XMsgObjects->msgList[i];
            }
        } else if (start != NULL) {
            if (TimeInfo_compare(start, &XMsgObjects->msgList[i]->time_info) <= 0) {
                msgObjects.list[msgObjects.size++] = XMsgObjects->msgList[i];
            }
        } else {
            if (TimeInfo_compare(end, &XMsgObjects->msgList[i]->time_info) >= 0) {
                msgObjects.list[msgObjects.size++] = XMsgObjects->msgList[i];
            }
        }
    }
    msgObjects.list = (XMsgObject**)realloc(msgObjects.list, sizeof(XMsgObject*) * msgObjects.size);
    if (msgObjects.size == 0) {
        if (msgObjects.list)free(msgObjects.list);
        msgObjects.list = NULL;
    }
    return msgObjects;
}

XMsgObjectsResult XMsgObject_selectMsgByTypes(XMsgObjectType* types, int typesSize) {
    //如果没有约定不限制
    if (typesSize == 0) return XMsgObject_selectAllMsg();
    //最少选择一种类型，允许多种类型
    XMsgObjectsResult msgObjects = { NULL, 0 };
    if (types == NULL) {
        return msgObjects;
    }
    initXMsgObjectsIfNeed();
    msgObjects.list = (XMsgObject**)malloc(sizeof(XMsgObject*) * XMsgObjects->msgListSize);
    msgObjects.size = 0;
    for (int i = 0; i < XMsgObjects->msgListSize; i++) {
        for (int j = 0; j < typesSize; j++) {
            if (XMsgObjects->msgList[i]->type == types[j]) {
                msgObjects.list[msgObjects.size++] = XMsgObjects->msgList[i];
                break;
            }
        }
    }
    msgObjects.list = (XMsgObject**)realloc(msgObjects.list, sizeof(XMsgObject*) * msgObjects.size);
    if (msgObjects.size == 0) {
        if (msgObjects.list)free(msgObjects.list);
        msgObjects.list = NULL;
    }
    return msgObjects;
}

XMsgObjectsResult XMsgObject_selectMsgByType(XMsgObjectType type) {
    return XMsgObject_selectMsgByTypes(&type, 1);
}

XMsgObjectsResult XMsgObject_selectMsgByOwner(const char* owner) {
    if (owner == NULL)return XMsgObject_selectAllMsg();//不限制
    User* user = User_getUser(owner);
    if (user == NULL) {
        XMsgObjectsResult msgObjects = { NULL, 0 };
        return msgObjects;
    }
    return XMsgObject_selectMsgByOwnerID(user->ownerID);
}

XMsgObjectsResult XMsgObject_selectMsgByOwnerID(int ownerID) {
    initXMsgObjectsIfNeed();
    XMsgObjectsResult msgObjects = { NULL, 0 };
    msgObjects.list = (XMsgObject**)malloc(sizeof(XMsgObject*) * XMsgObjects->msgListSize);
    msgObjects.size = 0;
    for (int i = 0; i < XMsgObjects->msgListSize; i++) {
        if (XMsgObjects->msgList[i]->ownerID == ownerID) {
            msgObjects.list[msgObjects.size++] = XMsgObjects->msgList[i];
        }
    }
    msgObjects.list = (XMsgObject**)realloc(msgObjects.list, sizeof(XMsgObject*) * msgObjects.size);
    if (msgObjects.size == 0) {
        if (msgObjects.list)free(msgObjects.list);
        msgObjects.list = NULL;
    }
    return msgObjects;
}

XMsgObjectsResult XMsgObject_selectMsgBySearch(const char* search) {
    if (search == NULL)return XMsgObject_selectAllMsg();//不限制
    initXMsgObjectsIfNeed();
    XMsgObjectsResult msgObjects = { NULL, 0 };
    msgObjects.list = (XMsgObject**)malloc(sizeof(XMsgObject*) * XMsgObjects->msgListSize);
    msgObjects.size = 0;
    //search也可能是msgId唯一指定
    XMsgObject* msg = XMsgObject_findMsgById(search);
    if (msg != NULL) {
        msgObjects.list[msgObjects.size++] = msg;
    }
    for (int i = 0; i < XMsgObjects->msgListSize; i++) {
        XMsgObject* msg2 = XMsgObjects->msgList[i];
        if (msg2 == msg)continue;
        if (strstr(msg2->msgName, search) != NULL) {
            msgObjects.list[msgObjects.size++] = msg2;
        }
    }
    msgObjects.list = (XMsgObject**)realloc(msgObjects.list, sizeof(XMsgObject*) * msgObjects.size);
    if (msgObjects.size == 0) {
        if (msgObjects.list)free(msgObjects.list);
        msgObjects.list = NULL;
    }
    return msgObjects;
}

XMsgObjectsResult XMsgObject_selectAllMsg() {
    XMsgObjectsResult msgObjects = { NULL, 0 };
    initXMsgObjectsIfNeed();
    msgObjects.list = (XMsgObject**)malloc(sizeof(XMsgObject*) * XMsgObjects->msgListSize);
    msgObjects.size = 0;
    for (int i = 0; i < XMsgObjects->msgListSize; i++) {
        msgObjects.list[msgObjects.size++] = XMsgObjects->msgList[i];
    }
    msgObjects.list = (XMsgObject**)realloc(msgObjects.list, sizeof(XMsgObject*) * msgObjects.size);
    if (msgObjects.size == 0) {
        if (msgObjects.list)free(msgObjects.list);
        msgObjects.list = NULL;
    }
    return msgObjects;
}
/*
    多次条件筛选，求交集
*/
XMsgObjectsResult XMsgObjectsResult_operator_intersection(XMsgObjectsResult r1, XMsgObjectsResult r2, int freeInput) {
    XMsgObjectsResult msgObjects = { NULL, 0 };
    initXMsgObjectsIfNeed();
    msgObjects.list = (XMsgObject**)malloc(sizeof(XMsgObject*) * XMsgObjects->msgListSize);
    msgObjects.size = 0;
    for (int i = 0; i < r1.size; i++) {
        for (int j = 0; j < r2.size; j++) {
            if (r1.list[i] == r2.list[j]) {//统一管理，指针唯一
                msgObjects.list[msgObjects.size++] = r1.list[i];
                break;
            }
        }
    }
    msgObjects.list = (XMsgObject**)realloc(msgObjects.list, sizeof(XMsgObject*) * msgObjects.size);
    if (msgObjects.size == 0) {
        if (msgObjects.list)free(msgObjects.list);
        msgObjects.list = NULL;
    }
    if (freeInput) {
        XMsgObjectsResult_free(r1);
        XMsgObjectsResult_free(r2);
    }
    return msgObjects;
}
void XMsgObjectsResult_add(XMsgObjectsResult* r, XMsgObject* msg) {
    //警告，这个add不去重
    if (r == NULL || msg == NULL)return;
    if (r->list == NULL) {
        r->list = (XMsgObject**)malloc(sizeof(XMsgObject*) * 1);
        r->size = 0;
    } else {
        r->list = (XMsgObject**)realloc(r->list, sizeof(XMsgObject*) * (r->size + 1));
    }
    r->list[r->size++] = msg;
}

/*
    传输格式：
    [
        {
            "type": 0,
            "ownerID": 0,
            "msgId": "xxx",
            "msgName": "xxx",
            "time": [0, 0, 0, 0, 0, 0],
            union({
                "textMsg": "text",
                "fileMsg": {
                    "name": "name",
                    //无意义，Web无法访问的，忽略："path": "path",
                    "size": 0,
                    "isCleared": true/false
                }
            })
        },
        ...
    ]
    统一UTF-8编码
    统一不可打印字符用\uXXXX表示
*/
char* XMsgObjectsResult_toJson(XMsgObjectsResult r) {
    int isGBKsys = isGBKSystem();
    sBuffer sb;
    sBuffer_init(&sb, r.size * 128);
    sBuffer_appendl(&sb, "[\n", 2);
    for (int i = 0; i < r.size; i++) {
        sBuffer_addchar(&sb, '{');
        sBuffer_fmt(&sb, "\n\"type\":%d,", r.list[i]->type);
        sBuffer_fmt(&sb, "\n\"ownerID\":%d,", r.list[i]->ownerID);
        sBuffer_fmt(&sb, "\n\"msgId\":\"%s\",", r.list[i]->msgId);
        {
            sBuffer_append(&sb, "\n\"msgName\":\"");
            char* text = r.list[i]->msgName;
            int len = strlen(text);
            if (isGBKsys && !isUTF8(text, len)) {
                text = gbk_to_utf8(text, &len);
            }
            sBuffer_utf8jsonstr(&sb, text, len);
            if (isGBKsys && text != r.list[i]->msgName)
                free(text);
            sBuffer_appendl(&sb, "\",", 2);
        }
        sBuffer_fmt(&sb, "\n\"time\":[%d,%d,%d,%d,%d,%d]",
            r.list[i]->time_info.year, r.list[i]->time_info.month, r.list[i]->time_info.day,
            r.list[i]->time_info.hour, r.list[i]->time_info.minute, r.list[i]->time_info.second);
        switch (r.list[i]->type) {
            case XMsgObjectType_TEXT:
            case XMsgObjectType_MARKDOWN:
                {
                    sBuffer_append(&sb, ",\n\"textMsg\":\"");
                    char* text = r.list[i]->textMsg.text;
                    int len = r.list[i]->textMsg.len;
                    if (isGBKsys && !isUTF8(text, len)) {
                        text = gbk_to_utf8(text, &len);
                    }
                    sBuffer_utf8jsonstr(&sb, text, len);
                    sBuffer_appendl(&sb, "\"\n", 2);
                    if (text != r.list[i]->textMsg.text && text) free(text);
                    break;
                }
            case XMsgObjectType_FILE:
            case XMsgObjectType_IMAGE:
            case XMsgObjectType_AUDIO:
            case XMsgObjectType_VIDEO:
                {
                    sBuffer_append(&sb, ",\n\"fileMsg\":{\n"
                                        "\"name\":\"");
                    char* name = r.list[i]->fileMsg.fileName;
                    int namelen = strlen(name);
                    if (isGBKsys && !isUTF8(name, namelen)) {
                        name = gbk_to_utf8(name, &namelen);
                    }
                    sBuffer_utf8jsonstr(&sb, name, namelen);
                    sBuffer_fmt(&sb, "\",\n\"size\":%d,\n\"isCleared\":%s\n}\n",
                        (int)r.list[i]->fileMsg.size, r.list[i]->fileMsg.isCleared ? "true" : "false");
                    if (name != r.list[i]->fileMsg.fileName && name) free(name);
                    break;
                    // sBuffer_append(&sb, ",\n\"fileMsg\":{\n"
                    //                     "\"name\":\"");
                    // char* name = r.list[i]->fileMsg.fileName;
                    // int namelen = strlen(name);
                    // if (isGBKsys && !isUTF8(name, namelen)) {
                    //     name = gbk_to_utf8(name, &namelen);
                    // }
                    // sBuffer_utf8jsonstr(&sb, name, namelen);
                    // sBuffer_append(&sb, "\",\n\"path\":\"");
                    // char* path = r.list[i]->fileMsg.filePath;
                    // int pathlen = strlen(path);
                    // if (isGBKsys && !isUTF8(path, pathlen)) {
                    //     path = gbk_to_utf8(path, &pathlen);
                    // }
                    // sBuffer_utf8jsonstr(&sb, path, pathlen);
                    // sBuffer_fmt(&sb, "\",\n\"size\":%d,\n\"isCleared\":%s\n}\n",
                    //     (int)r.list[i]->fileMsg.size, r.list[i]->fileMsg.isCleared ? "true" : "false");
                    // if (name != r.list[i]->fileMsg.fileName && name) free(name);
                    // if (path != r.list[i]->fileMsg.filePath && path) free(path);
                    // break;
                }
            case XMsgObjectType_BAD:
            default:
                {
                    sBuffer_addchar(&sb, '\n');
                    break;
                }
        }
        sBuffer_addchar(&sb, '}');
        if (i != r.size - 1) {
            sBuffer_appendl(&sb, ",\n", 2);
        }
    }
    sBuffer_addchar(&sb, ']');
    char* json = (char*)malloc(sb.size + 1);
    memcpy(json, sb.buff, sb.size);
    json[sb.size] = '\0';
    sBuffer_free(&sb);
    return json;
}

void XMsgObjectsResult_free(XMsgObjectsResult r) {
    if (r.list)
        free(r.list);
}

static inline void __filePathGen__(XMsgObject* msgObject) {
    if (msgObject == NULL) return;
    if (strstr(msgObject->fileMsg.fileName, ".") != NULL) {
        char ext[65] = { 0 };
        sscanf(msgObject->fileMsg.fileName, "%*[^.].%s", ext);
        //ext检查是不是a-zA-Z0-9.-
        int ok = 1;
        for (int i = 0; i < strlen(ext); ++i) {
            if (!((ext[i] >= 'a' && ext[i] <= 'z') ||
                (ext[i] >= 'A' && ext[i] <= 'Z') ||
                (ext[i] >= '0' && ext[i] <= '9') ||
                ext[i] == '.' || ext[i] == '-')) {
                goto normal_sprintf_HD;
            }
        }
        sprintf(msgObject->fileMsg.filePath, "%s/%s.%s", DEFAULT_STORE_DIR, msgObject->msgId, ext);
    } else {
    normal_sprintf_HD:
        sprintf(msgObject->fileMsg.filePath, "%s/%s", DEFAULT_STORE_DIR, msgObject->msgId);
    }
}
void storeTempFile(XMsgObject* msgObject, FILE* tmp) {
    if (msgObject == NULL || tmp == NULL) return;
    createDir(DEFAULT_STORE_DIR);
    switch (msgObject->type) {
        case XMsgObjectType_FILE:
        case XMsgObjectType_IMAGE:
        case XMsgObjectType_AUDIO:
        case XMsgObjectType_VIDEO:
            {
                size_t len = msgObject->fileMsg.size;
                __filePathGen__(msgObject);
                FILE* fp = fopen(msgObject->fileMsg.filePath, "wb");
                if (fp) {
                    fseek(tmp, 0, SEEK_SET);
                    char buff[8192];
                    size_t readlen = 0;
                    while ((readlen = fread(buff, 1, 8192, tmp)) > 0) {
                        fwrite(buff, 1, readlen, fp);
                    }
                    fclose(fp);
                    msgObject->fileMsg.isCleared = 0;
                } else {
                    msgObject->fileMsg.isCleared = 1;
                }
                fclose(tmp);
                break;
            }
        default:
            return;
    }
}
void storeFile(XMsgObject* msgObject, const char* content) {
    if (msgObject == NULL || content == NULL) return;
    createDir(DEFAULT_STORE_DIR);
    switch (msgObject->type) {
        case XMsgObjectType_FILE:
        case XMsgObjectType_IMAGE:
        case XMsgObjectType_AUDIO:
        case XMsgObjectType_VIDEO:
            {
                size_t len = msgObject->fileMsg.size;
                __filePathGen__(msgObject);
                FILE* fp = fopen(msgObject->fileMsg.filePath, "wb");
                if (fp) {
                    fwrite(content, 1, msgObject->fileMsg.size, fp);
                    fclose(fp);
                    msgObject->fileMsg.isCleared = 0;
                } else {
                    msgObject->fileMsg.isCleared = 1;
                }
                break;
            }
        default:
            return;
    }
}

char* getStoreFile(XMsgObject* msgObject, int* len) {
    if (msgObject == NULL) return NULL;
    switch (msgObject->type) {
        case XMsgObjectType_FILE:
        case XMsgObjectType_IMAGE:
        case XMsgObjectType_AUDIO:
        case XMsgObjectType_VIDEO:
            {
                if (msgObject->fileMsg.isCleared) {
                    if (len)
                        *len = 0;
                    char* content = (char*)malloc(1);
                    content[0] = '\0';
                    return content;
                }
                FILE* fp = fopen(msgObject->fileMsg.filePath, "rb");
                if (fp) {
                    fseek(fp, 0, SEEK_END);
                    size_t size = ftell(fp);
                    fseek(fp, 0, SEEK_SET);
                    char* content = (char*)malloc(size + 1);
                    fread(content, 1, size, fp);
                    content[size] = '\0';
                    fclose(fp);
                    if (len)
                        *len = size;
                    return content;
                } else {
                    msgObject->fileMsg.isCleared = 1;
                    if (len)
                        *len = 0;
                    char* content = (char*)malloc(1);
                    content[0] = '\0';
                    return content;
                }
                break;
            }
        default:
            return NULL;
    }
}

void cleanStoreFile(XMsgObject* msgObject) {
    if (msgObject == NULL) return;
    switch (msgObject->type) {
        case XMsgObjectType_FILE:
        case XMsgObjectType_IMAGE:
        case XMsgObjectType_AUDIO:
        case XMsgObjectType_VIDEO:
            {
                if (msgObject->fileMsg.isCleared) return;
                removeFile(msgObject->fileMsg.filePath);
                memset(msgObject->fileMsg.filePath, 0, sizeof(msgObject->fileMsg.filePath));
                msgObject->fileMsg.isCleared = 1;
                msgObject->type = XMsgObjectType_BAD;
                break;
            }
        default:
            break;
    }
}