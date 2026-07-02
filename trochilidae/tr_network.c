//
// Created by xMlex on 13.09.2021.
//

#include "php.h"
#include "trochilidae/tr_network.h"

DomainResolveCacheEntry domain_resolve_cache[PHP_TROCHILIDAE_COLLECTORS_MAX];
int domain_resolve_cache_size = 0;
int domain_resolve_cache_timeout = DOMAIN_RESOLVE_CACHE_TIMEOUT_SEC;

extern int* tr_network_get_domain_resolve_cache_size() {
    return &domain_resolve_cache_size;
}

int find_domain_resolve_cache_lru_entry_index() {
    if (domain_resolve_cache_size == 0) return 0;
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

    // If not found in cache, perform DNS lookup (thread-safe getaddrinfo)
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    struct addrinfo *result = NULL;
    const int err = getaddrinfo(domain, NULL, &hints, &result);
    if (err != 0 || result == NULL) {
        php_error_docref(NULL, E_WARNING, "[tr] getaddrinfo failed for %s: %s", domain, gai_strerror(err));
        ip.s_addr = INADDR_NONE;
    } else {
        ip = ((struct sockaddr_in *)result->ai_addr)->sin_addr;
        freeaddrinfo(result);

        // Add to cache
        if (domain_resolve_cache_size < DOMAIN_RESOLVE_MAX_CACHE_ENTRIES) {
            strlcpy(domain_resolve_cache[domain_resolve_cache_size].domain, domain,
                    sizeof(domain_resolve_cache[domain_resolve_cache_size].domain));
            domain_resolve_cache[domain_resolve_cache_size].ip = ip;
            domain_resolve_cache[domain_resolve_cache_size].last_used = time(NULL);
            domain_resolve_cache_size++;
        } else {
            // Replace least recently used entry
            int lru_index = find_domain_resolve_cache_lru_entry_index();
            strlcpy(domain_resolve_cache[lru_index].domain, domain,
                    sizeof(domain_resolve_cache[lru_index].domain));
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
    if (input == NULL || numPairs == NULL) {
        return NULL;
    }

    *numPairs = 0;
    char *input_dup = strdup(input);
    if (input_dup == NULL) {
        php_error_docref(NULL, E_WARNING, "parse_domain_port_pairs: strdup error");
        return NULL;
    }

    size_t input_len = strlen(input_dup);
    size_t max_pairs = input_len == 0 ? 1 : input_len;
    if (max_pairs > SIZE_MAX / sizeof(DomainPortEntry)) {
        php_error_docref(NULL, E_WARNING, "parse_domain_port_pairs: allocation size overflow");
        free(input_dup);
        return NULL;
    }
    DomainPortEntry* pairs = (DomainPortEntry*)malloc(sizeof(DomainPortEntry) * max_pairs);
    if (pairs == NULL) {
        php_error_docref(NULL, E_WARNING, "parse_domain_port_pairs: malloc error");
        free(input_dup);
        return NULL;
    }

    const char delimiter[] = ",";
    char* token = strtok(input_dup, delimiter);

    while (token != NULL) {
        char domain[MAX_DOMAIN_LENGTH];
        int port = PHP_TROCHILIDAE_SERVER_DEFAULT_PORT;

        char* portSeparator = strchr(token, ':');
        if (portSeparator != NULL) {
            size_t domain_len = (size_t)(portSeparator - token);
            if (domain_len >= sizeof(domain)) {
                php_error_docref(NULL, E_WARNING, "[tr] domain part too long in '%s', skipping", token);
                token = strtok(NULL, delimiter);
                continue;
            }
            memcpy(domain, token, domain_len);
            domain[domain_len] = '\0';
            port = str_to_int_with_default(portSeparator + 1, PHP_TROCHILIDAE_SERVER_DEFAULT_PORT);
        } else {
            strlcpy(domain, token, sizeof(domain));
        }

        strlcpy(pairs[*numPairs].domain, domain, sizeof(pairs[*numPairs].domain));
        pairs[*numPairs].port = port;

        (*numPairs)++;
        token = strtok(NULL, delimiter);
    }
    free(input_dup);
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
        php_error_docref(NULL, E_WARNING, "tr_client_create: client initialized");
        return false;
    }

    if ((client->socketFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        php_error_docref(NULL, E_WARNING, "tr_client_create: socket creation failed");
        return false;
    }

    int sndbuf = PHP_TROCHILIDAE_SO_SNDBUF_SIZE;
    if (setsockopt(client->socketFd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf)) < 0) {
        php_error_docref(NULL, E_WARNING, "[tr_client_create] setsockopt SO_SNDBUF failed");
    }

    int flags = fcntl(client->socketFd, F_GETFL, 0);
    fcntl(client->socketFd, F_SETFL, flags | O_NONBLOCK);

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
    if (client->socketFd >= 0) {
        close(client->socketFd);
    }
    if (client->host){
        free(client->host);
        client->host = NULL;
    }
}

int tr_client_set_addr_info(TrClient *client) {
    if (strcmp(client->host, "") == 0) {
        php_error_docref(NULL, E_WARNING, "tr_client_set_addr_info: not set client->host");
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
    const time_t t = time(NULL);
    if (client->sock_address_refresh_at != 0 && t < (client->sock_address_refresh_at + domain_resolve_cache_timeout)) {
        return true;
    }

    client->sock_address_in.sin_family = AF_INET;
    client->sock_address_in.sin_port = htons(client->port);
    struct in_addr tmp_addr = find_ip_address(client->host);

    if (tmp_addr.s_addr == INADDR_NONE) {
        php_error_docref(NULL, E_WARNING, "tr_client_refresh_server: INADDR_NONE for %s", client->host);
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
        php_error_docref(NULL, E_WARNING, "[tr-send_chunks] Invalid input parameters");
        return -1;
    }

    const size_t chunk_size = client->chunk_size - CHUNK_HEADER_SIZE;
    if (chunk_size <= 0) {
        php_error_docref(NULL, E_WARNING, "[tr-send_chunks] chunk_size small, need > %d", CHUNK_HEADER_SIZE);
        return -1;
    }
    if (client->chunk_size > MAX_CHUNK_SIZE) {
        php_error_docref(NULL, E_WARNING, "[tr-send_chunks] Chunk size too large(%zu), max: %d", chunk_size, MAX_CHUNK_SIZE);
        return -1;
    }

    const unsigned short total_chunks = (size + chunk_size - 1) / chunk_size;

    if (total_chunks > client->chunk_count) {
        php_error_docref(NULL, E_WARNING, "[tr-send_chunks] Data too large to send in %d chunks", client->chunk_count);
        return -1;
    }

    char *packet = malloc(MAX_CHUNK_SIZE);
    if (!packet) {
        php_error_docref(NULL, E_WARNING, "[tr-send_chunks] malloc failed");
        return -1;
    }

    const uint64_t packetId = generate_random_ulong();

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
        memcpy(packet, chunk_header, CHUNK_HEADER_SIZE);
        memcpy(packet + CHUNK_HEADER_SIZE, data + offset, current_chunk_size);

        // Отправляем пакет
        const ssize_t sent = sendto(client->socketFd, packet, current_chunk_size + CHUNK_HEADER_SIZE, 0,
                              (const struct sockaddr *)&client->sock_address_in, sizeof(client->sock_address_in));
        if (sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                client->drops++;
                continue;
            }
            php_error_docref(NULL, E_WARNING, "[tr-send_chunks] sendto error: %s", strerror(errno));
            return -1;
        }
        totalSentSize += sent;
    }
    free(packet);
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
