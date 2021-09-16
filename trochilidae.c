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
#include "tr_network.h"

static const zend_function_entry functions[];

static void php_trochilidae_ctor_globals(zend_trochilidae_globals *globals);

static void php_trochilidae_dtor_globals(zend_trochilidae_globals *globals);

static void collect_metrics_before_request();

static void collect_metrics_after_request();

static inline char *tr_fetch_global_var(char *name);

static inline zval *tr_fetch_global_var_ar(char *name);

static size_t sapi_ub_write_counter(const char *str, size_t length);

ZEND_DECLARE_MODULE_GLOBALS(trochilidae)

size_t (*sapi_old_ub_write)(const char *str, size_t str_length);

static void tr_client_w_tags(size_t *pos);

static void tr_client_w_argvs(size_t *pos);

static int send_data();

#ifdef COMPILE_DL_TROCHILIDAE
ZEND_GET_MODULE(trochilidae)
#endif

ZEND_BEGIN_ARG_INFO_EX(arginfo_trochilidae_set_tag, 0, 1, 0)
                ZEND_ARG_TYPE_INFO(0, str, IS_STRING, 1)
                ZEND_ARG_TYPE_INFO(0, str, IS_STRING, 1)
ZEND_END_ARG_INFO()

PHP_FUNCTION (trochilidae_set_tag) {
    zend_string *key;
    zend_string *val;
    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
            Z_PARAM_STR(key)
            Z_PARAM_STR(val)
    ZEND_PARSE_PARAMETERS_END();

    add_assoc_str(&TR_G(tags), key->val, val);
    RETURN_TRUE;
}

static const zend_function_entry functions[] = {
        PHP_FE(trochilidae_set_tag, arginfo_trochilidae_set_tag)
        PHP_FE_END
};

ZEND_INI_MH(onUpdateTrHost) {
    TR_G(collectors)[0].host = new_value->val;
    return SUCCESS;
}

ZEND_INI_MH(onUpdateTrPort) {
    TR_G(collectors)[0].port = new_value->val;
    return SUCCESS;
}

ZEND_INI_DISP(TrHost) {
    if (type == ZEND_INI_DISPLAY_ORIG && ini_entry->modified && ini_entry->orig_value) {
        TR_G(collectors)[0].host = ZSTR_VAL(ini_entry->orig_value);
    } else if (ini_entry->value) {
        TR_G(collectors)[0].host = ZSTR_VAL(ini_entry->value);
    }
}

ZEND_INI_DISP(TrPort) {
    if (type == ZEND_INI_DISPLAY_ORIG && ini_entry->modified && ini_entry->orig_value) {
        TR_G(collectors)[0].port = ZSTR_VAL(ini_entry->orig_value);
    } else if (ini_entry->value) {
        TR_G(collectors)[0].port = ZSTR_VAL(ini_entry->value);
    }
    tr_client_destroy(&TR_G(collectors)[0]);
}

ZEND_INI_DISP(TrEnabled) {
    if (type == ZEND_INI_DISPLAY_ORIG && ini_entry->modified && ini_entry->orig_value) {
        TR_G(enabled) = strcmp(ZSTR_VAL(ini_entry->orig_value), "1");
    } else if (ini_entry->value) {
        TR_G(enabled) = strcmp(ZSTR_VAL(ini_entry->value), "1");
    }
    tr_client_destroy(&TR_G(collectors)[0]);
}

PHP_INI_BEGIN()
                STD_PHP_INI_ENTRY_EX
                ("trochilidae.enabled", "1", PHP_INI_ALL, OnUpdateBool, enabled, zend_trochilidae_globals,
                 trochilidae_globals, TrEnabled)
                STD_PHP_INI_ENTRY_EX("trochilidae.server", "127.0.0.1", PHP_INI_ALL, onUpdateTrHost, collectors[0].host,
                                     zend_trochilidae_globals, trochilidae_globals, TrHost)
                STD_PHP_INI_ENTRY_EX
                ("trochilidae.port", "30001", PHP_INI_ALL, onUpdateTrPort, collectors[0].port, zend_trochilidae_globals,
                 trochilidae_globals, TrPort)
