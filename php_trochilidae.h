#ifndef PHP_TROCHILIDAE_H
#define PHP_TROCHILIDAE_H 1

#define PHP_TROCHILIDAE_VERSION "0.0.1-dev"
#define PHP_TROCHILIDAE_EXTNAME "trochilidae"
#define PHP_TROCHILIDAE_COLLECTORS_MAX 16
#define PHP_TROCHILIDAE_MODE_CLI 0x01
#define PHP_TROCHILIDAE_MODE_CGI 0x02

#include <sys/resource.h>
#include "utils.h"
#include "tr_network.h"

#ifdef PHP_WIN32
# define PHP_TROCHILIDAE_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
# define PHP_TROCHILIDAE_API __attribute__ ((visibility("default")))
#else
# define PHP_TROCHILIDAE_API
#endif

ZEND_BEGIN_MODULE_GLOBALS(trochilidae)
    bool modeCli;
    char hostName[128];
    TrCollector collectors[PHP_TROCHILIDAE_COLLECTORS_MAX];
    TrRequestData requestData;
ZEND_END_MODULE_GLOBALS(trochilidae)

#ifdef ZTS
#define TR_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(trochilidae, v)
#else
#define TR_G(v) (trochilidae_globals.v)
#endif

static PHP_FUNCTION(trochilidae_nop);

extern zend_module_entry trochilidae_module_entry;

#endif