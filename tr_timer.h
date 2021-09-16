//
// Created by mlex on 21.11.2022.
//

#ifndef PHP_TROCHILIDAE_TR_TIMER_H
#define PHP_TROCHILIDAE_TR_TIMER_H

typedef struct {
    char *name;
    int nameLen;
    char *value;
    int valueLen;
} TrTimerTag;

typedef struct {
    TrTimerTag **tags;
    int tagsCount;
    struct timeval tmpUTime;
    struct timeval tmpSTime;
    struct timeval uTime;
    struct timeval sTime;
} TrTimer;

#endif //PHP_TROCHILIDAE_TR_TIMER_H
