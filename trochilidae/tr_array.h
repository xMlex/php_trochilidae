//
// Created by mlex on 09.12.2024.
//

#ifndef TR_ARRAY_H
#define TR_ARRAY_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "trochilidae/utils.h"

#define DEFAULT_CAPACITY 5194910

/**
 * @struct tr_array
 * @details A structure that represents an array of bytes.
 * @field size The current size of the array.
 * @field capacity The maximum capacity of the array.
 * @field data A pointer to the array of bytes.
 */
struct tr_array {
    size_t position;
    size_t size;
    size_t init_capacity;
    size_t capacity;
    byte *data;
};

#define CHECK_ALLOC(ptr)                    \
    do {                                    \
        if (!(ptr)) {                       \
          fprintf(stderr, "Memory allocation failed\n"); \
          exit(EXIT_FAILURE);               \
        }                                   \
    } while (0)

extern void tr_array_init(struct tr_array *self, size_t capacity);
extern void tr_array_clear(struct tr_array *self);
extern void tr_array_free(const struct tr_array *self);
extern size_t tr_array_get_position(const struct tr_array *self);
extern void tr_array_set_position(struct tr_array *self, size_t position);
extern size_t tr_array_get_size(const struct tr_array *self);
extern void tr_array_ensure_capacity(struct tr_array *self, size_t additional_size);
extern void tr_array_write_data(struct tr_array *self, const void *data, size_t data_size);
extern void tr_array_write_data_at_pos(struct tr_array *self, size_t pos, const void *data, size_t data_size);
/**
 * @brief Writes a null-terminated string to the array.
 *
 * This function takes a pointer to a tr_array structure and a
 * null-terminated string, then writes the string into the array.
 * The function manages the memory allocation necessary to store
 * the string within the array.
 *
 * @param self A pointer to the tr_array structure.
 * @param string A constant character pointer to the null-terminated
 *               string that needs to be written to the array.
 */
extern void tr_array_write_string(struct tr_array *self, const char *string);
extern void tr_array_write_string_size(struct tr_array *self, const char *string, const size_t string_size);
extern void tr_array_write_byte(struct tr_array *self, const void *c);
/**
 * @brief Write a short value(2 bytes) to the array
 */
extern void tr_array_write_short(struct tr_array *self, const void *c);
/**
 * @brief Write a word value (4 bytes) to the array
 */
extern void tr_array_write_word(struct tr_array *self, const void *c);
/**
 * @brief Write a long value (8 bytes) to the array
 */
extern void tr_array_write_long(struct tr_array *self, const void *c);
extern void tr_array_write_tv(struct tr_array *self, struct timeval *tv);

#endif //TR_ARRAY_H
