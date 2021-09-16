//
// Created by xMlex on 13.09.2021.
//

#ifndef PHP_TROCHILIDAE_TR_NETWORK_H
#define PHP_TROCHILIDAE_TR_NETWORK_H

#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <netdb.h>
#include <zend.h>
#include <time.h>

typedef struct {
    int socketFd;
    struct sockaddr_in sockAddressIn;
    size_t sockAddressLen;
    time_t createAt;
    bool initialized;
} TrUDPSocket;

typedef struct {
    char *host;
    char *port;
    TrUDPSocket udpSocket;
} TrCollector;

typedef uint16_t ushort;
typedef uint32_t uint;
typedef unsigned long uLong;
typedef char ubyte;
typedef char tr_string;

typedef struct { /* {{{ */
    uLong response_http_size;
    uLong mem_peak_usage;
    int response_http_code;

    char *request_uri;
    char *request_domain;

    size_t req_count;
    struct timeval start;
} TrRequestData;

extern int tr_net_create_collector(TrCollector *result);
extern int tr_net_send(TrUDPSocket *socket, const void *buf, size_t size);
extern void tr_write_string(ubyte * buf, int * pos, char * str);
extern void tr_write_c(ubyte * buf, int * pos, void * c);
extern void tr_write_h(ubyte * buf, int * pos, void * c);
extern void tr_write_d(ubyte * buf, int * pos, void * c);
extern void tr_write_q(ubyte * buf, int * pos, void * c);

#endif //PHP_TROCHILIDAE_TR_NETWORK_H
