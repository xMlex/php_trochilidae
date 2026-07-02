#ifndef STUB_PHP_H
#define STUB_PHP_H

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>

#define emalloc(s)         malloc(s)
#define erealloc(p, s)     realloc(p, s)
#define efree(p)           free(p)
#define estrdup(s)         strdup(s)

typedef unsigned char zend_bool;
typedef struct { void *ptr; } zend_resource;

#endif
