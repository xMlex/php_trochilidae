#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <errno.h>
#include "php.h"
#include "php_ini.h"
#include "zend_exceptions.h"
#include "ext/standard/info.h"
#include "SAPI.h"
#include "php_trochilidae.h"
#include "trochilidae_arginfo.h"

static const zend_function_entry functions[];

ZEND_DECLARE_MODULE_GLOBALS(trochilidae)

size_t (*sapi_old_ub_write)(const char *str, size_t str_length);

TrTimer *get_or_create_tr_timer(zend_string *timerName);

void update_server_list();

#ifdef COMPILE_DL_TROCHILIDAE
ZEND_GET_MODULE(trochilidae)
#endif

int res_tr_timer;
int collector_count = 0;

PHP_FUNCTION(trochilidae_set_tag) {
    zend_string *k;
    zend_string *v;
    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
        Z_PARAM_STR(k)
        Z_PARAM_STR(v)
    ZEND_PARSE_PARAMETERS_END();

    add_assoc_str(&TR_G(tags), ZSTR_VAL(k), zend_string_copy(v));
}

PHP_FUNCTION(trochilidae_set_hostname) {
    zend_string *k;
    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
        Z_PARAM_STR(k)
    ZEND_PARSE_PARAMETERS_END();

    strlcpy(TR_G(hostName), ZSTR_VAL(k), sizeof(TR_G(hostName)) - 1);
}

PHP_FUNCTION(trochilidae_timer_start) {
    zend_string *timerName;
    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
        Z_PARAM_STR(timerName)
    ZEND_PARSE_PARAMETERS_END();
    tr_timer_start(get_or_create_tr_timer(timerName));
    RETURN_TRUE;
}

TrTimer *get_or_create_tr_timer(zend_string *timerName) {
    const zval *val = NULL;

    if ((val = zend_hash_find(Z_ARR_P(&TR_G(timers)), timerName)) != NULL) {
        return (TrTimer *) Z_RES_VAL_P(val);
    }

    TrTimer *timer = tr_timer_new(timerName->val);
    timer->resource = zend_register_resource(timer, res_tr_timer);
    add_assoc_resource(&TR_G(timers), timerName->val, timer->resource);

    return timer;
}

void update_server_list() {
    DomainPortEntry *pairs = parse_domain_port_pairs(TR_G(server_list), &collector_count);

    // Ограничение количества collectors
    collector_count = (collector_count > PHP_TROCHILIDAE_COLLECTORS_MAX)
        ? PHP_TROCHILIDAE_COLLECTORS_MAX
        : collector_count;

    // Предварительное освобождение старых клиентов
    for (int i = 0; i < collector_count; i++) {
        tr_client_destroy(&TR_G(collectors)[i]);
    }

    // Инициализация новых клиентов
    for (int i = 0; i < collector_count; i++) {
        TR_G(collectors)[i].host = strdup(pairs[i].domain); // используем strdup для безопасного копирования строки
        if (TR_G(collectors)[i].host == NULL) {
            continue; // Если не удалось выделить память для строки, пропускаем эту итерацию
        }

        TR_G(collectors)[i].port = pairs[i].port;

        if (!tr_client_init(&TR_G(collectors)[i])) {
            free(TR_G(collectors)[i].host); // Если инициализация не удалась, освобождаем память
        }
    }

    free(pairs);
}

PHP_FUNCTION(trochilidae_timer_stop) {
    zend_string *timerName;
    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
        Z_PARAM_STR(timerName)
    ZEND_PARSE_PARAMETERS_END();
    tr_timer_stop(get_or_create_tr_timer(timerName));
    RETURN_TRUE;
}

static PHP_FUNCTION(trochilidae_timer_get_info) {
    array_init(return_value);
    zend_ulong idx;
    zend_string *key;
    zval *val;
    zval timers, timer_info;
    array_init(&timers);
    ZEND_HASH_FOREACH_KEY_VAL(Z_ARR_P(&TR_G(timers)), idx, key, val) {
            array_init(&timer_info);
            add_assoc_long(&timer_info, "startCount", ((TrTimer *) Z_RES_VAL_P(val))->startCount);
            add_assoc_long(&timer_info, "stopCount", ((TrTimer *) Z_RES_VAL_P(val))->stopCount);
            add_assoc_double(&timer_info, "totalExecutionTime",
                             timeval_to_float(((TrTimer *) Z_RES_VAL_P(val))->totalExecutionTime));
            add_assoc_double(&timer_info, "lastExecutionTime",
                             timeval_to_float(((TrTimer *) Z_RES_VAL_P(val))->executionTime));
            add_next_index_zval(&timers, &timer_info);
        }
    ZEND_HASH_FOREACH_END();
    add_assoc_zval(return_value, "timers", &timers);
}

