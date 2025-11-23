/* Copyright (c) 2020-2021 tevador <tevador@gmail.com> */
/* See LICENSE for licensing information */

#ifndef STORAGE_H
#define STORAGE_H

#include "gf.h"

#include <limits.h>

#define SECRET_BUFFER_SIZE 32
#define SECRET_BITS 150
#define SECRET_SIZE (SECRET_BITS + CHAR_BIT - 1) / CHAR_BIT /* 19 */
#define CLEAR_BITS (SECRET_SIZE) * (CHAR_BIT) - (SECRET_BITS) /* 2 */
#define CLEAR_MASK ~(uint8_t)(((1u << (CLEAR_BITS)) - 1) << (CHAR_BIT - (CLEAR_BITS)))
#define TOTAL_BITS GF_BITS * POLYSEED_NUM_WORDS

/* Seed data structure for serialization */
typedef struct polyseed_data {
    unsigned birthday;
    unsigned features;
    /* padded with zeroes for future compatibility with longer seeds */
    uint8_t secret[SECRET_BUFFER_SIZE];
    gf_elem checksum;
} polyseed_data;

POLYSEED_PRIVATE
void polyseed_data_store(const polyseed_data* data, polyseed_storage storage);

POLYSEED_PRIVATE
polyseed_status polyseed_data_load(const polyseed_storage storage,
    polyseed_data* data);

#endif
