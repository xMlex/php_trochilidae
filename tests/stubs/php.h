#ifndef STUB_PHP_H
#define STUB_PHP_H

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>

#define emalloc(s)         malloc(s)
#define erealloc(p, s)     realloc(p, s)
#define efree(p)           free(p)
#define estrdup(s)         strdup(s)

typedef unsigned char zend_bool;
typedef struct { void *ptr; } zend_resource;

/* Minimal PHP error constants used by tr_network.c */
#define E_WARNING          2

/* No-op stub for php_error_docref used outside PHP context */
#define php_error_docref(...)

#endif
