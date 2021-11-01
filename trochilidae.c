#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "zend_exceptions.h"
#include "ext/standard/info.h"
#include "SAPI.h"
#include "php_trochilidae.h"
#include "tr_network.h"

byte packet[65535];

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

#ifdef COMPILE_DL_TROCHILIDAE
ZEND_GET_MODULE(trochilidae)
#endif

ZEND_BEGIN_ARG_INFO_EX(arginfo_trochilidae_nop, 0, 1, 0)
                ZEND_ARG_TYPE_INFO(0, str, IS_STRING, 1)
ZEND_END_ARG_INFO()

PHP_FUNCTION (trochilidae_nop) {
    zend_string *str;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
            Z_PARAM_STR(str)
    ZEND_PARSE_PARAMETERS_END();
    RETVAL_STR(str);
}

static const zend_function_entry functions[] = {
        PHP_FE(trochilidae_nop, arginfo_trochilidae_nop)
        PHP_FE_END
};

PHP_INI_BEGIN()
//                STD_PHP_INI_ENTRY("pinba.server", NULL, PHP_INI_ALL, OnUpdateCollectorAddress, collector_address, zend_pinba_globals, pinba_globals)
//                STD_PHP_INI_ENTRY("pinba.resolve_interval", "60", PHP_INI_ALL, OnUpdateLongGEZero, resolve_interval, zend_pinba_globals, pinba_globals)
//                STD_PHP_INI_ENTRY("pinba.enabled", "0", PHP_INI_ALL, OnUpdateBool, enabled, zend_pinba_globals, pinba_globals)
//                STD_PHP_INI_ENTRY("pinba.auto_flush", "1", PHP_INI_ALL, OnUpdateBool, auto_flush, zend_pinba_globals, pinba_globals)
PHP_INI_END()

static PHP_MINIT_FUNCTION(trochilidae) {
    ZEND_INIT_MODULE_GLOBALS(trochilidae, php_trochilidae_ctor_globals, php_trochilidae_dtor_globals);
    REGISTER_INI_ENTRIES();

    TR_G(modeCli) = (sapi_module.name && strcmp(sapi_module.name, "cli") == 0);
    sapi_old_ub_write = sapi_module.ub_write;
    sapi_module.ub_write = sapi_ub_write_counter;

    //TODO use php ini
    TR_G(collectors)[0].host = "127.0.0.1";
    TR_G(collectors)[0].port = "30001";
    tr_net_create_collector(&TR_G(collectors)[0]);

    return SUCCESS;
}

static PHP_MSHUTDOWN_FUNCTION(trochilidae) {
//    zend_printf("PHP_MSHUTDOWN_FUNCTION\n");
    return SUCCESS;
}

static PHP_RINIT_FUNCTION(trochilidae) {
    collect_metrics_before_request();
    return SUCCESS;
}

static PHP_RSHUTDOWN_FUNCTION(trochilidae) {
    collect_metrics_after_request();

    uint8_t modeType = PHP_TROCHILIDAE_MODE_CGI;
    int pos = 2, sizePos = 0, nul = 0;

    if (TR_G(modeCli)) {
        modeType = PHP_TROCHILIDAE_MODE_CLI;
    }

    tr_write_c(packet, &pos, &modeType);
    tr_write_c(packet, &pos, &TR_G(requestData).request_method);
    tr_write_q(packet, &pos, &TR_G(requestData).mem_peak_usage);
    tr_write_tv(packet, &pos, &TR_G(requestData).executionTime);
    tr_write_tv(packet, &pos, &TR_G(requestData).CPUUsageUserTime);
    tr_write_tv(packet, &pos, &TR_G(requestData).CPUUsageSystemTime);
    tr_write_q(packet, &pos, &TR_G(requestData).response_http_size);
    tr_write_d(packet, &pos, &TR_G(requestData).response_http_code);
    tr_write_string(packet, &pos, TR_G(hostName));
    if (TR_G(requestData).request_domain) {
        tr_write_string(packet, &pos, TR_G(requestData).request_domain);
    } else {
        tr_write_string(packet, &pos, strdup(sapi_module.name));
    }
    if (TR_G(requestData).request_uri) {
        tr_write_string(packet, &pos, TR_G(requestData).request_uri);
    } else {
        tr_write_string(packet, &pos, strdup(sapi_module.name));
    }
    uint32_t argvCount = 0;
    zval * argvList;
    if (TR_G(modeCli)) {
        argvList = tr_fetch_global_var_ar("argv");
        if (argvList) {
            argvCount = zend_array_count(Z_ARR_P(argvList));
        }
    }
    tr_write_h(packet, &pos, &argvCount);
    if (argvCount > 0 && TR_G(modeCli) && argvList) {
        zend_ulong idx;
        zend_string *key;
        zval *val, tmp;
        ZEND_HASH_FOREACH_KEY_VAL(Z_ARR_P(argvList), idx, key, val)
                {
                    tr_write_string(packet, &pos, Z_STRVAL_P(val));
                }ZEND_HASH_FOREACH_END();
    }


    tr_write_h(packet, &sizePos, &pos);

    tr_net_send(&TR_G(collectors)[0].udpSocket, &packet, pos);
    return SUCCESS;
}

static void php_trochilidae_ctor_globals(zend_trochilidae_globals *globals) {
    memset(globals, 0, sizeof(*globals));
    gethostname(globals->hostName, sizeof(globals->hostName));
}

static void php_trochilidae_dtor_globals(zend_trochilidae_globals *globals) {

}

static PHP_MINFO_FUNCTION(trochilidae) {
    php_info_print_table_start();
    php_info_print_table_header(2, "Trochilidae support", "enabled");
    php_info_print_table_row(2, "Extension version", PHP_TROCHILIDAE_VERSION);
    char bufHost[256], bufName[32];
    for (int i = 0; i < PHP_TROCHILIDAE_COLLECTORS_MAX; ++i) {
        if (!TR_G(collectors)[i].udpSocket.initialized) {
            continue;
        }
        snprintf(bufHost, sizeof(bufHost), "%s:%s", TR_G(collectors)[i].host, TR_G(collectors)[i].port);
        snprintf(bufName, sizeof(bufName), "Collector: %d", i);
        php_info_print_table_row(2, bufName, bufHost);
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
        TR_G(requestData).request_domain = tr_fetch_global_var("SERVER_NAME");
    }
}

static void collect_metrics_after_request() {
    struct rusage u;
    struct timeval tv;
    TR_G(requestData).mem_peak_usage = zend_memory_peak_usage(1);
    TR_G(requestData).response_http_code = SG(sapi_headers).http_response_code;

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
        }
    }
    return NULL;
}