#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/* Standalone unit test — compiled with -DTROCHILIDAE_STANDALONE.
   compat.h provides malloc-based stubs for emalloc/efree/estrdup. */

/* Include the actual source-under-test. */
#include "trochilidae/tr_array.h"
#include "trochilidae/tr_timer.h"

/* Pull in the implementations */
#include "trochilidae/tr_array.c"
#include "trochilidae/tr_timer.c"

/* ----------------------------------------------------------- */
/*  Test helpers                                                */
/* ----------------------------------------------------------- */

static int tests  = 0;
static int passed = 0;

#define TEST(name)       do { printf("  %-55s ", name); tests++; } while (0)
#define PASS()           do { puts("PASS"); passed++; } while (0)
#define FAIL(msg)        do { printf("FAIL: %s\n", msg); } while (0)
#define CHECK(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while (0)
#define CHECK_INT(a,op,b,msg) do { if (!((a) op (b))) { FAIL(msg); return; } } while (0)

/* ----------------------------------------------------------- */
/*  tr_array tests                                              */
/* ----------------------------------------------------------- */

static void test_arr_init_default() {
    TEST("tr_array_init with default capacity");
    struct tr_array a;
    tr_array_init(&a, 0);
    CHECK(a.data != NULL, "data is NULL");
    CHECK(a.capacity == DEFAULT_CAPACITY, "capacity != DEFAULT_CAPACITY");
    CHECK(a.init_capacity == DEFAULT_CAPACITY, "init_capacity mismatch");
    CHECK(a.size == 0, "size != 0");
    CHECK(a.position == 0, "position != 0");
    tr_array_free(&a);
    PASS();
}

static void test_arr_init_custom() {
    TEST("tr_array_init with custom capacity");
    struct tr_array a;
    tr_array_init(&a, 128);
    CHECK(a.data != NULL, "data is NULL");
    CHECK(a.capacity == 128, "capacity != 128");
    CHECK(a.init_capacity == 128, "init_capacity mismatch");
    tr_array_free(&a);
    PASS();
}

static void test_arr_init_null() {
    TEST("tr_array_init with NULL self (no-op)");
    tr_array_init(NULL, 0);
    PASS();
}

static void test_arr_write_byte() {
    TEST("tr_array_write_byte");
    struct tr_array a;
    tr_array_init(&a, 16);
    unsigned char v = 0xAB;
    tr_array_write_byte(&a, &v);
    CHECK(a.size == 1, "size != 1");
    CHECK(a.position == 1, "position != 1");
    CHECK(a.data[0] == 0xAB, "data[0] wrong value");
    tr_array_free(&a);
    PASS();
}

static void test_arr_write_short() {
    TEST("tr_array_write_short");
    struct tr_array a;
    tr_array_init(&a, 16);
    unsigned short v = 0x1234;
    tr_array_write_short(&a, &v);
    CHECK(a.size == 2, "size != 2");
    CHECK(memcmp(a.data, &v, 2) == 0, "data mismatch");
    tr_array_free(&a);
    PASS();
}

static void test_arr_write_word() {
    TEST("tr_array_write_word");
    struct tr_array a;
    tr_array_init(&a, 16);
    unsigned int v = 0xDEADBEEF;
    tr_array_write_word(&a, &v);
    CHECK(a.size == 4, "size != 4");
    CHECK(memcmp(a.data, &v, 4) == 0, "data mismatch");
    tr_array_free(&a);
    PASS();
}

static void test_arr_write_long() {
    TEST("tr_array_write_long");
    struct tr_array a;
    tr_array_init(&a, 16);
    unsigned long long v = 0x0123456789ABCDEFULL;
    tr_array_write_long(&a, &v);
    CHECK(a.size == 8, "size != 8");
    CHECK(memcmp(a.data, &v, 8) == 0, "data mismatch");
    tr_array_free(&a);
    PASS();
}

