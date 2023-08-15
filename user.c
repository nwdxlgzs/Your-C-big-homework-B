#include "user.h"
/*
    用户使用过程
    1.通过AreYouAlive接口由客户端判断程序死活，选择到可以通讯的服务器程序
    2.打开对应WebUI，输入账号密码，点击登录（或者注册），全程token生命周期在URL参数中存储（方便一个人一个浏览器多账号测试）
    3.

*/
UserList* userList = NULL;
static char* generateToken() {
    srand(time(NULL));
    if (userList == NULL) {
        userList = (UserList*)malloc(sizeof(UserList));
        userList->userList = (User**)malloc(sizeof(User*));
        userList->userListSize = 0;
    }
    char* token = (char*)malloc(65);
    memset(token, 0, 65);
retry:
    for (int i = 0; i < 64; i++) {
        int r = rand() % 62;
        if (r < 10) {
            token[i] = '0' + r;
        } else if (r < 36) {
            token[i] = 'a' + r - 10;
        } else {
            token[i] = 'A' + r - 36;
        }
    }
    for (int i = 0; i < userList->userListSize; i++) {
        if (strcmp(userList->userList[i]->token, token) == 0) {//重复了
            goto retry;
        }
    }
    return token;
}
static int generateOwnerID() {
    if (userList == NULL) {
        userList = (UserList*)malloc(sizeof(UserList));
        userList->userList = (User**)malloc(sizeof(User*));
        userList->userListSize = 0;
    }
    int ownerID = 0;
retry:
    ownerID = rand() % 100000000;
    if (ownerID < 0) goto retry;
    for (int i = 0; i < userList->userListSize; i++) {
        if (userList->userList[i]->ownerID == ownerID) {//重复了
            goto retry;
        }
    }
    return ownerID;
}

User* User_create(const char* account, const char* password) {
    if (userList == NULL) {
        userList = (UserList*)malloc(sizeof(UserList));
        userList->userList = (User**)malloc(sizeof(User*));
        userList->userListSize = 0;
    }
    if (strlen(account) > 1024 || strlen(password) > 1024) return NULL;//超长内容
    //遍历先看一下账号是否存在
    for (int i = 0; i < userList->userListSize; i++) {
        if (strcmp(userList->userList[i]->account, account) == 0) {
            return NULL;
        }
    }
    User* user = (User*)malloc(sizeof(User));
    memset(user, 0, sizeof(User));
    strcpy(user->account, account);
    strcpy(user->password, password);
    user->ownerID = generateOwnerID();
    userList->userListSize++;
    userList->userList = (User**)realloc(userList->userList, sizeof(User*) * userList->userListSize);
    userList->userList[userList->userListSize - 1] = user;
    UserList_dump(DEFAULT_USER_STORAGE_PATH);
    return user;
}

User* User_getUser(const char* account) {
    if (userList == NULL) {
        userList = (UserList*)malloc(sizeof(UserList));
        userList->userList = (User**)malloc(sizeof(User*));
        userList->userListSize = 0;
    }
    if (strlen(account) > 1024) return NULL;//超长内容
    for (int i = 0; i < userList->userListSize; i++) {
        if (strcmp(userList->userList[i]->account, account) == 0) {
            return userList->userList[i];
        }
    }
    return NULL;
}

User* User_getUserByToken(const char* token) {
    if (userList == NULL) {
        userList = (UserList*)malloc(sizeof(UserList));
        userList->userList = (User**)malloc(sizeof(User*));
        userList->userListSize = 0;
    }
    if (strlen(token) > 64) return NULL;//超长内容
    for (int i = 0; i < userList->userListSize; i++) {
        if (strcmp(userList->userList[i]->token, token) == 0) {
            return userList->userList[i];
        }
    }
    return NULL;
}

int User_login(User* user, const char* password) {
    if (user == NULL) return 0;
    if (strcmp(user->password, password) == 0) {
        char* token = generateToken();
        strcpy(user->token, token);
        free(token);
        UserList_dump(DEFAULT_USER_STORAGE_PATH);
        return 1;
    }
    return 0;
}

int User_logout(User* user) {
    if (user == NULL) return 0;
    memset(user->token, 0, 65);
    UserList_dump(DEFAULT_USER_STORAGE_PATH);
    return 1;
}

int User_isLogin(User* user) {
    if (user == NULL) return 0;
    if (strlen(user->token) > 0) {
        return 1;
    }
    return 0;
}


#define userListMagic "Users"
/*
    userList保存结构定义
    char[4] MSGs
    size_t size
    [0]{
        unsigned short accountLen
        char[accountLen] account
        unsigned short passwordLen
        char[passwordLen] password
        unsigned char tokenLen
        if(tokenLen > 0)
            char[tokenLen] token
        int ownerID
    }
    [1]{...}
    [2]{...}
    ...
 */
