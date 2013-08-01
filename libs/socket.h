/*
Copyright 2013 CurlyMo <curlymoo1@gmail.com>

This file is part of Splash - Linux GUI Framebuffer Splash.

Splash is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

Splash is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
details.

You should have received a copy of the GNU General Public License along
with Splash. If not, see <http://www.gnu.org/licenses/>

Various functions used in Splash are derived from fbi (Gerd Hoffmann <gerd@kraxel.org>)
*/

#ifndef _SOCKETS_H_
#define _SOCKETS_H_

#include "config.h"

typedef struct socket_callback_t socket_callback_t;

struct socket_callback_t {
    void (*client_connected_callback)(int);
    void (*client_disconnected_callback)(int);
    void (*client_data_callback)(int, char*);
};

int serverSocket;
int clientSocket;
int socket_clients[MAX_CLIENTS];

/* Start the socket server */
int socket_start(unsigned short port);
int socket_connect(char *address, unsigned short port);
void socket_close(int i);
void socket_write(int sockfd, const char *msg, ...);
void socket_write_big(int sockfd, const char *msg, ...);
char *socket_read(int sockfd);
char *socket_read_big(int sockfd);
int socket_msgcmp(char *a, char *b);
void *wait_for_data(void *param);

#endif
