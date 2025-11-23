/* Copyright (c) 2020-2021 tevador <tevador@gmail.com> */
/* See LICENSE for licensing information */

#include "gf.h"
#include "storage.h"
#include "birthday.h"
#include "features.h"
#include "dependency.h"

#include <assert.h>
#include <limits.h>
#include <string.h>

POLYSEED_PRIVATE gf_elem polyseed_mul2_table[8] = {
    5, 7, 1, 3, 13, 15, 9, 11
};

#define SHARE_BITS 10 /* bits of the secret per word */
#define DATA_WORDS POLYSEED_NUM_WORDS - POLY_NUM_CHECK_DIGITS

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

void polyseed_data_to_poly(const polyseed_data* data, gf_poly* poly) {

    unsigned extra_val = (data->features << DATE_BITS) | data->birthday;
    unsigned extra_bits = FEATURE_BITS + DATE_BITS;

    unsigned word_bits = 0;
    unsigned word_val = 0;

    unsigned secret_idx = 0;
    unsigned secret_val = data->secret[secret_idx];
    unsigned secret_bits = CHAR_BIT;
    unsigned seed_rem_bits = SECRET_BITS - CHAR_BIT;

    for (int i = 0; i < DATA_WORDS; ++i) {
        while (word_bits < SHARE_BITS) {
            if (secret_bits == 0) {
                secret_idx++;
                secret_bits = MIN(seed_rem_bits, CHAR_BIT);
                secret_val = data->secret[secret_idx];
                seed_rem_bits -= secret_bits;
            }
            unsigned chunk_bits = MIN(secret_bits, SHARE_BITS - word_bits);
            secret_bits -= chunk_bits;
            word_bits += chunk_bits;
            word_val <<= chunk_bits;
            word_val |= (secret_val >> secret_bits) & ((1u << chunk_bits) - 1);
        }
        word_val <<= 1;
        extra_bits--;
        word_val |= (extra_val >> extra_bits) & 1;
        poly->coeff[POLY_NUM_CHECK_DIGITS + i] = word_val;
        word_val = 0;
        word_bits = 0;
    }

    assert(seed_rem_bits == 0);
    assert(secret_bits == 0);
    assert(extra_bits == 0);
}

void polyseed_poly_to_data(const gf_poly* poly, polyseed_data* data) {
    data->birthday = 0;
    data->features = 0;
    memset(data->secret, 0, sizeof(data->secret));
    data->checksum = poly->coeff[0];

    unsigned extra_val = 0;
    unsigned extra_bits = 0;

    unsigned word_bits = 0;
    unsigned word_val = 0;

    unsigned secret_idx = 0;
    unsigned secret_bits = 0;
    unsigned seed_bits = 0;

    for (int i = POLY_NUM_CHECK_DIGITS; i < POLYSEED_NUM_WORDS; ++i) {
        word_val = poly->coeff[i];

        extra_val <<= 1;
        extra_val |= word_val & 1;
        word_val >>= 1;
        word_bits = GF_BITS - 1;
        extra_bits++;

        while (word_bits > 0) {
            if (secret_bits == CHAR_BIT) {
                secret_idx++;
                seed_bits += secret_bits;
                secret_bits = 0;
            }
            unsigned chunk_bits = MIN(word_bits, CHAR_BIT - secret_bits);
            word_bits -= chunk_bits;
            unsigned chunk_mask = ((1u << chunk_bits) - 1);
            if (chunk_bits < CHAR_BIT) {
                data->secret[secret_idx] <<= chunk_bits;
            }
            data->secret[secret_idx] |= (word_val >> word_bits) & chunk_mask;
            secret_bits += chunk_bits;
        }
    }

    seed_bits += secret_bits;

    assert(word_bits == 0);
    assert(seed_bits == SECRET_BITS);
    assert(extra_bits == FEATURE_BITS + DATE_BITS);

    data->birthday = extra_val & DATE_MASK;
    data->features = extra_val >> DATE_BITS;
}
