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
    struct hostent *host_entry = gethostbyname(domain);
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
    client->chunk_size = MAX_CHUNK_SIZE;
    client->chunk_count = MAX_CHUNKS;
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

ssize_t send_chunks(TrClient *client, const byte *data, const size_t size, const bool compressed) {
    if (!client || !data || size == 0) {
        fprintf(stderr, "[tr-send_chunks] Invalid input parameters\n");
        return -1;
    }

    const size_t chunk_size = client->chunk_size - CHUNK_HEADER_SIZE;
    if (chunk_size <= 0) {
        fprintf(stderr, "[tr-send_chunks] chunk_size small, need > %d\n", CHUNK_HEADER_SIZE);
        return -1;
    }
    if (client->chunk_size > MAX_CHUNK_SIZE) {
        fprintf(stderr, "[tr-send_chunks]  Chunk size too large(%lu), max: %d\n", chunk_size, MAX_CHUNK_SIZE);
        return -1;
    }

    const unsigned short total_chunks = (size + chunk_size - 1) / chunk_size;

    if (total_chunks > client->chunk_count) {
        fprintf(stderr, "[tr-send_chunks] Data too large to send in %d chunks\n", MAX_CHUNKS);
        return -1;
    }

    const unsigned long packetId = generate_random_ulong();
    //fprintf(stderr, "[tr-send_chunks] packetId %llu, chunks: %d\n", packetId, total_chunks);

    if (setsockopt(client->socketFd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)) < 0) {
        perror("[tr-send_chunks] setsockopt SO_SNDBUF failed");
    }

    ssize_t totalSentSize = 0;
    unsigned short i = 0;
    for (i = 0; i < total_chunks; ++i) {
        const size_t offset = i * chunk_size;
        const size_t current_chunk_size = (offset + chunk_size > size) ? (size - offset) : chunk_size;

        // Формат: [идентификатор (8 байт)][номер чанка (2 байта)][всего чанков (2 байта)][сжато да/нет (1 байт)][ключ 8 байт]
        unsigned char chunk_header[CHUNK_HEADER_SIZE];
        memcpy(chunk_header, &packetId, 8);
        memcpy(chunk_header + 8, &i, 2);
        memcpy(chunk_header + 10, &total_chunks, 2);
        memcpy(chunk_header + 12, &compressed, 1);
        memset(chunk_header + 13, 0, 8); // key

        // Формируем полный пакет
        char packet[MAX_CHUNK_SIZE];
        memcpy(packet, chunk_header, CHUNK_HEADER_SIZE);
        memcpy(packet + CHUNK_HEADER_SIZE, data + offset, current_chunk_size);

        // Отправляем пакет
        const ssize_t sent = sendto(client->socketFd, packet, current_chunk_size + CHUNK_HEADER_SIZE, 0,
                              (const struct sockaddr *)&client->sock_address_in, sizeof(client->sock_address_in));
        totalSentSize += sent;
        //fprintf(stderr, "[tr-send_chunks] process packetId %llu, chunk: %d, sent: %lu\n", packetId, i, sent);
        if (sent < 0) {
            perror("[tr-send_chunks] sendto error");
            return -1;
        }
    }
    //fprintf(stderr, "[tr-send_chunks] Total packetId %llu, chunk: %d, sent: %lu\n", packetId, i, totalSentSize);
    return totalSentSize;
}


ssize_t tr_client_send(TrClient *client, void *buf, const size_t size) {
    if (!client->initialized) {
        return -1;
    }
    if (!tr_client_refresh_server(client)) {
        return -1;
    }
    const ssize_t result = send_chunks(client, buf, size, false);
    return result;
}
