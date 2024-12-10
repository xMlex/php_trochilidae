//
// Created by mlex on 09.12.2024.
//

#include "tr_array.h"


void tr_array_init(struct tr_array *self, size_t capacity) {
    if (!self) {
        return;
    }
    if (capacity == 0) {
        capacity = DEFAULT_CAPACITY;
    }

    self->init_capacity = capacity;
    self->capacity = capacity;
    self->data = malloc(sizeof(byte) * capacity);
    tr_array_clear(self);
    //fprintf(stderr, "tr_array_init: %zu\n", sizeof(byte) * capacity);
    CHECK_ALLOC(self->data);
}

size_t tr_array_get_size(const struct tr_array *self) {
    fprintf(stderr, "tr_array_get_size: %zu capacity: %zu position: %zu\n", self->size, self->capacity, self->position);
    return self->size;
}

size_t tr_array_get_position(const struct tr_array *self) {
    return self->position;
}

void tr_array_set_position(struct tr_array *self, size_t position) {
  self->position = position;
}

void tr_array_clear(struct tr_array *self) {
    if (self->capacity > self->init_capacity) {
        free(self->data);
        self->capacity = self->init_capacity;
        self->data = (byte *)malloc(sizeof(byte) * self->capacity);
        CHECK_ALLOC(self->data);
    } else {
        memset(self->data, 0, self->capacity);
    }

    self->size = 0;
    self->position = 0;
}

void tr_array_free(const struct tr_array *self) {
    free(self->data);
}

void tr_array_ensure_capacity(struct tr_array *self, const size_t additional_size) {
    if (additional_size == 0) return;

    const size_t required_capacity = self->position + additional_size;
    if (required_capacity < self->capacity) {
        return;
    }
    size_t new_capacity = self->capacity * 2;
    if (new_capacity < required_capacity) {
        new_capacity = required_capacity;
    }
    //fprintf(stderr, "tr_array_ensure_capacity: %zu\n", new_capacity);

    byte *new_data = realloc(self->data, sizeof(byte) * new_capacity);
    CHECK_ALLOC(new_data);

    self->data = new_data;
    self->capacity = new_capacity;
}

void tr_array_write_data_at_pos(struct tr_array *self, size_t pos, const void *data, size_t data_size) {
    tr_array_ensure_capacity(self, data_size);
    memcpy(self->data + pos, data, data_size);
}

void tr_array_write_data(struct tr_array *self, const void *data, const size_t data_size) {
     tr_array_ensure_capacity(self, data_size);
     memcpy(&self->data[self->position], data, data_size);
     self->position += data_size;
     self->size += data_size;
 }

void tr_array_write_string_size(struct tr_array *self, const char *string, const size_t string_size) {
    const int nullByte = 0x00;
    if (string == NULL) {
        tr_array_write_word(self, &nullByte);
        return;
    }
    tr_array_write_word(self, &string_size);
    tr_array_write_data(self, string, string_size);
}

void tr_array_write_string(struct tr_array *self, const char *string) {
    tr_array_write_string_size(self, string, strlen(string));
}

// Запись одного байта
void tr_array_write_byte(struct tr_array *self, const void *c) {
     tr_array_write_data(self, c, 1);
 }

// Запись short (2 байта)
void tr_array_write_short(struct tr_array *self, const void *c) {
     tr_array_write_data(self, c, 2);
 }

// Запись word (4 байта)
void tr_array_write_word(struct tr_array *self, const void *c) {
     tr_array_write_data(self, c, 4);
 }

// Запись long (8 байт)
void tr_array_write_long(struct tr_array *self, const void *c) {
     tr_array_write_data(self, c, 8);
 }

void tr_array_write_tv(struct tr_array *self, struct timeval *tv) {
     tr_array_write_word(self, &tv->tv_sec);
     tr_array_write_word(self, &tv->tv_usec);
}