int UserList_dump(const char* path) {
    FILE* fp = fopen(path, "wb");
    if (fp == NULL || userList == NULL) return -1;
    fwrite(userListMagic, 1, 4, fp);
    fwrite(&userList->userListSize, 1, sizeof(size_t), fp);
    unsigned short u_short;
    unsigned char u_char;
    for (int i = 0; i < userList->userListSize; i++) {
        u_short = strlen(userList->userList[i]->account);
        fwrite(&u_short, 1, sizeof(unsigned short), fp);
        fwrite(userList->userList[i]->account, 1, u_short, fp);
        u_short = strlen(userList->userList[i]->password);
        fwrite(&u_short, 1, sizeof(unsigned short), fp);
        fwrite(userList->userList[i]->password, 1, u_short, fp);
        u_char = strlen(userList->userList[i]->token);
        fwrite(&u_char, 1, sizeof(unsigned char), fp);
        if (u_char > 0) {
            fwrite(userList->userList[i]->token, 1, u_char, fp);
        }
        fwrite(&userList->userList[i]->ownerID, 1, sizeof(int), fp);
    }
    fclose(fp);
    return 0;
}

int UserList_load(const char* path) {
    FILE* fp = fopen(path, "rb");
    if (fp == NULL) return -1;
    char magic[4];
    fread(magic, 1, 4, fp);
    if (strncmp(magic, userListMagic, 4) != 0) {
        fclose(fp);
        return -1;
    }
    if (userList == NULL) {
        userList = (UserList*)malloc(sizeof(UserList));
        userList->userList = NULL;
        userList->userListSize = 0;
    } else {
        if (userList->userList) {
            for (int i = 0; i < userList->userListSize; i++) {
                free(userList->userList[i]);
            }
            free(userList->userList);
        }
        userList->userList = NULL;
        userList->userListSize = 0;
    }
    fread(&userList->userListSize, 1, sizeof(size_t), fp);
    userList->userList = (User**)malloc(sizeof(User*) * userList->userListSize);
    unsigned short u_short;
    unsigned char u_char;
    for (int i = 0; i < userList->userListSize; i++) {
        userList->userList[i] = (User*)malloc(sizeof(User));
        memset(userList->userList[i], 0, sizeof(User));//提前全部置0
        fread(&u_short, 1, sizeof(unsigned short), fp);
        if (u_short > 1024) return -1;//超长内容
        fread(userList->userList[i]->account, 1, u_short, fp);
        fread(&u_short, 1, sizeof(unsigned short), fp);
        if (u_short > 1024) return -1;//超长内容
        fread(userList->userList[i]->password, 1, u_short, fp);
        fread(&u_char, 1, sizeof(unsigned char), fp);
        if (u_char > 0) {
            if (u_char > 64) return -1;//超长内容
            fread(userList->userList[i]->token, 1, u_char, fp);
        }
        fread(&userList->userList[i]->ownerID, 1, sizeof(int), fp);
    }
    fclose(fp);
    return 0;
}
/*
 传输格式：
    [
        {
            "account":account,
            "ownerID":ownerID
        },
        {...},
        {...},
        ...
    ]
*/
char* UserList_infosafeJson() {
    if (userList == NULL) {
        userList = (UserList*)malloc(sizeof(UserList));
        userList->userList = (User**)malloc(sizeof(User*));
        userList->userListSize = 0;
    }
    int isGBKsys = isGBKSystem();
    sBuffer sb;
    sBuffer_init(&sb, userList->userListSize * 64);
    sBuffer_appendl(&sb, "[\n", 2);
    for (size_t i = 0;i < userList->userListSize;i++) {
        sBuffer_append(&sb, "{\n\"account\":\"");
        char* text = userList->userList[i]->account;
        int len = strlen(text);
        if (isGBKsys && !isUTF8(text, len)) {
            text = gbk_to_utf8(text, &len);
        }
        sBuffer_utf8jsonstr(&sb, text, len);
        if (text != userList->userList[i]->account && text) free(text);
        sBuffer_fmt(&sb, "\",\n\"ownerID\":%d\n}", userList->userList[i]->ownerID);
        if (i < userList->userListSize - 1) {
            sBuffer_appendl(&sb, ",\n", 2);
        } else {
            sBuffer_appendl(&sb, "\n", 1);
        }
    }
    sBuffer_appendl(&sb, "]", 1);
    char* json = (char*)malloc(sb.size + 1);
    memcpy(json, sb.buff, sb.size);
    json[sb.size] = '\0';
    sBuffer_free(&sb);
    return json;
}