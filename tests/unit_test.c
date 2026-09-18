#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "../trochilidae/tr_network.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) do { printf("  TEST: %s ... ", name); } while(0)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); tests_failed++; } while(0)
#define ASSERT(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while(0)

static void test_normal_case() {
    TEST("normal case with mixed ports");
    char input[] = "example.com:80,localhost:3000,another.net";
    int numPairs = 0;
    DomainPortEntry *pairs = parse_domain_port_pairs(input, &numPairs);
    ASSERT(numPairs == 3, "expected 3 pairs");
    ASSERT(strcmp(pairs[0].domain, "example.com") == 0, "domain[0] mismatch");
    ASSERT(pairs[0].port == 80, "port[0] mismatch");
    ASSERT(strcmp(pairs[1].domain, "localhost") == 0, "domain[1] mismatch");
    ASSERT(pairs[1].port == 3000, "port[1] mismatch");
    ASSERT(strcmp(pairs[2].domain, "another.net") == 0, "domain[2] mismatch");
    ASSERT(pairs[2].port == PHP_TROCHILIDAE_SERVER_DEFAULT_PORT, "port[2] should be default");
    free(pairs);
    PASS();
}

static void test_single_domain_without_port() {
    TEST("single domain without port");
    char input[] = "my-host.local";
    int numPairs = 0;
    DomainPortEntry *pairs = parse_domain_port_pairs(input, &numPairs);
    ASSERT(numPairs == 1, "expected 1 pair");
    ASSERT(strcmp(pairs[0].domain, "my-host.local") == 0, "domain mismatch");
    ASSERT(pairs[0].port == PHP_TROCHILIDAE_SERVER_DEFAULT_PORT, "port should be default");
    free(pairs);
    PASS();
}

static void test_single_domain_with_port() {
    TEST("single domain with port");
    char input[] = "myserver:9090";
    int numPairs = 0;
    DomainPortEntry *pairs = parse_domain_port_pairs(input, &numPairs);
    ASSERT(numPairs == 1, "expected 1 pair");
    ASSERT(strcmp(pairs[0].domain, "myserver") == 0, "domain mismatch");
    ASSERT(pairs[0].port == 9090, "port should be 9090");
    free(pairs);
    PASS();
}

static void test_empty_string() {
    TEST("empty string");
    char input[] = "";
    int numPairs = 1;
    DomainPortEntry *pairs = parse_domain_port_pairs(input, &numPairs);
    ASSERT(pairs != NULL, "should return non-NULL pointer");
    ASSERT(numPairs == 0, "expected 0 pairs");
    free(pairs);
    PASS();
}

static void test_null_input() {
    TEST("NULL input returns NULL");
    int numPairs = 99;
    DomainPortEntry *pairs = parse_domain_port_pairs(NULL, &numPairs);
    ASSERT(pairs == NULL, "should return NULL");
    ASSERT(numPairs == 99, "numPairs should be unchanged");
    PASS();
}

static void test_null_numpairs() {
    TEST("NULL numPairs returns NULL");
    DomainPortEntry *pairs = parse_domain_port_pairs("test.com:80", NULL);
    ASSERT(pairs == NULL, "should return NULL");
    PASS();
}

static void test_multiple_without_ports() {
    TEST("multiple domains without ports");
    char input[] = "alpha,beta,gamma";
    int numPairs = 0;
    DomainPortEntry *pairs = parse_domain_port_pairs(input, &numPairs);
    ASSERT(numPairs == 3, "expected 3 pairs");
    ASSERT(strcmp(pairs[0].domain, "alpha") == 0, "domain[0] mismatch");
    ASSERT(pairs[0].port == PHP_TROCHILIDAE_SERVER_DEFAULT_PORT, "port[0] default");
    ASSERT(strcmp(pairs[1].domain, "beta") == 0, "domain[1] mismatch");
    ASSERT(pairs[1].port == PHP_TROCHILIDAE_SERVER_DEFAULT_PORT, "port[1] default");
    ASSERT(strcmp(pairs[2].domain, "gamma") == 0, "domain[2] mismatch");
    ASSERT(pairs[2].port == PHP_TROCHILIDAE_SERVER_DEFAULT_PORT, "port[2] default");
    free(pairs);
    PASS();
}

static void test_mixed_with_and_without_port() {
    TEST("mixed with and without ports");
    char input[] = "secured:443,plain,admin:8080";
    int numPairs = 0;
    DomainPortEntry *pairs = parse_domain_port_pairs(input, &numPairs);
    ASSERT(numPairs == 3, "expected 3 pairs");
    ASSERT(strcmp(pairs[0].domain, "secured") == 0, "domain[0] mismatch");
    ASSERT(pairs[0].port == 443, "port[0] should be 443");
    ASSERT(strcmp(pairs[1].domain, "plain") == 0, "domain[1] mismatch");
    ASSERT(pairs[1].port == PHP_TROCHILIDAE_SERVER_DEFAULT_PORT, "port[1] default");
    ASSERT(strcmp(pairs[2].domain, "admin") == 0, "domain[2] mismatch");
    ASSERT(pairs[2].port == 8080, "port[2] should be 8080");
    free(pairs);
    PASS();
}

static void test_invalid_port_falls_back_to_default() {
    TEST("invalid port falls back to default");
    char input[] = "host:notaport";
    int numPairs = 0;
    DomainPortEntry *pairs = parse_domain_port_pairs(input, &numPairs);
    ASSERT(numPairs == 1, "expected 1 pair");
    ASSERT(strcmp(pairs[0].domain, "host") == 0, "domain mismatch");
    ASSERT(pairs[0].port == PHP_TROCHILIDAE_SERVER_DEFAULT_PORT, "port should be default");
    free(pairs);
    PASS();
}

static void test_empty_port_value() {
    TEST("domain with colon but no port (empty port)");
    char input[] = "host:";
    int numPairs = 0;
    DomainPortEntry *pairs = parse_domain_port_pairs(input, &numPairs);
    ASSERT(numPairs == 1, "expected 1 pair");
    ASSERT(strcmp(pairs[0].domain, "host") == 0, "domain mismatch");
    ASSERT(pairs[0].port == PHP_TROCHILIDAE_SERVER_DEFAULT_PORT, "port should be default");
    free(pairs);
    PASS();
}

int main() {
    printf("Running extended unit tests for parse_domain_port_pairs...\n\n");

    test_normal_case();
    test_single_domain_without_port();
    test_single_domain_with_port();
    test_empty_string();
    test_multiple_without_ports();
    test_mixed_with_and_without_port();
    test_invalid_port_falls_back_to_default();
    test_empty_port_value();
    test_null_input();
    test_null_numpairs();

    printf("\n=== Results: %d passed, %d failed ===\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
