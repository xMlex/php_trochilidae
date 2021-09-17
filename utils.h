//
// Created by xMlex on 17.09.2021.
//

#ifndef PHP_TROCHILIDAE_UTILS_H
#define PHP_TROCHILIDAE_UTILS_H

typedef uint16_t ushort;
typedef uint32_t uint;
typedef unsigned long uLong;
typedef char byte;
typedef char tr_string;

#define tv_assign(a, b) { (a)->tv_sec = (b)->tv_sec; (a)->tv_usec = (b)->tv_usec; };
#define tv_assign_rusage(a, b) { (a)->tv_sec = (b)->ru_stime; (a)->tv_usec = (b)->ru_utime; };

#endif //PHP_TROCHILIDAE_UTILS_H
