#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
#include <math.h>
#include "map.h"

#define INBUFLEN 1551
#define TOPICMAXLEN 50
#define CONTENTMAXLEN 1501
#define CMDMAXLEN 12
#define TYPEMAXLEN 12
#define IPLEN 16
#define IDMAXLEN 11
#define MAXCLIENTS 100
#define MAXMESSAGES 1000
#define CLIENTPORT 5

#define DIE(assertion, call_description)    \
    do {                                    \
        if (assertion) {                    \
            fprintf(stderr, "Error: " call_description);    \
            exit(EXIT_FAILURE);                \
        }                                    \
    } while(0)

#define ERR(assertion, call_description)    \
    do {                                    \
        if (assertion) {                    \
            fprintf(stderr, call_description);    \
        }                                    \
    } while(0)

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

//#pragma pack(push, 1)

// Message received from UDP
struct message {
	char topic[TOPICMAXLEN];
    uint8_t data_type;
    char content[CONTENTMAXLEN];
};

struct queued_message {
    char ip[IPLEN];
    uint16_t portno;
	char topic[TOPICMAXLEN];
    uint8_t data_type;
    char content[CONTENTMAXLEN];
    char id[IDMAXLEN];
};

struct UDP2TCP {
    char ip[IPLEN];
    uint16_t portno;
    struct message received_message;
};

struct subscribe_command {
    char topic[TOPICMAXLEN];
    char command[CMDMAXLEN];//subscribe or unsubscribe
    uint8_t sf;
    char id[IDMAXLEN];
};

struct client {
    char id[IDMAXLEN];
    bool online;
    map_int_t subscriptions;
};

//#pragma pack(pop)

void remove_message(struct queued_message *array, int index, int array_length) {
    int i;
    for(i = index; i < array_length - 1; i++) {
        array[i] = array[i + 1];
    }
    struct queued_message empty_msg;
    array[array_length - 1] = empty_msg;
}

#endif