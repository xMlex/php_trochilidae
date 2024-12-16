//
// Created by mlex on 21.11.2022.
//

#include "tr_timer.h"

extern TrTimer* tr_timer_new(const char* name) {
    TrTimer* s = (TrTimer*)emalloc(sizeof(TrTimer));
    s->name = estrdup(name);
    s->executionTime.tv_usec = 0;
    s->executionTime.tv_sec = 0;
    s->totalExecutionTime.tv_usec = 0;
    s->totalExecutionTime.tv_sec = 0;
    s->startCount = 0;
    s->stopCount = 0;
    return s;
}

extern void tr_timer_start(TrTimer* s) {
    if (s->startCount > s->stopCount) {
        tr_timer_stop(s);
    }
    s->startCount++;
    gettimeofday( &s->executionTime, NULL);
}

extern void tr_timer_stop(TrTimer* s) {
    if (s->startCount <= s->stopCount) {
        return;
    }
    s->stopCount++;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    timersub(&tv, &s->executionTime, &s->executionTime);
    timeradd(&s->totalExecutionTime, &s->executionTime, &s->totalExecutionTime);
}

extern void tr_timer_free(TrTimer* s) {
    efree(s->name);
    efree(s);
}

extern void res_tr_timer_dtor(zend_resource *rsrc) {
    tr_timer_free((TrTimer *)rsrc->ptr);
}