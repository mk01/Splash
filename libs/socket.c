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
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "config.h"
#include "log.h"
#include "socket.h"

/* Start the socket server */
int socket_start(unsigned short port) {
	int opt = 1;
    struct sockaddr_in address;

    //create a master socket
    if((serverSocket = socket(AF_INET , SOCK_STREAM , 0)) == 0)  {
        logprintf(LOG_ERR, "could not create new socket");
        exit(EXIT_FAILURE);
    }

    //set master socket to allow multiple connections , this is just a good habit, it will work without this
    if(setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ) {
        logprintf(LOG_ERR, "could not set proper socket options");
        exit(EXIT_FAILURE);
    }

    //type of socket created
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    //bind the socket to localhost
    if (bind(serverSocket, (struct sockaddr *)&address, sizeof(address))<0) {
        logprintf(LOG_ERR, "cannot bind to socket port %d, address already in use?", address);
        exit(EXIT_FAILURE);
    }

    //try to specify maximum of 3 pending connections for the master socket
    if(listen(serverSocket, 3) < 0) {
        logprintf(LOG_ERR, "failed to listen to socket");
        exit(EXIT_FAILURE);
    }

	logprintf(LOG_INFO, "server started at port %d", port);

    return 0;
}

int socket_connect(char *address, unsigned short port) {
	struct sockaddr_in serv_addr;
	int sockfd;

	/* Try to open a new socket */
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        logprintf(LOG_ERR, "could not create socket");
		return -1;
    }

	/* Clear the server address */
    memset(&serv_addr, '\0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, address, &serv_addr.sin_addr);

	/* Connect to the server */
    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		return -1;
    } else {
		return sockfd;
	}
}


