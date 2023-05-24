//
// Created by mlex on 22.11.2022.
//

#include "trochilidae/utils.h"

extern void d2tv(double x, struct timeval *tv) {
    tv->tv_sec = (long) x;
    tv->tv_usec = (x - (double) tv->tv_sec) * 1000000.0 + 0.5;
}