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
        tr_timer.h
        php_trochilidae.h
        tr_network.c
        tr_network.h
        utils.h
        utils.c
        trochilidae.c
        tr_timer.c
        trochilidae_arginfo.h)