static void test_arr_write_data() {
    TEST("tr_array_write_data");
    struct tr_array a;
    tr_array_init(&a, 16);
    const char *payload = "hello";
    tr_array_write_data(&a, payload, 5);
    CHECK(a.size == 5, "size != 5");
    CHECK(memcmp(a.data, payload, 5) == 0, "data mismatch");
    tr_array_free(&a);
    PASS();
}

static void test_arr_write_data_at_pos() {
    TEST("tr_array_write_data_at_pos");
    struct tr_array a;
    tr_array_init(&a, 16);
    const char *payload = "world";
    tr_array_write_data(&a, "xxxxx", 5);
    tr_array_write_data_at_pos(&a, 0, payload, 5);
    CHECK(memcmp(a.data, "world", 5) == 0, "data at pos mismatch");
    CHECK(a.position == 5, "position changed by at_pos write");
    tr_array_free(&a);
    PASS();
}

static void test_arr_write_string() {
    TEST("tr_array_write_string");
    struct tr_array a;
    tr_array_init(&a, 64);
    tr_array_write_string(&a, "abc");
    /* format: word(length) + bytes */
    unsigned int exp_len = 3;
    CHECK(a.size == 4 + 3, "bad total size");
    CHECK(memcmp(a.data, &exp_len, 4) == 0, "length word mismatch");
    CHECK(memcmp(a.data + 4, "abc", 3) == 0, "string data mismatch");
    tr_array_free(&a);
    PASS();
}

static void test_arr_write_string_null() {
    TEST("tr_array_write_string(NULL)");
    struct tr_array a;
    tr_array_init(&a, 64);
    tr_array_write_string(&a, NULL);
    /* writes word(0) */
    unsigned int exp = 0;
    CHECK(a.size == 4, "expected 4 bytes for word(0)");
    CHECK(memcmp(a.data, &exp, 4) == 0, "word should be 0");
    tr_array_free(&a);
    PASS();
}

static void test_arr_write_string_size() {
    TEST("tr_array_write_string_size");
    struct tr_array a;
    tr_array_init(&a, 64);
    tr_array_write_string_size(&a, "xyz", 3);
    unsigned int exp_len = 3;
    CHECK(a.size == 4 + 3, "bad total size");
    CHECK(memcmp(a.data, &exp_len, 4) == 0, "length word mismatch");
    CHECK(memcmp(a.data + 4, "xyz", 3) == 0, "string data mismatch");
    tr_array_free(&a);
    PASS();
}

static void test_arr_set_position() {
    TEST("tr_array_set_position / get");
    struct tr_array a;
    tr_array_init(&a, 64);
    tr_array_write_byte(&a, (byte[]){0x01});
    tr_array_set_position(&a, 0);
    CHECK(tr_array_get_position(&a) == 0, "position should be 0");
    CHECK(tr_array_get_size(&a) == 1, "size should be 1");
    tr_array_free(&a);
    PASS();
}

static void test_arr_clear() {
    TEST("tr_array_clear resets size and position");
    struct tr_array a;
    tr_array_init(&a, 64);
    tr_array_write_byte(&a, (byte[]){0x01});
    tr_array_write_byte(&a, (byte[]){0x02});
    CHECK(a.size == 2, "size should be 2 before clear");
    tr_array_clear(&a);
    CHECK(a.size == 0, "size != 0 after clear");
    CHECK(a.position == 0, "position != 0 after clear");
    CHECK(a.data != NULL, "data should still be allocated");
    CHECK(a.capacity == 64, "capacity should be preserved");
    tr_array_free(&a);
    PASS();
}

static void test_arr_ensure_capacity_growth() {
    TEST("tr_array_ensure_capacity doubles buffer");
    struct tr_array a;
    tr_array_init(&a, 8);
    size_t old_cap = a.capacity;
    /* write enough to trigger realloc */
    char buf[20];
    memset(buf, 0xFF, 20);
    tr_array_write_data(&a, buf, old_cap + 1);
    CHECK(a.capacity >= old_cap * 2, "capacity should at least double");
    CHECK(a.size == old_cap + 1, "size mismatch after growth");
    tr_array_free(&a);
    PASS();
}

