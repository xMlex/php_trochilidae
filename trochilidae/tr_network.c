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
    const time_t now = time(NULL);
    int cache_index = -1;

    for (int i = 0; i < domain_resolve_cache_size; ++i) {
        if (strcmp(domain_resolve_cache[i].domain, domain) == 0) {
            domain_resolve_cache[i].last_used = now;
            /* `last_used` is for LRU eviction only; freshness is `resolved_at`. */
            if (now - domain_resolve_cache[i].resolved_at < domain_resolve_cache_timeout) {
                return domain_resolve_cache[i].ip;
            }
            cache_index = i; /* expired: re-resolve and refresh in place */
            break;
        }
    }

    // No fresh entry in cache: perform DNS lookup
    struct hostent *host_entry = gethostbyname(domain);
    if (host_entry == NULL || host_entry->h_addr_list[0] == NULL) {
        ip.s_addr = INADDR_NONE;
        return ip;
    }
    ip = *(struct in_addr *) host_entry->h_addr_list[0];

    if (cache_index < 0) {
        if (domain_resolve_cache_size < DOMAIN_RESOLVE_MAX_CACHE_ENTRIES) {
            cache_index = domain_resolve_cache_size++;
        } else {
            cache_index = find_domain_resolve_cache_lru_entry_index();
        }
    }
    strncpy(domain_resolve_cache[cache_index].domain, domain,
            sizeof(domain_resolve_cache[cache_index].domain) - 1);
    domain_resolve_cache[cache_index].domain[sizeof(domain_resolve_cache[cache_index].domain) - 1] = '\0';
    domain_resolve_cache[cache_index].ip = ip;
    domain_resolve_cache[cache_index].last_used = now;
    domain_resolve_cache[cache_index].resolved_at = now;

    return ip;
}

/**
 * @param input example: example.com:80,192.168.0.1,localhost:8080
 * @param numPairs
 * @return
 */
extern DomainPortEntry * parse_domain_port_pairs(const char* input, int* numPairs) {
    *numPairs = 0;
    if (input == NULL) {
        return NULL;
    }

    /* Never tokenise in place: `input` points into the INI entry's zend_string,
     * and strtok() would replace the commas with NULs for every later reader.
     * Bound the token count by the number of collectors the caller can use. */
    const int max_pairs = PHP_TROCHILIDAE_COLLECTORS_MAX;
    DomainPortEntry* pairs = (DomainPortEntry*)malloc((size_t)max_pairs * sizeof(DomainPortEntry));
    if (pairs == NULL) {
        fprintf(stderr,"parse_domain_port_pairs: malloc error\n");
        return NULL;
    }

    const char* cursor = input;
    while (*cursor != '\0' && *numPairs < max_pairs) {
        const char* comma = strchr(cursor, ',');
        const size_t token_len = (comma != NULL)
            ? (size_t)(comma - cursor)
            : strlen(cursor);

        const char* port_separator = memchr(cursor, ':', token_len);
        const size_t domain_len = (port_separator != NULL)
            ? (size_t)(port_separator - cursor)
            : token_len;

        /* strtok skipped empty tokens but kept `:` as an empty-domain pair;
         * keep that behaviour while rejecting oversized names. */
        if (token_len > 0 && domain_len < MAX_DOMAIN_LENGTH) {
            int port = PHP_TROCHILIDAE_SERVER_DEFAULT_PORT;

            if (port_separator != NULL) {
                const size_t port_len = token_len - domain_len - 1;
                char port_str[16];
                if (port_len > 0 && port_len < sizeof(port_str)) {
                    memcpy(port_str, port_separator + 1, port_len);
                    port_str[port_len] = '\0';
                    port = str_to_int_with_default(port_str, PHP_TROCHILIDAE_SERVER_DEFAULT_PORT);
                }
            }

            memcpy(pairs[*numPairs].domain, cursor, domain_len);
            pairs[*numPairs].domain[domain_len] = '\0';
            pairs[*numPairs].port = port;
            ++(*numPairs);
        }

        if (comma == NULL) {
            break;
        }
        cursor = comma + 1;
    }

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
    /* The send buffer does not depend on the current message size: configure
     * it once per client, not on every flush (one less syscall per send). */
    const int sndbuf = MAX_CHUNK_SIZE;
    if (setsockopt(client->socketFd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf)) < 0) {
        perror("[tr_client_create] setsockopt SO_SNDBUF failed");
    }
    if (!tr_client_set_addr_info(client)) {
        close(client->socketFd);
        client->socketFd = -1;
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
    if (client->socketFd >= 0) {
        close(client->socketFd);
        client->socketFd = -1;
    }
    if (client->host) {
        free(client->host);
        client->host = NULL;
    }
    client->initialized = false;
    client->port = 0;
    client->initAt = 0;
    client->sock_address_refresh_at = 0;
    memset(&client->sock_address_in, 0, sizeof(client->sock_address_in));
    client->sock_address_in.sin_addr.s_addr = INADDR_NONE;
    client->chunk_size = 0;
    client->chunk_count = 0;
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
    if (!client || !client->initialized || !client->host) {
        return false;
    }
    const time_t t = time(NULL);
    /* `sock_address_refresh_at` is the time of the last successful resolve (0
     * means "never"). A failed resolve stores `t - timeout + 25`, which leaves
     * a 25 second retry window with this comparison. Report success there only
     * if an address was resolved before (last-known-good); if none was ever
     * resolved, sin_addr is still INADDR_NONE and sendto() would attempt a
     * broadcast to 255.255.255.255 instead of dropping the packet. */
    if (client->sock_address_refresh_at > (t - domain_resolve_cache_timeout)) {
        return client->sock_address_in.sin_addr.s_addr != INADDR_NONE;
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
    if (!client || !client->initialized || !client->host) {
        return -1;
    }
    if (!tr_client_refresh_server(client)) {
        return -1;
    }
    const ssize_t result = send_chunks(client, buf, size, false);
    return result;
}
