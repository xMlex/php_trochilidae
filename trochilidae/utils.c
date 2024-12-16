//
// Created by mlex on 22.11.2022.
//

#include "trochilidae/utils.h"

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

unsigned long generate_random_ulong() {
#ifdef HAVE_ARC4RANDOM
    //fprintf(stderr, "generate_random_ulong used: arc4random\n");
    return ((unsigned long)arc4random() << 32) | arc4random();
#else
    //fprintf(stderr, "generate_random_ulong used: rand + time\n");
    return ((unsigned long)time(NULL) << 32) | rand();
#endif

}