static void test_arr_free_resets() {
    TEST("tr_array_free resets fields");
    struct tr_array a;
    tr_array_init(&a, 64);
    tr_array_free(&a);
    CHECK(a.data == NULL, "data should be NULL");
    CHECK(a.size == 0, "size should be 0");
    CHECK(a.capacity == 0, "capacity should be 0");
    PASS();
}

static void test_arr_write_data_zero() {
    TEST("tr_array_write_data with 0 bytes (no-op)");
    struct tr_array a;
    tr_array_init(&a, 16);
    size_t old_pos = a.position;
    tr_array_write_data(&a, "ignored", 0);
    CHECK(a.position == old_pos, "position changed");
    CHECK(a.size == old_pos, "size changed");
    tr_array_free(&a);
    PASS();
}

static void test_arr_ensure_capacity_zero() {
    TEST("tr_array_ensure_capacity(0) is no-op");
    struct tr_array a;
    tr_array_init(&a, 16);
    size_t old_cap = a.capacity;
    tr_array_ensure_capacity(&a, 0);
    CHECK(a.capacity == old_cap, "capacity changed");
    tr_array_free(&a);
    PASS();
}

static void test_arr_write_string_size_null() {
    TEST("tr_array_write_string_size(NULL, 0) writes word(0)");
    struct tr_array a;
    tr_array_init(&a, 64);
    tr_array_write_string_size(&a, NULL, 0);
    unsigned int exp = 0;
    CHECK(a.size == 4, "expected 4 bytes");
    CHECK(memcmp(a.data, &exp, 4) == 0, "word should be 0");
    tr_array_free(&a);
    PASS();
}

static void test_arr_write_tv() {
    TEST("tr_array_write_tv (timeval)");
    struct tr_array a;
    tr_array_init(&a, 64);
    struct timeval tv = { .tv_sec = 12345, .tv_usec = 67890 };
    tr_array_write_tv(&a, &tv);
    CHECK(a.size == 8, "size should be 4+4=8");
    uint32_t sec;
    memcpy(&sec, a.data, 4);
    CHECK(sec == 12345, "tv_sec mismatch");
    uint32_t usec;
    memcpy(&usec, a.data + 4, 4);
    CHECK(usec == 67890, "tv_usec mismatch");
    tr_array_free(&a);
    PASS();
}

static void test_arr_consecutive_writes_trigger_multiple_growth() {
    TEST("tr_array consecutive writes trigger multiple reallocs");
    struct tr_array a;
    tr_array_init(&a, 4);
    /* Write small amounts repeatedly to force several doublings */
    char buf[256];
    memset(buf, 0xAA, sizeof(buf));
    int i;
    for (i = 0; i < 10; i++) {
        tr_array_write_data(&a, buf, 20);
    }
    CHECK(a.size == 200, "size should be 200");
    CHECK(a.capacity >= 256, "capacity should have grown to >=256");
    CHECK(a.init_capacity == 4, "init_capacity should remain 4");
    /* verify content is intact */
    for (i = 0; i < 10; i++) {
        CHECK(memcmp(a.data + i * 20, buf, 20) == 0, "data corrupted after realloc");
    }
    tr_array_free(&a);
    PASS();
}

