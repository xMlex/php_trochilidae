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
#include <stdbool.h>
#include "utils.h"

#define PHP_TROCHILIDAE_REQUEST_METHOD_NONE 0x00
#define PHP_TROCHILIDAE_REQUEST_METHOD_GET 0x01
#define PHP_TROCHILIDAE_REQUEST_METHOD_HEAD 0x02
#define PHP_TROCHILIDAE_REQUEST_METHOD_POST 0x03
#define PHP_TROCHILIDAE_REQUEST_METHOD_PUT 0x04
#define PHP_TROCHILIDAE_REQUEST_METHOD_DELETE 0x05
#define PHP_TROCHILIDAE_REQUEST_METHOD_CONNECT 0x06
#define PHP_TROCHILIDAE_REQUEST_METHOD_OPTIONS 0x07
#define PHP_TROCHILIDAE_REQUEST_METHOD_TRACE 0x08
#define PHP_TROCHILIDAE_REQUEST_METHOD_PATCH 0x09

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


typedef struct {
    uLong response_http_size;
    uLong mem_peak_usage;
    int response_http_code;

    byte request_method;
    char *request_uri;
    char *request_domain;

    size_t req_count;
    struct timeval executionTime;
    struct timeval CPUUsageUserTime;
    struct timeval CPUUsageSystemTime;

} TrRequestData;

extern byte tr_request_method_map(const void *httpMethod);

extern int tr_net_create_collector(TrCollector *result);

extern int tr_net_send(TrUDPSocket *socket, const void *buf, size_t size);

extern void tr_write_string(byte *buf, int *pos, char *str);

extern void tr_write_c(byte *buf, int *pos, void *c);

extern void tr_write_h(byte *buf, int *pos, void *c);

extern void tr_write_d(byte *buf, int *pos, void *c);

extern void tr_write_q(byte *buf, int *pos, void *c);

extern void tr_write_tv(byte *buf, int *pos, struct timeval *tv);

#endif //PHP_TROCHILIDAE_TR_NETWORK_H