void socket_close(int sockfd) {
	int i = 0;
	struct sockaddr_in address;
	int addrlen = sizeof(address);

	if(sockfd > 0) {
		getpeername(sockfd, (struct sockaddr*)&address, (socklen_t*)&addrlen);

		for(i=0;i<MAX_CLIENTS;i++) {
			if(socket_clients[i] == sockfd) {
				socket_clients[i] = 0;
				break;
			}
		}

		shutdown(sockfd, SHUT_WR);
		close(sockfd);
		logprintf(LOG_INFO, "client disconnected, ip %s, port %d", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
	}
}

void socket_write(int sockfd, const char *msg, ...) {
	char message[BUFFER_SIZE];
	va_list ap;

	if(strlen(msg) > 0 && sockfd > 0) {
		memset(message, '\0', BUFFER_SIZE);

		va_start(ap, msg);
		vsprintf(message, msg, ap);
		va_end(ap);
		strcat(message, "\n");
		if(send(sockfd, message, strlen(message), MSG_NOSIGNAL) == -1) {
			logprintf(LOG_DEBUG, "socket write failed: %s", message);
			socket_close(sockfd);
		} else {
			if(strcmp(message, "BEAT\n") != 0) {
				logprintf(LOG_DEBUG, "socket write succeeded: %s", message);
			}
			// Prevent buffering of messages
			usleep(100);
		}
	}
}

void socket_write_big(int sockfd, const char *msg, ...) {
	char message[BIG_BUFFER_SIZE];
	va_list ap;

	if(strlen(msg) > 0 && sockfd > 0) {
		memset(message, '\0', BIG_BUFFER_SIZE);

		va_start(ap, msg);
		vsprintf(message, msg, ap);
		va_end(ap);
		strcat(message, "\n");
		if(send(sockfd, message, strlen(message), MSG_NOSIGNAL) == -1) {
			logprintf(LOG_DEBUG, "socket write failed: %s", message);
			socket_close(sockfd);
		} else {
			if(strcmp(message, "BEAT\n") != 0) {
				logprintf(LOG_DEBUG, "socket write succeeded: %s", message);
			}
			// Prevent buffering of messages
			usleep(100);
		}
	}
}

char *socket_read(int sockfd) {
	char *recvBuff = malloc((sizeof(char)*BUFFER_SIZE));
	memset(recvBuff, '\0', BUFFER_SIZE);

	if(read(sockfd, recvBuff, BUFFER_SIZE) < 1) {
		return NULL;
	} else {
		return recvBuff;
	}
}

char *socket_read_big(int sockfd) {
	char *recvBuff = malloc((sizeof(char)*BIG_BUFFER_SIZE));
	memset(recvBuff, '\0', BIG_BUFFER_SIZE);

	if(read(sockfd, recvBuff, BIG_BUFFER_SIZE) < 1) {
		return NULL;
	} else {
		return recvBuff;
	}
}

int socket_msgcmp(char *a, char *b) {
	char tmp[BUFFER_SIZE];
	strcpy(tmp, b);
	strcat(tmp, "\n");
	return strcmp(a, tmp);
}

void *wait_for_data(void *param) {
	struct socket_callback_t *socket_callback = (struct socket_callback_t *)param;

	int activity;
	int i, n, sd;
    int max_sd;
    struct sockaddr_in address;
	int addrlen = sizeof(address);
    char readBuff[BUFFER_SIZE];
	fd_set readfds;
	char *pch;

	while(1) {
		do {
			//clear the socket set
			FD_ZERO(&readfds);

			//add master socket to set
			FD_SET((long unsigned int)serverSocket, &readfds);
			max_sd = serverSocket;

			//add child sockets to set
			for(i=0;i<MAX_CLIENTS;i++) {
				//socket descriptor
				sd = socket_clients[i];

				//if valid socket descriptor then add to read list
				if(sd > 0)
					FD_SET((long unsigned int)sd, &readfds);

				//highest file descriptor number, need it for the select function
				if(sd > max_sd)
					max_sd = sd;
			}
			//wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
			activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
		} while(activity == -1 && errno == EINTR);

        //If something happened on the master socket , then its an incoming connection
        if(FD_ISSET((long unsigned int)serverSocket, &readfds)) {
            if((clientSocket = accept(serverSocket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
                logprintf(LOG_ERR, "failed to accept client");
                exit(EXIT_FAILURE);
            }

            //inform user of socket number - used in send and receive commands
            logprintf(LOG_INFO, "new client, ip: %s, port: %d", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
			logprintf(LOG_DEBUG, "client fd: %d", clientSocket);
            //send new connection accept message
            socket_write(clientSocket, "{\"message\":\"accept connection\"}");

            //add new socket to array of sockets
            for(i=0;i<MAX_CLIENTS;i++) {
                //if position is empty
                if(socket_clients[i] == 0) {
                    socket_clients[i] = clientSocket;
					if(socket_callback->client_connected_callback != NULL)
						socket_callback->client_connected_callback(i);
                    logprintf(LOG_DEBUG, "client id: %d", i);
                    break;
                }
            }
        }
        memset(readBuff, '\0', BUFFER_SIZE);
        //else its some IO operation on some other socket :)
        for(i=0;i<MAX_CLIENTS;i++) {
			sd = socket_clients[i];

            if(FD_ISSET((long unsigned int)sd , &readfds)) {
                //Check if it was for closing, and also read the incoming message
                if((n = (int)read(sd, readBuff, sizeof(readBuff)-1)) == 0) {
                    //Somebody disconnected, get his details and print
                    getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
					logprintf(LOG_INFO, "client disconnected, ip %s, port %d", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
					if(socket_callback->client_disconnected_callback != NULL)
						socket_callback->client_disconnected_callback(i);
					//Close the socket and mark as 0 in list for reuse
					close(sd);
					socket_clients[i] = 0;
                } else {
                    //set the string terminating NULL byte on the end of the data read
                    readBuff[n] = '\0';

					if(n > -1 && socket_callback->client_data_callback != NULL) {
						pch = strtok(readBuff, "\n");
						while(pch != NULL) {
							strcat(pch, "\n");
							socket_callback->client_data_callback(i, pch);
							pch = strtok(NULL, "\n");
							if(pch == NULL)
								break;
						}
					}
                }
            }
        }
    }
}
