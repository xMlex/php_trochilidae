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
#include <arpa/inet.h>
#include "trochilidae/utils.h"

#define PHP_TROCHILIDAE_SERVER_DEFAULT_PORT 30002

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

#define DOMAIN_RESOLVE_MAX_CACHE_ENTRIES 5
#define DOMAIN_RESOLVE_CACHE_TIMEOUT_SEC 600 // 10 minutes

#define MAX_DOMAIN_LENGTH 254

#define CHUNK_HEADER_SIZE 21
#define MAX_CHUNK_SIZE 65507   // Размер данных в одном UDP-пакете (идеал - 1400 байт)
#define MAX_CHUNKS 256        // Максимальное количество чанков
#define MIN_COMPRESSION_SIZE 2048 // Минимальный размер для сжатия данных

typedef struct {
    uLong response_http_size;
    uLong mem_peak_usage;
    int responseCode;

    byte request_method;
    char *request_uri;
    char *request_domain;
    char *request_id;

    size_t req_count;
    struct timeval request_start_time;
    struct timeval executionTime;
    struct timeval CPUUsageUserTime;
    struct timeval CPUUsageSystemTime;
} TrRequestData;

typedef struct {
    bool initialized;
    int socketFd;
    struct sockaddr_in sock_address_in;
    time_t sock_address_refresh_at;
    time_t initAt;
    char *host;
    int port;
    size_t chunk_size;
    unsigned short chunk_count;
} TrClient;

typedef struct {
    char domain[256];
    struct in_addr ip;
    time_t last_used;
    time_t resolved_at;
} DomainResolveCacheEntry;

typedef struct {
    char domain[MAX_DOMAIN_LENGTH];
    int port;
} DomainPortEntry;

extern int* tr_network_get_domain_resolve_cache_size();

extern DomainPortEntry * parse_domain_port_pairs(const char* input, int* numPairs);

extern byte tr_request_method_map(const void *httpMethod);

extern bool tr_client_init(TrClient *client);

extern bool tr_client_create(TrClient *client);

extern void tr_client_destroy(TrClient *client);

extern int tr_client_set_addr_info(TrClient *client);

extern ssize_t tr_client_send(TrClient *client, void *buf, size_t size);

extern bool tr_client_refresh_server(TrClient *client);

#endif //PHP_TROCHILIDAE_TR_NETWORK_H
