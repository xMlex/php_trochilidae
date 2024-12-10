cmake_minimum_required(VERSION 3.8)
project(php_trochilidae C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

execute_process (
        COMMAND php-config --include-dir
        OUTPUT_VARIABLE PHP_SOURCE
)
string(REGEX REPLACE "\n$" "" PHP_SOURCE "${PHP_SOURCE}")

message("Using source directory: ${PHP_SOURCE}")

include_directories(${PHP_SOURCE})
include_directories(${PHP_SOURCE}/main)
include_directories(${PHP_SOURCE}/Zend)
include_directories(${PHP_SOURCE}/TSRM)
include_directories(${PROJECT_SOURCE_DIR})


add_executable(php_trochilidae
        config.h
        php_trochilidae.h
        trochilidae/tr_network.c
        trochilidae/tr_network.h
        trochilidae/utils.h
        trochilidae/utils.c
        trochilidae/tr_timer.h
        trochilidae/tr_timer.c
        trochilidae/tr_array.h
        trochilidae/tr_array.c
        trochilidae.c
        trochilidae_arginfo.h
        trochilidae/tr_hooks.c
        trochilidae/tr_hooks.h
)
