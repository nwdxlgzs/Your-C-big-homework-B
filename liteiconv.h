#ifndef LITEICONV_H
#define LITEICONV_H
#ifdef __WIN32__
#include <windows.h>
#else
#include <iconv.h>
#include <locale.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
extern int isGBKSystem();
extern char* urlDecode(const char* src, int* Retlen);
extern char* urlEncode(const char* Rsrc, int* Retlen, int skip_rl);
//���Ҫ����������⣬�����ṩ��GBK��UTF8���໥ת�����ж�
extern char* utf8_to_gbk(const char* src, int* len);
extern char* gbk_to_utf8(const char* src, int* len);
extern int isUTF8(const char* str, int length);
#endif  // LITEICONV_H

