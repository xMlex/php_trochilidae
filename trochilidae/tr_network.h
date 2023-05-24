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
#include "trochilidae/utils.h"

#define PHP_TROCHILIDAE_REQUEST_METHOD_UNDEFINED 0xFF
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
    uLong response_http_size;
    uLong mem_peak_usage;
    int responseCode;

    byte request_method;
    char *request_uri;
    char *request_domain;

    size_t req_count;
    struct timeval executionTime;
    struct timeval CPUUsageUserTime;
    struct timeval CPUUsageSystemTime;

} TrRequestData;

typedef struct {
    bool initialized;
    int socketFd;
    struct sockaddr_in sockAddressIn;
    size_t sockAddressLen;
    time_t initAt;
    char *host;
    char *port;
    char *portStr;
} TrClient;


extern byte tr_request_method_map(const void *httpMethod);

extern int tr_client_init(TrClient *client);

extern int tr_client_create(TrClient *client);

extern void tr_client_destroy(TrClient *client);

extern int tr_client_set_addr_info(TrClient *client);

extern size_t tr_client_send(TrClient *client, const void *buf, size_t size);

extern void tr_client_w_h(char *buf, size_t *pos, void *c);

extern void tr_client_w_str(char *buf, size_t *pos, char *str);

extern void tr_client_w_c(char *buf, size_t *pos, void *c);

extern void tr_client_w_d(char *buf, size_t *pos, void *c);

extern void tr_client_w_q(char *buf, size_t *pos, void *c);

extern void tr_client_w_tv(char *buf, size_t *pos, struct timeval *tv);

#endif //PHP_TROCHILIDAE_TR_NETWORK_H