PHP_INI_END()

static PHP_MINIT_FUNCTION(trochilidae) {
    ZEND_INIT_MODULE_GLOBALS(trochilidae, php_trochilidae_ctor_globals, php_trochilidae_dtor_globals);
    REGISTER_INI_ENTRIES();

    TR_G(modeCli) = (sapi_module.name && strcmp(sapi_module.name, "cli") == 0);
    sapi_old_ub_write = sapi_module.ub_write;
    sapi_module.ub_write = sapi_ub_write_counter;

    if (tr_client_init(&TR_G(collectors)[0]) == 1) {
        php_error_docref(NULL, E_NOTICE,
                         "[trochilidae] tr_client_init address: %s:%s",
                         TR_G(collectors)[0].host, TR_G(collectors)[0].port
        );
        return FAILURE;
    }
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
    array_init(&TR_G(tags));
    return SUCCESS;
}

static PHP_RSHUTDOWN_FUNCTION(trochilidae) {
    if (TR_G(enabled) == false) {
        zval_dtor(&TR_G(tags));
        return SUCCESS;
    }
    return send_data();
}

static int send_data() {
    collect_metrics_after_request();

    uint8_t modeType = PHP_TROCHILIDAE_MODE_CGI;
    size_t pos = 2, sizePos = 0;

    if (TR_G(modeCli)) {
        modeType = PHP_TROCHILIDAE_MODE_CLI;
    }

    tr_client_w_c(TR_G(packet), &pos, &modeType);
    // REQUEST_TIME_FLOAT
    struct timeval requestTV;
    zval *requestTime = tr_fetch_global_var_ar(strdup("REQUEST_TIME_FLOAT"));
    if (requestTime) {
        double rt = zval_get_double(requestTime);
        d2tv(rt, &requestTV);
    } else {
        struct timeval requestTVtmp;
        gettimeofday(&requestTV, &requestTVtmp);
    }
    tr_client_w_tv(TR_G(packet), &pos, &requestTV);

    tr_client_w_c(TR_G(packet), &pos, &TR_G(requestData).request_method);
    tr_client_w_q(TR_G(packet), &pos, &TR_G(requestData).mem_peak_usage);
    tr_client_w_tv(TR_G(packet), &pos, &TR_G(requestData).executionTime);
    tr_client_w_tv(TR_G(packet), &pos, &TR_G(requestData).CPUUsageUserTime);
    tr_client_w_tv(TR_G(packet), &pos, &TR_G(requestData).CPUUsageSystemTime);
    tr_client_w_q(TR_G(packet), &pos, &TR_G(requestData).response_http_size);
    tr_client_w_d(TR_G(packet), &pos, &TR_G(requestData).responseCode);
    tr_client_w_str(TR_G(packet), &pos, TR_G(hostName));
    if (TR_G(requestData).request_domain) {
        tr_client_w_str(TR_G(packet), &pos, TR_G(requestData).request_domain);
    } else {
        tr_client_w_str(TR_G(packet), &pos, strdup(sapi_module.name));
    }
    if (TR_G(requestData).request_uri) {
        tr_client_w_str(TR_G(packet), &pos, TR_G(requestData).request_uri);
    } else {
        tr_client_w_str(TR_G(packet), &pos, strdup(sapi_module.name));
    }

    tr_client_w_argvs(&pos);
    // tags
    tr_client_w_tags(&pos);
    zval_dtor(&TR_G(tags));

    TR_G(bytesSend) += pos;
    // total pkg size
    tr_client_w_h(TR_G(packet), &sizePos, &pos);

    if (tr_client_init(&TR_G(collectors)[0]) == 1) {
        php_error_docref(NULL, E_NOTICE,
                         "[trochilidae] tr_client_init address: %s:%s",
                         TR_G(collectors)[0].host, TR_G(collectors)[0].port
        );
        return FAILURE;
    }
    size_t cnt = tr_client_send(&TR_G(collectors)[0], &TR_G(packet), pos);
    if (cnt == -1) {
        char *errorBuf = strerror(errno);
        php_error_docref(NULL, E_NOTICE,
                         "[trochilidae] tr_net_send: %zu - %s,  address: %s:%s",
                         cnt, errorBuf, TR_G(collectors)[0].host, TR_G(collectors)[0].port
        );
        return FAILURE;
    }

    return SUCCESS;
}

