#ifndef TROCHILIDAE_COMPAT_H
#define TROCHILIDAE_COMPAT_H

#ifdef TROCHILIDAE_STANDALONE

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define emalloc(s)         malloc(s)
#define erealloc(p, s)     realloc(p, s)
#define efree(p)           free(p)
#define estrdup(s)         strdup(s)

#define php_error_docref(...)
#define E_WARNING          0

#define ZEND_BEGIN_MODULE_GLOBALS(module)
#define ZEND_END_MODULE_GLOBALS(module)
#define ZEND_MODULE_GLOBALS_ACCESSOR(module, v) (module##_globals.v)
#define ZEND_INIT_MODULE_GLOBALS(module, init, copy, dtor)
#define ZEND_DECLARE_MODULE_GLOBALS(module)

typedef unsigned char zend_bool;
typedef struct { void *ptr; } zend_resource;
typedef struct _zend_function zend_function;
typedef void zval;
typedef struct _zend_execute_data zend_execute_data;

#define INTERNAL_FUNCTION_PARAMETERS
#define INTERNAL_FUNCTION_PARAM_PASSTHRU
#define Z_OBJ_P(zv) ((void*)(zv))
#define Z_PTR_P(zv) ((void*)(zv))
#define ZVAL_STRING(zv, s)
#define RETURN_STRING(s)
#define RETURN_TRUE
#define RETURN_FALSE
#define RETURN_LONG(l)
#define RETURN_DOUBLE(d)
#define RETURN_NULL()
#define RETURN_EMPTY_STRING()
#define RETURN_BOOL(b)
#define RETURN_RESOURCE(r)
#define RETURN_SELF()
#define RETURN_THIS()
#define RETURN_OBJ(o)
#define RETURN_ZVAL(zv, copy, dtor)
#define RETURN_ZVAL_REF(r)
#define RETURN_ARR(arr)
#define RETURN_NEW_OBJ(obj)
#define RETURN_STRINGL(s, l)
#define RETURN_STRINGL_COPY(s, l)
#define RETURN_STRING_COPY(s)
#define RETURN_EMPTY_ARRAY()
#define array_init(zv)
#define add_assoc_string(zv, key, str)
#define add_assoc_stringl(zv, key, str, len)
#define add_assoc_long(zv, key, val)
#define add_assoc_double(zv, key, val)
#define add_assoc_bool(zv, key, val)
#define add_assoc_null(zv, key)
#define add_next_index_string(zv, str)
#define add_next_index_long(zv, val)
#define add_next_index_double(zv, val)
#define add_next_index_bool(zv, val)
#define add_next_index_null(zv)
#define zend_parse_parameters(...)
#define ZEND_NUM_ARGS() 0
#define ZEND_FUNCTION(name) void name(INTERNAL_FUNCTION_PARAMETERS)
#define ZEND_METHOD(class, name) void class##_##name(INTERNAL_FUNCTION_PARAMETERS)
#define PHP_FUNCTION(name) void name(INTERNAL_FUNCTION_PARAMETERS)
#define PHP_MINIT_FUNCTION(name) int name##_minit(INIT_FUNC_ARGS)
#define PHP_MSHUTDOWN_FUNCTION(name) int name##_mshutdown(INIT_FUNC_ARGS)
#define PHP_RINIT_FUNCTION(name) int name##_rinit(INIT_FUNC_ARGS)
#define PHP_RSHUTDOWN_FUNCTION(name) int name##_rshutdown(INIT_FUNC_ARGS)
#define PHP_MINFO_FUNCTION(name) void name##_minfo(ZEND_MODULE_INFO_FUNC_ARGS)
#define PHP_GINIT_FUNCTION(name) void name##_ginit(void *globals)
#define PHP_GSHUTDOWN_FUNCTION(name) void name##_gshutdown(void *globals)
#define INIT_FUNC_ARGS
#define ZEND_MODULE_INFO_FUNC_ARGS
#define ZEND_MODULE_STARTUP_N(module) (SUCCESS)
#define ZEND_MODULE_SHUTDOWN_N(module) (SUCCESS)
#define ZEND_MODULE_ACTIVATE_N(module) (SUCCESS)
#define ZEND_MODULE_DEACTIVATE_N(module) (SUCCESS)
#define ZEND_MODULE_INFO_N(module)
#define ZEND_GET_MODULE(module) NULL
#define PHP_MINIT(module) ({ 0; })
#define PHP_MSHUTDOWN(module) ({ 0; })
#define PHP_RINIT(module) ({ 0; })
#define PHP_RSHUTDOWN(module) ({ 0; })
#define PHP_MINFO(module)
#define PHP_GINIT(module)
#define PHP_GSHUTDOWN(module)

#else

#include "php.h"

#endif /* TROCHILIDAE_STANDALONE */

#endif /* TROCHILIDAE_COMPAT_H */