static void test_arr_mixed_writes() {
    TEST("tr_array mixed write_byte/short/word/long");
    struct tr_array a;
    tr_array_init(&a, 64);
    byte       b  = 0xBB;
    short      s  = 0x1122;
    uint32_t   w  = 0xAABBCCDD;
    uint64_t   l  = 0x0102030405060708ULL;

    tr_array_write_byte (&a, &b);
    tr_array_write_short(&a, &s);
    tr_array_write_word (&a, &w);
    tr_array_write_long (&a, &l);

    size_t pos = 0;
    CHECK(a.data[pos] == b,       "byte mismatch"); pos += 1;
    CHECK(memcmp(a.data + pos, &s, 2) == 0, "short mismatch"); pos += 2;
    CHECK(memcmp(a.data + pos, &w, 4) == 0, "word mismatch");  pos += 4;
    CHECK(memcmp(a.data + pos, &l, 8) == 0, "long mismatch");
    tr_array_free(&a);
    PASS();
}

static void test_arr_size_after_setpos_and_overwrite() {
    TEST("size after set_position + overwrite (shrink)");
    struct tr_array a;
    tr_array_init(&a, 64);
    tr_array_write_data(&a, "abcdefghij", 10);
    CHECK(a.size == 10, "initial size");
    tr_array_set_position(&a, 0);
    tr_array_write_data(&a, "xyz", 3);
    CHECK(a.position == 3, "position after write");
    CHECK(a.size == 10, "size should remain 10 (overwrite, not append)");
    CHECK(memcmp(a.data, "xyzdefghij", 10) == 0, "data mismatch after overwrite");
    tr_array_free(&a);
    PASS();
}

static void test_arr_size_after_setpos_forward() {
    TEST("size after set_position forward + write");
    struct tr_array a;
    tr_array_init(&a, 64);
    tr_array_write_data(&a, "hello", 5);
    CHECK(a.size == 5, "initial size");
    CHECK(a.position == 5, "initial position");
    tr_array_set_position(&a, 100);
    tr_array_write_data(&a, "abc", 3);
    CHECK(a.position == 103, "position after write");
    CHECK(a.size == 103, "size should be 103 (position is new end)");
    CHECK(memcmp(a.data + 100, "abc", 3) == 0, "data at offset 100");
    tr_array_free(&a);
    PASS();
}

static void test_arr_size_with_inline_writes_after_setpos() {
    TEST("size with inline helpers after set_position");
    struct tr_array a;
    tr_array_init(&a, 64);
    tr_array_write_byte(&a, (byte[]){0xAA});
    tr_array_write_word(&a, (uint32_t[]){0x12345678});
    CHECK(a.size == 5, "size after 1+4 bytes");
    tr_array_set_position(&a, 0);
    tr_array_write_short(&a, (short[]){0xBEEF});
    CHECK(a.position == 2, "position after set+short");
    CHECK(a.size == 5, "size stays 5 (overwrite, not append)");
    tr_array_set_position(&a, 50);
    tr_array_write_long(&a, (uint64_t[]){0xDEADBEEFCAFEULL});
    CHECK(a.position == 58, "position after set+long");
    CHECK(a.size == 58, "size = 58 (new end past old size)");
    tr_array_free(&a);
    PASS();
}

/* ----------------------------------------------------------- */
/*  tr_timer tests                                              */
/* ----------------------------------------------------------- */

static void test_tmr_new() {
    TEST("tr_timer_new sets name and zeroes counters");
    TrTimer *t = tr_timer_new("test_timer");
    CHECK(t != NULL, "timer is NULL");
    CHECK(strcmp(t->name, "test_timer") == 0, "name mismatch");
    CHECK(t->startCount == 0, "startCount != 0");
    CHECK(t->stopCount  == 0, "stopCount != 0");
    CHECK(t->executionTime.tv_sec == 0, "executionTime.tv_sec != 0");
    CHECK(t->executionTime.tv_usec == 0, "executionTime.tv_usec != 0");
    CHECK(t->totalExecutionTime.tv_sec == 0, "total.tv_sec != 0");
    CHECK(t->totalExecutionTime.tv_usec == 0, "total.tv_usec != 0");
    tr_timer_free(t);
    PASS();
}

