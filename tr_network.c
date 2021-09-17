//
// Created by xMlex on 13.09.2021.
//

#include "tr_network.h"

byte tr_request_method_map(const void *httpMethod) {
    if (!httpMethod) {
        return PHP_TROCHILIDAE_REQUEST_METHOD_NONE;
    }
    if (strcmp(httpMethod, "GET") == 0) {
        return PHP_TROCHILIDAE_REQUEST_METHOD_GET;
    }
    if (strcmp(httpMethod, "HEAD") == 0) {
        return PHP_TROCHILIDAE_REQUEST_METHOD_HEAD;
    }
    if (strcmp(httpMethod, "POST") == 0) {
        return PHP_TROCHILIDAE_REQUEST_METHOD_POST;
    }
    if (strcmp(httpMethod, "PUT") == 0) {
        return PHP_TROCHILIDAE_REQUEST_METHOD_PUT;
    }
    if (strcmp(httpMethod, "DELETE") == 0) {
        return PHP_TROCHILIDAE_REQUEST_METHOD_DELETE;
    }
    if (strcmp(httpMethod, "CONNECT") == 0) {
        return PHP_TROCHILIDAE_REQUEST_METHOD_CONNECT;
    }
    if (strcmp(httpMethod, "OPTIONS") == 0) {
        return PHP_TROCHILIDAE_REQUEST_METHOD_OPTIONS;
    }
    if (strcmp(httpMethod, "TRACE") == 0) {
        return PHP_TROCHILIDAE_REQUEST_METHOD_TRACE;
    }
    if (strcmp(httpMethod, "PATCH") == 0) {
        return PHP_TROCHILIDAE_REQUEST_METHOD_PATCH;
    }
    return PHP_TROCHILIDAE_REQUEST_METHOD_NONE;
}

int tr_net_send(TrUDPSocket *socket, const void *buf, size_t size) {
    return sendto(socket->socketFd, buf, size, MSG_CONFIRM, (const struct sockaddr *) &socket->sockAddressIn,
                  socket->sockAddressLen);
}

int tr_net_create_collector(TrCollector *result) {
    struct addrinfo *ai_list;
    struct addrinfo *ai_ptr = NULL;
    struct addrinfo ai_hints;

    if (result->udpSocket.initialized && (time(NULL) - result->udpSocket.createAt) < 65) {
        return 0;
    }

    memset(&ai_hints, 0, sizeof(ai_hints));
    ai_hints.ai_flags = 0;
#ifdef AI_ADDRCONFIG
    ai_hints.ai_flags |= AI_ADDRCONFIG;
#endif
    ai_hints.ai_family = AF_UNSPEC;
    ai_hints.ai_socktype = SOCK_DGRAM;
    ai_hints.ai_addr = NULL;
    ai_hints.ai_canonname = NULL;
    ai_hints.ai_next = NULL;

    ai_list = NULL;
    int status = getaddrinfo(result->host, result->port, &ai_hints, &ai_list);
    if (status != 0) {
        return 1;
    }

    for (ai_ptr = ai_list; ai_ptr != NULL; ai_ptr = ai_ptr->ai_next) {
        int fd = socket(ai_ptr->ai_family, ai_ptr->ai_socktype, ai_ptr->ai_protocol);
        if (fd >= 0) {
            memcpy(&result->udpSocket.sockAddressIn, ai_ptr->ai_addr, ai_ptr->ai_addrlen);
            result->udpSocket.sockAddressLen = ai_ptr->ai_addrlen;
            result->udpSocket.createAt = time(NULL);
            result->udpSocket.socketFd = fd;
            result->udpSocket.initialized = true;
            break;
        }
    }
    freeaddrinfo(ai_list);
    return 0;
}


void tr_write_string(byte *buf, int *pos, char *str) {
    size_t len = strlen(str);
    memcpy(&buf[*pos], str, len);
    *pos += len;
    buf[*pos] = 0x00;
    *pos += 1;
}

void tr_write_c(byte *buf, int *pos, void *c) {
    memcpy(&buf[*pos], c, 1);
    *pos += 1;
}

void tr_write_h(byte *buf, int *pos, void *c) {
    memcpy(&buf[*pos], c, 2);
    *pos += 2;
}

void tr_write_d(byte *buf, int *pos, void *c) {
    memcpy(&buf[*pos], c, 4);
    *pos += 4;
}

void tr_write_tv(byte *buf, int *pos, struct timeval *tv) {
    tr_write_d(buf, pos, &tv->tv_sec);
    tr_write_d(buf, pos, &tv->tv_usec);
}

void tr_write_q(byte *buf, int *pos, void *c) {
    memcpy(&buf[*pos], c, 8);
    *pos += 8;
}
