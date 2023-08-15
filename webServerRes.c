#include "webServerRes.h"
typedef struct WebServerResFile {
    char path[4097];
    int size;
    const unsigned char* buffer;
} WebServerResFile;
static WebServerResFile* mirrorFiles = NULL;
static int mirrorFilesSize = 0;
static const unsigned char __MirrorFile__0[] = {// webui/MIRROR_FILE.html
    'H','e','l','l','o',' ','W','o','r','l','d',
    0
};
static void initMirrorFiles() {
    const int mirrorLen = 1;
    if (mirrorFiles == NULL && mirrorLen > 0) {
        mirrorFiles = malloc(sizeof(WebServerResFile) * mirrorLen);
        mirrorFilesSize = mirrorLen;
        WebServerResFile* file = mirrorFiles;
        {
            strcpy(file->path, "webui/MIRROR_FILE.html");
            file->size = sizeof(__MirrorFile__0);
            file->buffer = __MirrorFile__0;
            file++;
        }
    }
}

HDStatus getWebServerRes(const char* full_url_fpath, BufferBox* buffer_box, int client_sockfd, SocketData* data) {
    initMirrorFiles();
    if (mirrorFiles) {
        for (int i = 0; i < mirrorFilesSize; i++) {
            WebServerResFile* file = &mirrorFiles[i];
            if (strcmp(file->path, full_url_fpath) == 0) {
                buffer_box->size = file->size > 0 ? file->size - 1 : 0;
                buffer_box->buffer = malloc(file->size);
                memcpy(buffer_box->buffer, file->buffer, file->size);
                return HDStatus_normal;
            }
        }
    }
    FILE* fp = fopen(full_url_fpath, "rb");
    if (fp != NULL) {
        fseek(fp, 0, SEEK_END);
        buffer_box->size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        buffer_box->buffer = malloc(buffer_box->size + 1);
        fread(buffer_box->buffer, 1, buffer_box->size, fp);
        buffer_box->buffer[buffer_box->size] = '\0';
        fclose(fp);
    } else {
        do404NotFound(data, client_sockfd);//文件找不到
        return HDStatus_continue;
    }
    return HDStatus_normal;
}


// HDStatus getWebServerRes(const char* full_url_fpath, BufferBox* buffer_box, int client_sockfd, SocketData* data) {
//     FILE* fp = fopen(full_url_fpath, "rb");
//     if (fp != NULL) {
//         fseek(fp, 0, SEEK_END);
//         buffer_box->size = ftell(fp);
//         fseek(fp, 0, SEEK_SET);
//         buffer_box->buffer = malloc(buffer_box->size + 1);
//         fread(buffer_box->buffer, 1, buffer_box->size, fp);
//         buffer_box->buffer[buffer_box->size] = '\0';
//         fclose(fp);
//     } else {
//         do404NotFound(data, client_sockfd);//文件找不到
//         return HDStatus_continue;
//     }
//     return HDStatus_normal;
// }


