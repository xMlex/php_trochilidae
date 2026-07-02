#ifndef STUB_PHP_H
#define STUB_PHP_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>

#define emalloc(s)         malloc(s)
#define erealloc(p, s)     realloc(p, s)
#define efree(p)           free(p)
#define estrdup(s)         strdup(s)
#define ecalloc(n, s)      calloc(n, s)

typedef unsigned char zend_bool;
typedef struct { void *ptr; } zend_resource;

/*
 * Stub for php_error_docref – used by tr_network.c.
 * In test builds this is a no-op.
 */
#define php_error_docref(...)

/* Minimal Zend macros used by public headers */
#define Z_OBJ_P(zv)        ((void*)(zv))
#define Z_OBJ(zv)          (*Z_OBJ_P(zv))
#define INTERNAL_FUNCTION_PARAMETERS void

#endif
