#ifndef MAIN_H
#define MAIN_H
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "world.h"
#include "player.h"

#define PORT 4200
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

#define STATE_GET_NAME 0
#define STATE_GET_PASSWORD 1
#define STATE_CONFIRM_NEW 2
#define STATE_NEW_PASSWORD 3
#define STATE_PLAYING 4

struct connection
{
    int sockfd;
    int state;
    int active;
    struct sockaddr_in addr;
    struct player *player;
    char bufferin[BUFFER_SIZE];
    char bufferout[BUFFER_SIZE];
    int bufferin_len;
    int bufferout_len;
    struct room *current_room;
};


void handle_login(struct connection *conn);

extern struct connection clients[];

#endif