static const zend_function_entry functions[] = {
    PHP_FE(trochilidae_set_tag, arginfo_trochilidae_set_tag)
    PHP_FE(trochilidae_set_hostname, arginfo_trochilidae_set_hostname)
    PHP_FE(trochilidae_timer_start, arginfo_trochilidae_timer_start)
    PHP_FE(trochilidae_timer_stop, arginfo_trochilidae_timer_stop)
    PHP_FE(trochilidae_timer_get_info, arginfo_trochilidae_get_info)
    PHP_FE_END
};

ZEND_INI_MH(onUpdateServerList) {
    if (!new_value) {
        return FAILURE;
    }
    TR_G(server_list) = new_value->val;
    update_server_list();
    return SUCCESS;
}

PHP_INI_BEGIN()
    STD_PHP_INI_BOOLEAN("trochilidae.enabled", "1", PHP_INI_ALL, OnUpdateBool, enabled, zend_trochilidae_globals,
                        trochilidae_globals)
    STD_PHP_INI_ENTRY("trochilidae.server_list", NULL, PHP_INI_ALL, onUpdateServerList, server_list,
                      zend_trochilidae_globals, trochilidae_globals)
PHP_INI_END()

static PHP_MINIT_FUNCTION(trochilidae) {
    ZEND_INIT_MODULE_GLOBALS(trochilidae, php_trochilidae_ctor_globals, php_trochilidae_dtor_globals);
    REGISTER_INI_ENTRIES();

    TR_G(modeCli) = (sapi_module.name && strcmp(sapi_module.name, "cli") == 0);
    sapi_old_ub_write = sapi_module.ub_write;
    sapi_module.ub_write = sapi_ub_write_counter;

    res_tr_timer = zend_register_list_destructors_ex(res_tr_timer_dtor, NULL, "res_trochilidae_timer", module_number);

    array_init(&TR_G(tags));
    array_init(&TR_G(timers));

    return SUCCESS;
}

static PHP_MSHUTDOWN_FUNCTION(trochilidae) {
    for (int i = 0; i < PHP_TROCHILIDAE_COLLECTORS_MAX; ++i) {
        tr_client_destroy(&TR_G(collectors[i]));
    }
    return SUCCESS;
}

static PHP_RINIT_FUNCTION(trochilidae) {
    collect_metrics_before_request();
    return SUCCESS;
}

static PHP_RSHUTDOWN_FUNCTION(trochilidae) {
    if (TR_G(enabled) != false) {
        send_data();
    }
    zend_hash_clean(Z_ARRVAL_P(&TR_G(tags)));
    zend_hash_clean(Z_ARRVAL_P(&TR_G(timers)));
    return SUCCESS;
}

