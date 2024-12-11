//
// Created by xMlex on 17.09.2021.
//
#ifndef PHP_TROCHILIDAE_UTILS_H
#define PHP_TROCHILIDAE_UTILS_H

#include <time.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>

#define PHP_TROCHILIDAE_COLLECTORS_MAX 10

typedef unsigned long uLong;
typedef unsigned char byte;

#define tv_assign(a, b) { (a)->tv_sec = (b)->tv_sec; (a)->tv_usec = (b)->tv_usec; };
#define tv_assign_rusage(a, b) { (a)->tv_sec = (b)->ru_stime; (a)->tv_usec = (b)->ru_utime; };
#define timeval_to_float(t) (float)(t).tv_sec + (float)(t).tv_usec / 1000000.0


extern void d2tv(double x, struct timeval *tv);
extern int str_to_int_with_default(const char *str, int default_value);
extern unsigned long generate_random_ulong();

#endif //PHP_TROCHILIDAE_UTILS_H