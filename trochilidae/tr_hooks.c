#include "php_trochilidae.h"
#include "trochilidae/tr_hooks.h"

#ifndef ZTS
extern zend_trochilidae_globals trochilidae_globals;
#endif

static void tr_hook_bridge(INTERNAL_FUNCTION_PARAMETERS) {
    zend_function *func = execute_data->func;
    for (uint32_t i = 0; i < TR_G(hook_count); i++) {
        if (TR_G(hooks)[i].zend_func == func) {
            struct timeval s, e, d;
            gettimeofday(&s, NULL);
            TR_G(hooks)[i].original_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU);
            gettimeofday(&e, NULL);
            timersub(&e, &s, &d);
            timeradd(&d, &TR_G(hooks)[i].total_time, &TR_G(hooks)[i].total_time);
            TR_G(hooks)[i].call_count++;
            return;
        }
    }
}

static const char *skip_spaces(const char *p) {
    while (*p == ' ' || *p == '\t') p++;
    return p;
}

static void trim_tail(char *p) {
    size_t len = strlen(p);
    while (len > 0 && (p[len - 1] == ' ' || p[len - 1] == '\t')) len--;
    p[len] = '\0';
}

static zval *tr_hash_str_find_ci(HashTable *ht, const char *name) {
    size_t len = strlen(name);
    zval *found = zend_hash_str_find(ht, name, len);
    if (found != NULL) {
        return found;
    }

    char *lower_name = estrndup(name, len);
    zend_str_tolower(lower_name, len);
    found = zend_hash_str_find(ht, lower_name, len);
    efree(lower_name);

    return found;
}

void tr_hooks_lazy_attach(void) {
    if (TR_G(hooks_attached)) return;
    TR_G(hooks_attached) = true;
    TR_G(hook_count) = 0;

    const char *list = TR_G(hook_list);
    if (!list || !*list) return;

    char *dup = estrdup(list);
    char *p = dup;

    while (p && *p && TR_G(hook_count) < TR_MAX_HOOKS) {
        p = (char *)skip_spaces(p);
        if (!*p) break;

        char *end = strchr(p, ',');
        if (end) *end++ = '\0';

        trim_tail(p);
        if (!*p) { p = end; continue; }

        TrHookEntry *e = &TR_G(hooks)[TR_G(hook_count)];
        memset(e, 0, sizeof(*e));

        char *sep = strstr(p, "->");
        if (!sep) sep = strstr(p, "::");
        if (!sep) sep = strchr(p, '.');

        if (sep && sep != p) {
            *sep = '\0';
            char *method = sep + (sep[1] == '>' ? 2 : (sep[1] == ':' ? 2 : 1));
            trim_tail(p);
            method = (char *)skip_spaces(method);
            trim_tail(method);
            if (*p && *method) {
                e->type = TR_HOOK_METHOD;
                strlcpy(e->class_name, p, sizeof(e->class_name));
                strlcpy(e->func_name, method, sizeof(e->func_name));
            } else {
                p = end;
                continue;
            }
        } else {
            if (*p) {
                e->type = TR_HOOK_FUNCTION;
                strlcpy(e->func_name, p, sizeof(e->func_name));
            } else {
                p = end;
                continue;
            }
        }

        zend_function *fe = NULL;
        if (e->type == TR_HOOK_FUNCTION) {
            zval *tmp = tr_hash_str_find_ci(EG(function_table), e->func_name);
            if (tmp) fe = Z_PTR_P(tmp);
        } else {
            zval *tmp = tr_hash_str_find_ci(CG(class_table), e->class_name);
            if (tmp) {
                zend_class_entry *ce = Z_PTR_P(tmp);
                zval *m = tr_hash_str_find_ci(&ce->function_table, e->func_name);
                if (m) fe = Z_PTR_P(m);
            }
        }

        if (fe && fe->type == ZEND_INTERNAL_FUNCTION) {
            bool dup = false;
            for (uint32_t i = 0; i < TR_G(hook_count); i++) {
                if (TR_G(hooks)[i].zend_func == fe) {
                    dup = true;
                    break;
                }
            }
            if (!dup) {
                e->zend_func = fe;
                e->original_handler = ((zend_internal_function*)fe)->handler;
                ((zend_internal_function*)fe)->handler = tr_hook_bridge;
                TR_G(hook_count)++;
            }
        } else if (TR_G(debug)) {
            if (e->type == TR_HOOK_FUNCTION) {
                php_error(E_WARNING, "trochilidae: hook \"%s\" not found", e->func_name);
            } else {
                php_error(E_WARNING, "trochilidae: hook \"%s::%s\" not found", e->class_name, e->func_name);
            }
        }

        p = end;
    }

    efree(dup);
}

void tr_hooks_detach(void) {
    for (uint32_t i = 0; i < TR_G(hook_count); i++) {
        if (!TR_G(hooks)[i].zend_func || !TR_G(hooks)[i].original_handler) {
            continue;
        }
        ((zend_internal_function *)TR_G(hooks)[i].zend_func)->handler = TR_G(hooks)[i].original_handler;
        TR_G(hooks)[i].zend_func = NULL;
        TR_G(hooks)[i].original_handler = NULL;
        TR_G(hooks)[i].call_count = 0;
        TR_G(hooks)[i].total_time = (struct timeval){0, 0};
    }
    TR_G(hook_count) = 0;
    TR_G(hooks_attached) = false;
}

void tr_hooks_reset(void) {
    for (uint32_t i = 0; i < TR_G(hook_count); i++) {
        TR_G(hooks)[i].call_count = 0;
        TR_G(hooks)[i].total_time = (struct timeval){0, 0};
    }
}

static void build_hook_name(TrHookEntry *e, char *buf, size_t bufsz) {
    if (e->type == TR_HOOK_FUNCTION) {
        strlcpy(buf, e->func_name, bufsz);
    } else {
        snprintf(buf, bufsz, "%s->%s", e->class_name, e->func_name);
    }
}

void tr_hooks_serialize(struct tr_array *msg) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < TR_G(hook_count); i++) {
        if (TR_G(hooks)[i].zend_func && TR_G(hooks)[i].call_count > 0) count++;
    }
    tr_array_write_short(msg, &count);

    for (uint32_t i = 0; i < TR_G(hook_count); i++) {
        if (!TR_G(hooks)[i].zend_func || TR_G(hooks)[i].call_count == 0) continue;
        char name[512];
        build_hook_name(&TR_G(hooks)[i], name, sizeof(name));
        tr_array_write_string(msg, name);
        tr_array_write_long(msg, &TR_G(hooks)[i].call_count);
        tr_array_write_tv(msg, &TR_G(hooks)[i].total_time);
    }
}
