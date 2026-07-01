//
// Created by mlex on 09.12.2024.
//

#include "php.h"
#include "tr_array.h"


void tr_array_init(struct tr_array *self, size_t capacity) {
    if (!self) {
        return;
    }
    if (capacity == 0) {
        capacity = DEFAULT_CAPACITY;
    }

    if (capacity > SIZE_MAX / sizeof(byte)) {
        fprintf(stderr, "tr_array_init: capacity too large\n");
        exit(EXIT_FAILURE);
    }
    self->data = (byte *)emalloc(sizeof(byte) * capacity);
    CHECK_ALLOC(self->data);
    self->init_capacity = capacity;
    self->capacity = capacity;
    tr_array_clear(self);
}

size_t tr_array_get_size(const struct tr_array *self) {
    return self->size;
}

size_t tr_array_get_position(const struct tr_array *self) {
    return self->position;
}

void tr_array_set_position(struct tr_array *self, size_t position) {
  self->position = position;
}

void tr_array_clear(struct tr_array *self) {
    self->size = 0;
    self->position = 0;
}

void tr_array_free(struct tr_array *self) {
    efree(self->data);
    self->data = NULL;
    self->size = 0;
    self->position = 0;
    self->capacity = 0;
    self->init_capacity = 0;
}

void tr_array_ensure_capacity(struct tr_array *self, const size_t additional_size) {
    if (additional_size == 0) return;

    if (self->position > SIZE_MAX - additional_size) {
        fprintf(stderr, "tr_array_ensure_capacity: position + additional_size overflow\n");
        exit(EXIT_FAILURE);
    }
    const size_t required_capacity = self->position + additional_size;
    if (required_capacity <= self->capacity) {
        return;
    }
    size_t new_capacity = self->capacity * 2;
    if (new_capacity < required_capacity) {
        new_capacity = required_capacity;
    }
    if (new_capacity < self->capacity) {
        fprintf(stderr, "tr_array_ensure_capacity: new_capacity overflow\n");
        exit(EXIT_FAILURE);
    }

    if (new_capacity > SIZE_MAX / sizeof(byte)) {
        fprintf(stderr, "tr_array_ensure_capacity: allocation size overflow\n");
        exit(EXIT_FAILURE);
    }
    byte *new_data = (byte *)erealloc(self->data, sizeof(byte) * new_capacity);
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
    if (string == NULL) {
        tr_array_write_string_size(self, NULL, 0);
        return;
    }
    tr_array_write_string_size(self, string, strlen(string));
}

void tr_array_write_tv(struct tr_array *self, struct timeval *tv) {
     tr_array_write_word(self, &tv->tv_sec);
     tr_array_write_word(self, &tv->tv_usec);
}