static int send_data() {
    collect_metrics_after_request();

    uint8_t modeType = PHP_TROCHILIDAE_MODE_CGI;
    if (TR_G(modeCli)) {
        modeType = PHP_TROCHILIDAE_MODE_CLI;
    }

    char *request_domain;
    if (TR_G(requestData).request_domain) {
        request_domain = TR_G(requestData).request_domain;
    } else {
        request_domain = strdup(sapi_module.name);
    }

    char *request_uri;
    if (TR_G(requestData).request_uri) {
        request_uri = TR_G(requestData).request_uri;
    } else {
        request_uri = strdup(sapi_module.name);
    }

    tr_array_init(&TR_G(msg), 0);
    tr_array_write_tv(&TR_G(msg), &TR_G(requestData).start_time);
    tr_array_write_byte(&TR_G(msg), &modeType);
    tr_array_write_byte(&TR_G(msg), &TR_G(requestData).request_method);
    tr_array_write_long(&TR_G(msg), &TR_G(requestData).mem_peak_usage);
    tr_array_write_tv(&TR_G(msg), &TR_G(requestData).executionTime);
    tr_array_write_tv(&TR_G(msg), &TR_G(requestData).CPUUsageUserTime);
    tr_array_write_tv(&TR_G(msg), &TR_G(requestData).CPUUsageSystemTime);
    tr_array_write_long(&TR_G(msg), &TR_G(requestData).response_http_size);
    tr_array_write_word(&TR_G(msg), &TR_G(requestData).responseCode);

    tr_array_write_string(&TR_G(msg), TR_G(hostName));
    tr_array_write_string(&TR_G(msg), request_domain);
    tr_array_write_string(&TR_G(msg), request_uri);
    tr_array_write_string(&TR_G(msg), TR_G(requestData).request_id);

    //argv
    uint32_t argvCount = 0;
    const zval *argvList = tr_fetch_global_var_ar(strdup("argv"));
    argvCount = zend_array_count(Z_ARR_P(argvList));
    tr_array_write_short(&TR_G(msg), &argvCount); // count
    if (argvCount > 0) {
        zend_string *key;
        zval *val;
        // Итерация по аргументам
        ZEND_HASH_FOREACH_VAL(Z_ARR_P(argvList), val) {
            tr_array_write_string(&TR_G(msg), Z_STRVAL_P(val));
        } ZEND_HASH_FOREACH_END();
    }

    //tags
    const uint32_t tagCount = zend_array_count(Z_ARR_P(&TR_G(tags)));
    tr_array_write_short(&TR_G(msg), &tagCount); // count
    if (tagCount > 0) {
        zend_string *key;
        zval *val;
        ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARR_P(&TR_G(tags)), key, val) {
            tr_array_write_string_size(&TR_G(msg), key->val, key->len);
            tr_array_write_string_size(&TR_G(msg), Z_STRVAL_P(val), Z_STRLEN_P(val));
        }
        ZEND_HASH_FOREACH_END();
    }

    // timers
    const uint32_t timersCount = zend_array_count(Z_ARR_P(&TR_G(timers)));
    tr_array_write_short(&TR_G(msg), &timersCount); // count
    if (timersCount > 0) {
        zend_string *key;
        zval *val;
        ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARR_P(&TR_G(timers)), key, val) {
            tr_array_write_string(&TR_G(msg), key->val);
            tr_array_write_word(&TR_G(msg), &((TrTimer *) Z_RES_VAL_P(val))->startCount);
            tr_array_write_tv(&TR_G(msg), &((TrTimer *) Z_RES_VAL_P(val))->totalExecutionTime);
        }
        ZEND_HASH_FOREACH_END();
    }

    const size_t sizeMsg = tr_array_get_size(&TR_G(msg));

    TR_G(bytesSend) += sizeMsg;

    // init clients
    for (int i = 0; i < collector_count; i++) {
        tr_client_refresh_server(&TR_G(collectors)[i]);

        //printf("send_data: %zu to %s:%d\n", sizeMsg, TR_G(collectors)[i].host, TR_G(collectors)[i].port);

        const ssize_t cnt = tr_client_send(&TR_G(collectors)[i], TR_G(msg).data, sizeMsg);
        if (cnt == -1) {
            char *errorBuf = strerror(errno);
            php_error_docref(NULL, E_NOTICE,
                             "[trochilidae] tr_net_send: %zu - %s,  address: %s:%i",
                             cnt, errorBuf, TR_G(collectors)[i].host, TR_G(collectors)[i].port
            );
        }
    }

    tr_array_free(&TR_G(msg));

    return SUCCESS;

}

static void php_trochilidae_ctor_globals(zend_trochilidae_globals *globals) {
    memset(globals, 0, sizeof(*globals));
    gethostname(globals->hostName, sizeof(globals->hostName));
}

static void php_trochilidae_dtor_globals(zend_trochilidae_globals *globals) {
}

static PHP_MINFO_FUNCTION(trochilidae) {
    char bufHost[256], bufName[32], initialized[32];

    php_info_print_table_start();
    php_info_print_table_header(2, "Trochilidae", "Info");
    php_info_print_table_row(2, "Version", PHP_TROCHILIDAE_VERSION);

    snprintf(bufName, sizeof(bufName), "%lu", TR_G(requestCount));
    php_info_print_table_row(2, "Requests", bufName);

    snprintf(bufName, sizeof(bufName), "%lu AVG: %lu", TR_G(bytesSend), TR_G(bytesSend) / TR_G(requestCount));
    php_info_print_table_row(2, "Bytes send", bufName);

    snprintf(bufName, sizeof(bufName), "%d", *tr_network_get_domain_resolve_cache_size());
    php_info_print_table_row(2, "DNS Resolve cache count:", bufName);

    for (int i = 0; i < collector_count; ++i) {
        if (!TR_G(collectors)[i].initialized) {
            continue;
        }
        snprintf(bufHost, sizeof(bufHost), "%s:%i", TR_G(collectors)[i].host, TR_G(collectors)[i].port);
        snprintf(bufName, sizeof(bufName), "Collector: %d", i + 1);
        snprintf(initialized, sizeof(initialized), "Init: %d At: %ld", TR_G(collectors[i].initialized),
                 TR_G(collectors[i].initAt));
        php_info_print_table_row(2, bufName, bufHost);
    }
    php_info_print_table_end();

    DISPLAY_INI_ENTRIES();
}

