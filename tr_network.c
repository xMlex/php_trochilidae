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
    return PHP_TROCHILIDAE_REQUEST_METHOD_UNDEFINED;
}

extern int tr_client_init(TrClient *client) {
    if (client->initialized == 1) {
        return 0;
    }
    return tr_client_create(client);
}

extern int tr_client_create(TrClient *client) {
    if (client->initialized == 1) {
        close(client->socketFd);
    }
    client->initialized = 0;
    if ((client->socketFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        return 1;
    }
    memset(&client->sockAddressIn, 0, sizeof(client->sockAddressIn));

    if (tr_client_set_addr_info(client) == 1) {
        client->initialized = 0;
        return 1;
    }

//    struct timeval timeout;
//    timeout.tv_sec = 0;
//    timeout.tv_usec = 100000; //100ms
//  not work on UDP?
//    if (setsockopt(client->socketFd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof timeout) < 0) {
//        return 1;
//    }

    client->initialized = 1;
    client->initAt = time(NULL);
    return 0;
}

extern void tr_client_destroy(TrClient *client) {
//    return;
    if (client->initialized == 1) {
        close(client->socketFd);
    }
    client->initialized = 0;
}

int tr_client_set_addr_info(TrClient *client) {
    struct addrinfo ai_hints;

    memset(&ai_hints, 0, sizeof(ai_hints));

#ifdef AI_ADDRCONFIG
    ai_hints.ai_flags |= AI_ADDRCONFIG;
#endif
    ai_hints.ai_family = AF_UNSPEC;
    ai_hints.ai_socktype = SOCK_DGRAM;
    ai_hints.ai_addr = NULL;
    ai_hints.ai_canonname = NULL;
    ai_hints.ai_next = NULL;

    struct addrinfo *ai_list;

    int status = getaddrinfo(client->host, client->port, &ai_hints, &ai_list);
    if (status != 0) {
        return 1;
    }

    struct addrinfo *ai_ptr = NULL;
    for (ai_ptr = ai_list; ai_ptr != NULL; ai_ptr = ai_ptr->ai_next) {
        int fd = socket(ai_ptr->ai_family, ai_ptr->ai_socktype, ai_ptr->ai_protocol);
        if (fd >= 0) {
            memcpy(&client->sockAddressIn, ai_ptr->ai_addr, ai_ptr->ai_addrlen);
            client->sockAddressLen = ai_ptr->ai_addrlen;
            break;
        }
    }
    freeaddrinfo(ai_list);
    return 0;
}

size_t tr_client_send(TrClient *client, const void *buf, size_t size) {
    if (client->initialized) {
        return sendto(client->socketFd, buf, size, MSG_CONFIRM, (const struct sockaddr *) &client->sockAddressIn,
                      client->sockAddressLen);
    }
    return -1;
}


void tr_client_w_str(char *buf, size_t *pos, char *str) {
    if (str == NULL) {
        buf[*pos] = 0x00;
        *pos += 1;
        return;
    }

    size_t len = strlen(str);
    memcpy(&buf[*pos], str, len);
    *pos += len;
    buf[*pos] = 0x00;
    *pos += 1;
}

void tr_client_w_h(char *buf, size_t *pos, void *c) {
    memcpy(&buf[*pos], c, 2);
    *pos += 2;
}

void tr_client_w_c(char *buf, size_t *pos, void *c) {
    memcpy(&buf[*pos], c, 1);
    *pos += 1;
}

void tr_client_w_d(char *buf, size_t *pos, void *c) {
    memcpy(&buf[*pos], c, 4);
    *pos += 4;
}

void tr_client_w_tv(char *buf, size_t *pos, struct timeval *tv) {
    tr_client_w_d(buf, pos, &tv->tv_sec);
    tr_client_w_d(buf, pos, &tv->tv_usec);
}

void tr_client_w_q(char *buf, size_t *pos, void *c) {
    memcpy(&buf[*pos], c, 8);
    *pos += 8;
}