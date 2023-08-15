#ifndef WEBSERVERRES_H
#define WEBSERVERRES_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "socketLib.h"
#include "util.h"
#include "proxyLib.h"
extern HDStatus getWebServerRes(const char* full_url_fpath, BufferBox* buffer_box, int client_sockfd, SocketData* data);

#endif // WEBSERVERRES_H