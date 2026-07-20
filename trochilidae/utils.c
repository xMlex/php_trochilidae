//
// Created by mlex on 22.11.2022.
//

#include "trochilidae/utils.h"
#include <inttypes.h>
#include <unistd.h>

extern void d2tv(double x, struct timeval *tv) {
    tv->tv_sec = (long) x;
    tv->tv_usec = (x - (double) tv->tv_sec) * 1000000.0 + 0.5;
}


int str_to_int_with_default(const char *str, int default_value) {
    char *endPtr;
    long int result = strtol(str, &endPtr, 10);

    if (endPtr == str || *endPtr != '\0') {
        return default_value;
    }
    return (int)result;
}

uint64_t generate_random_ulong() {
#ifdef HAVE_ARC4RANDOM
    return ((uint64_t)arc4random() << 32) | arc4random();
#else
    static bool prng_seeded = false;
    if (!prng_seeded) {
        srand((unsigned int)(time(NULL) ^ getpid()));
        prng_seeded = true;
    }
    return ((uint64_t)time(NULL) << 32) | (unsigned long)rand();
#endif
}
