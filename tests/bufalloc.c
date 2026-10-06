/*
 * LD_PRELOAD-счётчик аллокаций буфера сообщения trochilidae.
 *
 * Нужен тестам .phpt, чтобы доказать на уровне процесса три факта:
 *   1) сколько раз расширение просит у glibc буфер DEFAULT_CAPACITY;
 *   2) переиспользуется ли он между вызовами trochilidae_flush();
 *   3) освобождается ли он к концу запроса (не переживает запрос в FPM).
 *
 * Тесты интересуют два "магических" размера:
 *   5194910 - старый DEFAULT_CAPACITY: ровно то число, что попадало в
 *             сообщение "Allowed memory size of 134217728 bytes exhausted";
 *   9216    - текущий DEFAULT_CAPACITY (легко ищется по коду, если ошибка
 *             повторится).
 *
 * Тесты гоняют php дважды - с flush'ами и без, - и сравнивают счётчики:
 * разница показывает, сколько памяти реально выделили flush'и.
 *
 * Сборка:  cc -shared -fPIC -O2 -o bufalloc.so bufalloc.c -ldl
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stddef.h>
#include <stdio.h>

#define SIZE_OLD_CAPACITY 5194910UL
#define SIZE_NEW_CAPACITY 9216UL
#define MAX_TRACK 256

static size_t n_old_alloc = 0, n_new_alloc = 0;
static size_t n_old_free = 0, n_new_free = 0;

struct entry {
    void *ptr;
    size_t size;
};
static struct entry tracked[MAX_TRACK];
static size_t n_tracked = 0;

static void *(*real_malloc)(size_t);
static void *(*real_calloc)(size_t, size_t);
static void *(*real_realloc)(void *, size_t);
static void (*real_free)(void *);

static void resolve(void) {
    if (!real_malloc) {
        real_malloc = dlsym(RTLD_NEXT, "malloc");
        real_calloc = dlsym(RTLD_NEXT, "calloc");
        real_realloc = dlsym(RTLD_NEXT, "realloc");
        real_free = dlsym(RTLD_NEXT, "free");
    }
}

static int find_tracked(void *p) {
    for (size_t i = 0; i < n_tracked; i++) {
        if (tracked[i].ptr == p) return (int) i;
    }
    return -1;
}

static void account_alloc(void *p, size_t n) {
    if (n == SIZE_OLD_CAPACITY) n_old_alloc++;
    if (n == SIZE_NEW_CAPACITY) n_new_alloc++;
    if (!p || (n != SIZE_OLD_CAPACITY && n != SIZE_NEW_CAPACITY)) return;
    if (n_tracked < MAX_TRACK) {
        tracked[n_tracked].ptr = p;
        tracked[n_tracked].size = n;
        n_tracked++;
    }
}

void *malloc(size_t size) {
    resolve();
    void *p = real_malloc(size);
    account_alloc(p, size);
    return p;
}

void *calloc(size_t nmemb, size_t size) {
    resolve();
    void *p = real_calloc(nmemb, size);
    account_alloc(p, nmemb * size);
    return p;
}

void *realloc(void *ptr, size_t size) {
    resolve();
    int idx = find_tracked(ptr);
    if (idx >= 0) {
        if (tracked[idx].size == SIZE_OLD_CAPACITY) n_old_free++;
        else n_new_free++;
        tracked[idx] = tracked[--n_tracked];
    }
    void *p = real_realloc(ptr, size);
    account_alloc(p, size);
    return p;
}

void free(void *ptr) {
    if (!ptr) return;
    resolve();
    int idx = find_tracked(ptr);
    if (idx >= 0) {
        if (tracked[idx].size == SIZE_OLD_CAPACITY) n_old_free++;
        else n_new_free++;
        tracked[idx] = tracked[--n_tracked];
    }
    real_free(ptr);
}

__attribute__((destructor)) static void report(void) {
    fprintf(stdout,
            "\nBUFALLOC old_alloc=%zu old_free=%zu new_alloc=%zu new_free=%zu live=%zu\n",
            n_old_alloc, n_old_free, n_new_alloc, n_new_free, n_tracked);
    fflush(stdout);
}
