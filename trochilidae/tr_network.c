//
// Created by xMlex on 13.09.2021.
//

#include "trochilidae/tr_network.h"

DomainResolveCacheEntry domain_resolve_cache[PHP_TROCHILIDAE_COLLECTORS_MAX];
int domain_resolve_cache_size = 0;
int domain_resolve_cache_timeout = DOMAIN_RESOLVE_CACHE_TIMEOUT_SEC;

extern int* tr_network_get_domain_resolve_cache_size() {
    return &domain_resolve_cache_size;
}

int find_domain_resolve_cache_lru_entry_index() {
    int lru_index = 0;
    time_t oldest_time = domain_resolve_cache[0].last_used;

    for (int i = 1; i < domain_resolve_cache_size; ++i) {
        if (domain_resolve_cache[i].last_used < oldest_time) {
            oldest_time = domain_resolve_cache[i].last_used;
            lru_index = i;
        }
    }

    return lru_index;
}

struct in_addr find_ip_address(const char *domain) {
    struct in_addr ip;
    for (int i = 0; i < domain_resolve_cache_size; ++i) {
        if (strcmp(domain_resolve_cache[i].domain, domain) == 0) {
            domain_resolve_cache[i].last_used = time(NULL);
            return domain_resolve_cache[i].ip;
        }
    }

    // If not found in cache, perform DNS lookup
    struct hostent *host_entry;
    host_entry = gethostbyname(domain);
    if (host_entry == NULL || host_entry->h_addr_list[0] == NULL) {
        ip.s_addr = INADDR_NONE;
    } else {
        ip = *(struct in_addr *) host_entry->h_addr_list[0];
        // Add to cache
        if (domain_resolve_cache_size < DOMAIN_RESOLVE_MAX_CACHE_ENTRIES) {
            strcpy(domain_resolve_cache[domain_resolve_cache_size].domain, domain);
            domain_resolve_cache[domain_resolve_cache_size].ip = ip;
            domain_resolve_cache[domain_resolve_cache_size].last_used = time(NULL);
            domain_resolve_cache_size++;
        } else {
            // Replace least recently used entry
            int lru_index = find_domain_resolve_cache_lru_entry_index();
            strcpy(domain_resolve_cache[lru_index].domain, domain);
            domain_resolve_cache[lru_index].ip = ip;
            domain_resolve_cache[lru_index].last_used = time(NULL);
        }
    }

    return ip;
}

/**
 * @param input example: example.com:80,192.168.0.1,localhost:8080
 * @param numPairs
 * @return
 */
extern DomainPortEntry * parse_domain_port_pairs(const char* input, int* numPairs) {
    DomainPortEntry* pairs = (DomainPortEntry*)malloc(strlen(input) * sizeof(DomainPortEntry));
    if (pairs == NULL) {
        fprintf(stderr,"parse_domain_port_pairs: malloc error\n");
        return pairs;
    }

    *numPairs = 0;
    const char delimiter[] = ",";
    char* token = strtok((char*)input, delimiter);

    while (token != NULL) {
        char domain[MAX_DOMAIN_LENGTH];
        int port = PHP_TROCHILIDAE_SERVER_DEFAULT_PORT;

        char* portSeparator = strchr(token, ':');
        if (portSeparator != NULL) {
            strncpy(domain, token, portSeparator - token);
            domain[portSeparator - token] = '\0';
            port = str_to_int_with_default(portSeparator + 1, PHP_TROCHILIDAE_SERVER_DEFAULT_PORT);
        } else {
            strcpy(domain, token);
        }

        strcpy(pairs[*numPairs].domain, domain);
        pairs[*numPairs].port = port;

        (*numPairs)++;
        token = strtok(NULL, delimiter);
    }
    //printf("parse_domain_port_pairs: %d\n", (*numPairs));
    return pairs;
}

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

extern bool tr_client_init(TrClient *client) {
    if (client->initialized) {
        return false;
    }
    return tr_client_create(client);
}

extern bool tr_client_create(TrClient *client) {
    if (client->initialized) {
        fprintf(stderr, "tr_client_create: client initialized\n");
        return false;
    }

    if ((client->socketFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        fprintf(stderr, "tr_client_create: socket creation failed\n");
        return false;
    }
    if (!tr_client_set_addr_info(client)) {
        return false;
    }
    client->initialized = true;
    client->initAt = time(NULL);
    return true;
}

extern void tr_client_destroy(TrClient *client) {
    if (!client) {
        return;
    }
    client->initialized = false;
    if (client->socketFd > 0) {
        close(client->socketFd);
    }
    if (client->host){
        free(client->host);
    }
}

int tr_client_set_addr_info(TrClient *client) {
    if (strcmp(client->host, "") == 0) {
        fprintf(stderr, "tr_client_set_addr_info: not set client->host\n");
        return false;
    }
    if (client->port <= 0) {
        client->port = PHP_TROCHILIDAE_SERVER_DEFAULT_PORT;
    }
    client->sock_address_refresh_at = 0;
    memset(&client->sock_address_in, 0, sizeof(client->sock_address_in));
    client->sock_address_in.sin_addr.s_addr = INADDR_NONE;
    return true;
}

extern bool tr_client_refresh_server(TrClient *client) {
    time_t t = time(NULL);
    if (client->sock_address_refresh_at > (t + domain_resolve_cache_timeout)) {
        return true;
    }

    client->sock_address_in.sin_family = AF_INET;
    client->sock_address_in.sin_port = htons(client->port);
    struct in_addr tmp_addr = find_ip_address(client->host);

    if (tmp_addr.s_addr == INADDR_NONE) {
        fprintf(stderr, "tr_client_refresh_server: INADDR_NONE for %s\n", client->host);
        // try refresh after 25 sec
        client->sock_address_refresh_at = t - domain_resolve_cache_timeout + 25;
        return false;
    }
    client->sock_address_in.sin_addr = tmp_addr;
    client->sock_address_refresh_at = t;

    return true;
}

size_t tr_client_send(TrClient *client, const void *buf, size_t size) {
    if (!client->initialized) {
        return -1;
    }
    if (!tr_client_refresh_server(client)) {
        return -1;
    }
    return sendto(client->socketFd, buf, size, MSG_CONFIRM, (const struct sockaddr *) &client->sock_address_in,
                  sizeof(client->sock_address_in));
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