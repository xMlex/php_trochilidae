//
// Created by mlex on 21.11.2022.
//

#ifndef PHP_TROCHILIDAE_TR_TIMER_H
#define PHP_TROCHILIDAE_TR_TIMER_H

#include <php.h>

#define Z_TR_TIMER_P(zv) (TrTimer*)Z_OBJ_P((zv))
#define Z_TR_TIMER(zv) (*Z_TR_TIMER_P(zv))

typedef struct {
    char *name;
    int nameLen;
    struct timeval executionTime, totalExecutionTime;
    uint startCount, stopCount;
    // php
    zend_resource *resource;
} TrTimer;

extern TrTimer* tr_timer_new(const char* name);
extern void tr_timer_del(TrTimer* s);
extern void res_tr_timer_dtor(zend_resource *rsrc);
extern void tr_timer_start(TrTimer* s);
extern void tr_timer_stop(TrTimer* s);

#endif //PHP_TROCHILIDAE_TR_TIMER_H
