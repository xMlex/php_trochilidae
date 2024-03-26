//
// Created by mlex on 29.11.2022.
//
static void php_trochilidae_ctor_globals(zend_trochilidae_globals *globals);

static void php_trochilidae_dtor_globals(zend_trochilidae_globals *globals);

static void collect_metrics_before_request();

static void collect_metrics_after_request();

static inline char *tr_fetch_global_var(char *name);

static inline zval *tr_fetch_global_var_ar(char *name);

static size_t sapi_ub_write_counter(const char *str, size_t length);

static void tr_client_w_tags(size_t *pos);
static void tr_client_w_timers(size_t *pos);

static void tr_client_w_argvs(size_t *pos);
static int tr_client_init_or_error(TrClient * client);

static int send_data();

ZEND_BEGIN_ARG_INFO_EX(arginfo_trochilidae_set_tag, 0, 0, 2)
ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
ZEND_ARG_TYPE_INFO(0, val, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_trochilidae_timer_start, 0, 0, 0)
                ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_trochilidae_timer_stop, 0, 0, 0)
                ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_trochilidae_get_info, 0, 0, 0)
ZEND_END_ARG_INFO()