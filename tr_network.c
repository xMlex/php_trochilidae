//
// Created by xMlex on 13.09.2021.
//

#include "tr_network.h"

int tr_net_send(TrUDPSocket *socket, const void *buf, size_t size)
{
    return sendto(socket->socketFd, buf, size, MSG_CONFIRM, (const struct sockaddr *) &socket->sockAddressIn, socket->sockAddressLen);
}

int tr_net_create_collector(TrCollector *result) {
    struct addrinfo *ai_list;
    struct addrinfo *ai_ptr = NULL;
    struct addrinfo ai_hints;

    if (result->udpSocket.initialized && (time(NULL) - result->udpSocket.createAt) < 65) {
//        zend_printf("udpSocket cache\n");
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


void tr_write_string(ubyte * buf, int * pos, char * str)
{
    size_t len = strlen(str);
    memcpy(&buf[*pos], str, len);
    *pos += len;
    buf[*pos] = 0x00;
    *pos += 1;
}
void tr_write_c(ubyte * buf, int * pos, void * c)
{
    memcpy(&buf[*pos], c, 1);
    *pos += 1;
}
void tr_write_h(ubyte * buf, int * pos, void * c)
{
    memcpy(&buf[*pos], c, 2);
    *pos += 2;
}
void tr_write_d(ubyte * buf, int * pos, void * c)
{
    memcpy(&buf[*pos], c, 4);
    *pos += 4;
}
void tr_write_q(ubyte * buf, int * pos, void * c)
{
    memcpy(&buf[*pos], c, 8);
    *pos += 8;
}