zend_module_entry trochilidae_module_entry = {
    STANDARD_MODULE_HEADER,
    PHP_TROCHILIDAE_EXTNAME,
    functions,
    PHP_MINIT(trochilidae),
    PHP_MSHUTDOWN(trochilidae),
    PHP_RINIT(trochilidae),
    PHP_RSHUTDOWN(trochilidae),
    PHP_MINFO(trochilidae),
    PHP_TROCHILIDAE_VERSION,
    STANDARD_MODULE_PROPERTIES
};

static void collect_metrics_before_request() {
    TR_G(requestCount)++;
    struct rusage u;
    gettimeofday(&TR_G(requestData).executionTime, NULL);
    gettimeofday(&TR_G(requestData).start_time, NULL);
    getrusage(RUSAGE_SELF, &u);
    tv_assign(&TR_G(requestData).CPUUsageUserTime, &u.ru_utime);
    tv_assign(&TR_G(requestData).CPUUsageSystemTime, &u.ru_stime);

    TR_G(requestData).response_http_size = 0;
    if (TR_G(modeCli)) {
        TR_G(requestData).request_method = PHP_TROCHILIDAE_REQUEST_METHOD_NONE;
        TR_G(requestData).request_uri = tr_fetch_global_var("SCRIPT_FILENAME");
        TR_G(requestData).request_domain = tr_fetch_global_var("PWD");
    } else {
        TR_G(requestData).request_method = tr_request_method_map(tr_fetch_global_var("REQUEST_METHOD"));
        TR_G(requestData).request_uri = tr_fetch_global_var("REQUEST_URI");
        TR_G(requestData).request_domain = tr_fetch_global_var("HTTP_HOST");
        if (TR_G(requestData).request_domain == NULL) {
            TR_G(requestData).request_domain = tr_fetch_global_var("SERVER_NAME");
        }
    }
    TR_G(requestData).request_id = tr_fetch_global_var("HTTP_X_REQUEST_ID");
}

static void collect_metrics_after_request() {
    struct rusage u;
    struct timeval tv;
    TR_G(requestData).mem_peak_usage = zend_memory_peak_usage(1);
    TR_G(requestData).responseCode = SG(sapi_headers).http_response_code;

    gettimeofday(&tv, NULL);
    getrusage(RUSAGE_SELF, &u);
    timersub(&tv, &TR_G(requestData).executionTime, &TR_G(requestData).executionTime);
    timersub(&u.ru_utime, &TR_G(requestData).CPUUsageUserTime, &TR_G(requestData).CPUUsageUserTime);
    timersub(&u.ru_stime, &TR_G(requestData).CPUUsageSystemTime, &TR_G(requestData).CPUUsageSystemTime);
}

static size_t sapi_ub_write_counter(const char *str, size_t length) {
    TR_G(requestData).response_http_size += length;
    return sapi_old_ub_write(str, length);
}

static inline char *tr_fetch_global_var(char *name) {
    zval *tmp;
    if ((Z_TYPE(PG(http_globals)[TRACK_VARS_SERVER]) == IS_ARRAY || zend_is_auto_global_str(ZEND_STRL("_SERVER")))) {
        zend_string *findName = zend_string_init(name, strlen(name), 0);
        tmp = zend_hash_str_find(Z_ARRVAL(PG(http_globals)[TRACK_VARS_SERVER]), ZSTR_VAL(findName), ZSTR_LEN(findName));
        zend_string_release(findName);
        if (tmp) {
            return Z_STRVAL_P(tmp);
        }
    }
    return NULL;
}

static inline zval *tr_fetch_global_var_ar(char *name) {
    zval *tmp;
    if ((Z_TYPE(PG(http_globals)[TRACK_VARS_SERVER]) == IS_ARRAY || zend_is_auto_global_str(ZEND_STRL("_SERVER")))) {
        zend_string *findName = zend_string_init(name, strlen(name), 0);
        tmp = zend_hash_str_find(Z_ARRVAL(PG(http_globals)[TRACK_VARS_SERVER]), ZSTR_VAL(findName), ZSTR_LEN(findName));
        zend_string_release(findName);
        if (Z_TYPE_P(tmp) == IS_ARRAY && zend_array_count(Z_ARR_P(tmp)) > 0) {
            return tmp;
        } else if (Z_TYPE_P(tmp) != IS_NULL) {
            return tmp;
        }
    }
    return NULL;
}