static void tr_client_w_argvs(size_t *pos) {
    uint32_t argvCount = 0;
    if (!TR_G(modeCli)) {
        tr_client_w_h(TR_G(packet), pos, &argvCount);
        return;
    }
    zval *argvList = tr_fetch_global_var_ar(strdup("argv"));
    if (argvList) {
        argvCount = zend_array_count(Z_ARR_P(argvList));
        if (argvCount-1 <= 0) {
            argvCount = 0;
            tr_client_w_h(TR_G(packet), pos, &argvCount);
        }
    }
    if (argvCount > 0 && argvList) {
        argvCount = argvCount - 1;
        tr_client_w_h(TR_G(packet), pos, &argvCount);
        zend_ulong idx;
        zend_string *key;
        zval *val;
        int skippedFirst = false;
        ZEND_HASH_FOREACH_KEY_VAL(Z_ARR_P(argvList), idx, key, val)
                {
                    if (skippedFirst == true) {
                        tr_client_w_str(TR_G(packet), pos, Z_STRVAL_P(val));
                    } else {
                        skippedFirst = true;
                    }
                }
        ZEND_HASH_FOREACH_END();
    }
}

static void tr_client_w_tags(size_t *pos) {
    uint32_t tagCount = zend_array_count(Z_ARR_P(&TR_G(tags)));
    tr_client_w_h(TR_G(packet), pos, &tagCount);
    if (tagCount <= 0) {
        return;
    }
    zend_string *key;
    zval *val;
    ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARR_P(&TR_G(tags)), key, val)
            {
                tr_client_w_str(TR_G(packet), pos, key->val);
                tr_client_w_str(TR_G(packet), pos, Z_STRVAL_P(val));
            }
    ZEND_HASH_FOREACH_END();
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
    php_info_print_table_header(3, "Trochilidae support", "Info", "Additional");
    php_info_print_table_row(3, "Extension version", PHP_TROCHILIDAE_VERSION, initialized);

    snprintf(bufName, sizeof(bufName), "%d", TR_G(enabled));
    php_info_print_table_row(3, "Enabled", bufName, initialized);

    snprintf(bufHost, sizeof(bufName), "Requests: %lu", TR_G(requestCount));
    snprintf(bufName, sizeof(bufName), "BytesSend: %lu Avg: %lu byte", TR_G(bytesSend),
             TR_G(bytesSend) / TR_G(requestCount));
    php_info_print_table_row(3, "Metrics", bufHost, bufName);

    snprintf(bufName, sizeof(bufName), "%d ", getpid());
    php_info_print_table_row(3, "PID", bufName, "");

    for (int i = 0; i < PHP_TROCHILIDAE_COLLECTORS_MAX; ++i) {
        snprintf(bufHost, sizeof(bufHost), "%s:%s", TR_G(collectors)[i].host, TR_G(collectors)[i].port);
        snprintf(bufName, sizeof(bufName), "Collector: %d", i + 1);
        snprintf(initialized, sizeof(initialized), "Init: %d At: %ld", TR_G(collectors[i].initialized),
                 TR_G(collectors[i].initAt));
        php_info_print_table_row(3, bufName, bufHost, initialized);
    }
    php_info_print_table_end();
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
        } else if(Z_TYPE_P(tmp) != IS_ARRAY) {
            return tmp;
        }
    }
    return NULL;
}