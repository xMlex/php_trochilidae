#ifndef PHP_TROCHILIDAE_HOOKS_H
#define PHP_TROCHILIDAE_HOOKS_H

#include "php.h"
#include "trochilidae/tr_array.h"

#define TR_MAX_HOOKS 64
#define TR_HOOK_CLASS_NAME_MAX 64
#define TR_HOOK_FUNC_NAME_MAX 64

typedef enum {
    TR_HOOK_FUNCTION,
    TR_HOOK_METHOD,
} TrHookType;

typedef struct {
    TrHookType type;
    char class_name[TR_HOOK_CLASS_NAME_MAX];
    char func_name[TR_HOOK_FUNC_NAME_MAX];
    zend_function *zend_func;
    void (*original_handler)(INTERNAL_FUNCTION_PARAMETERS);
    unsigned long call_count;
    struct timeval total_time;
} TrHookEntry;

void tr_hooks_lazy_attach(void);
void tr_hooks_reset(void);
void tr_hooks_serialize(struct tr_array *msg);

#endif