static void test_tmr_start_stop_once() {
    TEST("tr_timer_start/stop once");
    TrTimer *t = tr_timer_new("once");
    tr_timer_start(t);
    struct timespec ts = { .tv_sec = 0, .tv_nsec = 1000000 }; /* 1ms */
    nanosleep(&ts, NULL);
    tr_timer_stop(t);
    CHECK(t->startCount == 1, "startCount != 1");
    CHECK(t->stopCount  == 1, "stopCount != 1");
    CHECK(t->totalExecutionTime.tv_sec == 0, "total sec should be 0 for 1ms");
    CHECK(t->totalExecutionTime.tv_usec >= 500, "total usec should be >= 500");
    CHECK(t->totalExecutionTime.tv_usec < 10000, "total usec should be < 10ms");
    tr_timer_free(t);
    PASS();
}

static void test_tmr_auto_stop_on_restart() {
    TEST("tr_timer_start auto-stops before restart");
    TrTimer *t = tr_timer_new("auto_stop");
    tr_timer_start(t);
    struct timespec ts = { .tv_sec = 0, .tv_nsec = 500000 }; /* 0.5ms */
    nanosleep(&ts, NULL);
    /* second start should auto-stop the first lap */
    tr_timer_start(t);
    CHECK(t->startCount == 2, "startCount != 2");
    CHECK(t->stopCount  == 1, "stopCount != 1 (auto-stop)");
    CHECK(t->totalExecutionTime.tv_usec > 0, "total should have accumulated");
    tr_timer_free(t);
    PASS();
}

static void test_tmr_stop_noop_if_not_running() {
    TEST("tr_timer_stop is no-op when not running");
    TrTimer *t = tr_timer_new("noop");
    tr_timer_stop(t);
    CHECK(t->startCount == 0, "startCount changed");
    CHECK(t->stopCount  == 0, "stopCount changed");
    CHECK(t->totalExecutionTime.tv_sec == 0, "total changed");
    tr_timer_free(t);
    PASS();
}

static void test_tmr_free_null_name() {
    TEST("tr_timer_free does not crash");
    TrTimer *t = tr_timer_new("freeme");
    tr_timer_free(t);
    PASS();
}

static void test_tmr_execution_time_is_delta_after_stop() {
    TEST("tr_timer executionTime holds delta after stop");
    TrTimer *t = tr_timer_new("delta");
    tr_timer_start(t);
    struct timespec ts = { .tv_sec = 0, .tv_nsec = 2000000 }; /* 2ms */
    nanosleep(&ts, NULL);
    tr_timer_stop(t);
    /* After stop, executionTime = last lap duration */
    CHECK(t->executionTime.tv_sec == 0, "lap sec should be 0");
    CHECK(t->executionTime.tv_usec >= 1000, "lap usec should be >= 1000");
    CHECK(t->executionTime.tv_usec < 20000, "lap usec should be < 20ms");
    tr_timer_free(t);
    PASS();
}

static void test_tmr_stop_twice() {
    TEST("tr_timer_stop called twice (second is no-op)");
    TrTimer *t = tr_timer_new("double_stop");
    tr_timer_start(t);
    struct timespec ts = { .tv_sec = 0, .tv_nsec = 500000 };
    nanosleep(&ts, NULL);
    tr_timer_stop(t);
    long first_total = t->totalExecutionTime.tv_usec;
    tr_timer_stop(t);
    CHECK(t->stopCount == 1, "stopCount should still be 1");
    CHECK(t->totalExecutionTime.tv_usec == first_total, "total should not change");
    tr_timer_free(t);
    PASS();
}

