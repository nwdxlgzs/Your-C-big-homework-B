'''
webuiResBuild.py
将webui的资源文件打包成一个webServerRes.c文件
'''
import os
import sys


class MirrorFile:
    def __init__(self, path, idx):
        self.path = path
        self.idx = idx

    def getUPath(self):
        return self.path.replace("\\", "/")

    def getId(self):
        return self.idx

    def make__MirrorFile_array(self):
        sbuff = []
        sbuff.append(
            f"static const unsigned char __MirrorFile__{self.idx}[] = {{// {self.getUPath()}\n    ")
        with open(self.path, "rb") as f:
            cnt = 0
            while True:
                b = f.read(1)
                if not b:
                    break
                cnt += 1
                b = int.from_bytes(b, byteorder=sys.byteorder) & 0xff
                sbuff.append(f"{b},")
                if cnt % 32 == 0:
                    sbuff.append("\n    ")
        if cnt % 32 != 0:
            sbuff.append("\n    ")
        sbuff.append("0\n};\n")
        return "".join(sbuff)

    def make__MirrorFile_set(self):
        sbuff = []
        sbuff.append(f"        {{\n")
        sbuff.append(
            f"            strcpy(file->path, \"{self.getUPath()}\");\n")
        sbuff.append(
            f"            file->size = sizeof(__MirrorFile__{self.idx});\n")
        sbuff.append(f"            file->buffer = __MirrorFile__{self.idx};\n")
        sbuff.append(f"            file++;\n")
        sbuff.append(f"        }}\n")
        return "".join(sbuff)


fileCnt = 0
mirrorFileList = []


def scanDir(path):
    global fileCnt
    global mirrorFileList
    for root, dirs, files in os.walk(path, topdown=False):
        for name in files:
            print(os.path.join(root, name))
            mirrorFileList.append(MirrorFile(
                os.path.join(root, name), fileCnt))
            fileCnt += 1


scanDir("webui")

print("webServerRes.c.txt文件生成中")
# 生成webServerRes.c.txt文件
with open("webServerRes.c.txt", "w") as f:
    f.write('''#include "webServerRes.h"
typedef struct WebServerResFile {
    char path[128];
    int size;
    const unsigned char* buffer;
} WebServerResFile;
static WebServerResFile* mirrorFiles = NULL;
static int mirrorFilesSize = 0;
''')
    for mf in mirrorFileList:
        f.write(mf.make__MirrorFile_array())
    f.write('''static void initMirrorFiles() {
    const int mirrorLen = '''+str(fileCnt)+''';
    if (mirrorFiles == NULL && mirrorLen > 0) {
        mirrorFiles = malloc(sizeof(WebServerResFile) * mirrorLen);
        mirrorFilesSize = mirrorLen;
        WebServerResFile* file = mirrorFiles;'''+"\n")
    for mf in mirrorFileList:
        f.write(mf.make__MirrorFile_set())
    f.write('''    }
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
        buffer_box->buffer[buffer_box->size] = 0;
        fclose(fp);
    } else {
        do404NotFound(data, client_sockfd);//文件找不到
        return HDStatus_continue;
    }
    return HDStatus_normal;
}''')

print("webServerRes.c.txt文件生成成功")
print("webServerRes.c.txt请检查内容是否正确再替换webServerRes.c文件")

'''
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



'''