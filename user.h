#ifndef USER_H
#define USER_H
#define DEFAULT_USER_STORAGE_PATH "user.dat"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "liteiconv.h"
typedef struct User {
    char account[1025];//’À∫≈
    char password[1025];//√‹¬Î
    char token[65];//¡Ó≈∆
    int ownerID;//À˘”–’ﬂID
}User;
typedef struct UserList {
    User** userList;
    size_t userListSize;
}UserList;
extern UserList* userList;

extern User* User_create(const char* account, const char* password);
extern User* User_getUser(const char* account);
extern User* User_getUserByToken(const char* token);
extern int User_login(User* user, const char* password);
extern int User_logout(User* user);
extern int User_isLogin(User* user);
extern int UserList_dump(const char* path);
extern int UserList_load(const char* path);
extern char* UserList_infosafeJson();



#endif // USER_H