static void test_tmr_two_independent_timers() {
    TEST("two independent timers do not interfere");
    TrTimer *a = tr_timer_new("timer_a");
    TrTimer *b = tr_timer_new("timer_b");
    struct timespec ts_a = { .tv_sec = 0, .tv_nsec = 1000000 };
    struct timespec ts_b = { .tv_sec = 0, .tv_nsec = 2000000 };

    tr_timer_start(a);
    nanosleep(&ts_a, NULL);
    tr_timer_stop(a);

    tr_timer_start(b);
    nanosleep(&ts_b, NULL);
    tr_timer_stop(b);

    CHECK(a->startCount == 1, "a startCount");
    CHECK(b->startCount == 1, "b startCount");
    CHECK(a->totalExecutionTime.tv_usec >= 300, "a should be ~1ms");
    CHECK(a->totalExecutionTime.tv_usec < 10000, "a sanity check");
    CHECK(b->totalExecutionTime.tv_usec >= 800, "b should be ~2ms");
    CHECK(b->totalExecutionTime.tv_usec < 20000, "b sanity check");
    /* b should have taken roughly twice as long as a */
    CHECK(b->totalExecutionTime.tv_usec > a->totalExecutionTime.tv_usec,
          "b should be slower than a");

    tr_timer_free(a);
    tr_timer_free(b);
    PASS();
}

static void test_tmr_long_running() {
    TEST("tr_timer accumulates tv_sec for long runs");
    TrTimer *t = tr_timer_new("long");
    struct timespec ts = { .tv_sec = 1, .tv_nsec = 0 };
    tr_timer_start(t);
    nanosleep(&ts, NULL);
    tr_timer_stop(t);
    CHECK(t->totalExecutionTime.tv_sec >= 1, "total sec should be >= 1");
    CHECK(t->totalExecutionTime.tv_sec <= 2, "total sec sanity");
    tr_timer_free(t);
    PASS();
}

static void test_tmr_accumulate() {
    TEST("tr_timer accumulates across multiple start/stop");
    TrTimer *t = tr_timer_new("accumulate");
    struct timespec ts = { .tv_sec = 0, .tv_nsec = 1000000 };
    int i;
    for (i = 0; i < 3; i++) {
        tr_timer_start(t);
        nanosleep(&ts, NULL);
        tr_timer_stop(t);
    }
    CHECK(t->startCount == 3, "startCount != 3");
    CHECK(t->stopCount  == 3, "stopCount != 3");
    CHECK(t->totalExecutionTime.tv_usec >= 1500, "total should be sum of 3 laps");
    tr_timer_free(t);
    PASS();
}

/* ----------------------------------------------------------- */
/*  Main                                                        */
/* ----------------------------------------------------------- */

int main() {
    puts("=== tr_array tests ===");
    test_arr_init_default();
    test_arr_init_custom();
    test_arr_init_null();
    test_arr_write_byte();
    test_arr_write_short();
    test_arr_write_word();
    test_arr_write_long();
    test_arr_write_data();
    test_arr_write_data_at_pos();
    test_arr_write_string();
    test_arr_write_string_null();
    test_arr_write_string_size();
    test_arr_set_position();
    test_arr_clear();
    test_arr_ensure_capacity_growth();
    test_arr_write_data_zero();
    test_arr_ensure_capacity_zero();
    test_arr_write_string_size_null();
    test_arr_write_tv();
    test_arr_consecutive_writes_trigger_multiple_growth();
    test_arr_free_resets();
    test_arr_mixed_writes();
    test_arr_size_after_setpos_and_overwrite();
    test_arr_size_after_setpos_forward();
    test_arr_size_with_inline_writes_after_setpos();

    puts("\n=== tr_timer tests ===");
    test_tmr_new();
    test_tmr_start_stop_once();
    test_tmr_auto_stop_on_restart();
    test_tmr_stop_noop_if_not_running();
    test_tmr_stop_twice();
    test_tmr_execution_time_is_delta_after_stop();
    test_tmr_free_null_name();
    test_tmr_accumulate();
    test_tmr_two_independent_timers();
    test_tmr_long_running();

    printf("\n=== Results: %d/%d passed ===\n", passed, tests);
    return passed == tests ? 0 : 1;
}
