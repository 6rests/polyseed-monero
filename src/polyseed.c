/* Copyright (c) 2020-2021 tevador <tevador@gmail.com> */
/* See LICENSE for licensing information */

#include "polyseed.h"
#include "dependency.h"
#include "birthday.h"
#include "features.h"
#include "lang.h"
#include "gf.h"
#include "storage.h"

#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

#define KDF_NUM_ITERATIONS 10000

static void write_str(char** pos, const char* str) {
    char* loc = *pos;
    while (*str != '\0') {
        *loc = *str;
        ++str;
        ++loc;
    }
    *pos = loc;
}

static int str_split(char* str, polyseed_phrase words) {
    char* pos = str;
    char* word = str;
    int w = 0;

    /* split on space */
    while (*pos != '\0') {
        while (*pos != '\0' && *pos != ' ') {
            ++pos;
        }
        words[w] = word;
        if (*pos != '\0') {
            *pos = '\0';
            ++pos;
        }
        word = pos;
        ++w;
        if (w == POLYSEED_NUM_WORDS) {
            if (*pos != '\0') {
                ++w; /* too many words */
            }
            break;
        }
    }
    return w;
}

polyseed_status polyseed_create(unsigned features, polyseed_data** seed_out) {
    CHECK_DEPS();

    /* check features */
    unsigned seed_features = make_features(features);
    if (!polyseed_features_supported(seed_features)) {
        return POLYSEED_ERR_UNSUPPORTED;
    }

    /* alocate memory */
    polyseed_data* seed = ALLOC(sizeof(polyseed_data));
    if (seed == NULL) {
        return POLYSEED_ERR_MEMORY;
    }

    /* create seed */
    seed->birthday = birthday_encode(GET_TIME());
    seed->features = seed_features;
    memset(seed->secret, 0, sizeof(seed->secret));
    GET_RANDOM_BYTES(seed->secret, SECRET_SIZE);
    seed->secret[SECRET_SIZE - 1] &= CLEAR_MASK;

    /* encode polynomial */
    gf_poly poly = { 0 };
    polyseed_data_to_poly(seed, &poly);

    /* calculate checksum */
    gf_poly_encode(&poly);
    seed->checksum = poly.coeff[0];

    MEMZERO_LOC(poly);

    *seed_out = seed;
    return POLYSEED_OK;
}

void polyseed_free(polyseed_data* seed) {
    if (seed != NULL) {
        MEMZERO_PTR(seed, polyseed_data);
        FREE(seed);
    }
}

uint64_t polyseed_get_birthday(const polyseed_data* data) {
    assert(data != NULL);
    return birthday_decode(data->birthday);
}

unsigned polyseed_get_feature(const polyseed_data* seed, unsigned mask) {
    assert(seed != NULL);
    return get_features(seed->features, mask);
}

size_t polyseed_encode(const polyseed_data* data, const polyseed_lang* lang,
    polyseed_coin coin, polyseed_str str_out) {

    assert(data != NULL);
    assert(lang != NULL);
    assert((gf_elem)coin < GF_SIZE);
    assert(str_out != NULL);
    CHECK_DEPS();

    /* encode polynomial with the existing checksum */
    gf_poly poly = { 0 };
    poly.coeff[0] = data->checksum;
    polyseed_data_to_poly(data, &poly);

    /* apply coin */
    poly.coeff[POLY_NUM_CHECK_DIGITS] ^= coin;

    polyseed_str str_tmp;
    char* pos = str_tmp;
    int w;
    size_t str_size;

#define WORD(i) lang->words[poly.coeff[i]]

    /* output words */
    for (w = 0; w < POLYSEED_NUM_WORDS - 1; ++w) {
        write_str(&pos, WORD(w));
        write_str(&pos, lang->separator);
    }
    write_str(&pos, WORD(w));
    *pos = '\0';
    str_size = pos - str_tmp;
    assert(str_size < POLYSEED_STR_SIZE);

#undef WORD

    /* compose if needed by the language */
    if (lang->compose) {
        str_size = UTF8_COMPOSE(str_tmp, str_out);
        assert(str_size < POLYSEED_STR_SIZE);
    }
    else {
        memcpy(str_out, str_tmp, str_size + 1);
    }

    MEMZERO_LOC(poly);
    MEMZERO_LOC(str_tmp);

    return str_size;
}

polyseed_status polyseed_decode(const char* str, polyseed_coin coin,
    const polyseed_lang** lang_out, polyseed_data** seed_out) {

    assert(str != NULL);
    assert((gf_elem)coin < GF_SIZE);
    assert(seed_out != NULL);
    CHECK_DEPS();

    polyseed_str str_tmp;
    polyseed_phrase words;
    gf_poly poly = { 0 };
    polyseed_status res;
    polyseed_data* seed;

    /* canonical decomposition */
    size_t str_size = UTF8_DECOMPOSE(str, str_tmp);
    assert(str_size < POLYSEED_STR_SIZE);

    /* split into words */
    if (str_split(str_tmp, words) != POLYSEED_NUM_WORDS) {
        res = POLYSEED_ERR_NUM_WORDS;
        goto cleanup;
    }

    /* decode words into polynomial coefficients */
    res = polyseed_phrase_decode(words, poly.coeff, lang_out);

    if (res != POLYSEED_OK) {
        goto cleanup;
    }

    /* finalize polynomial */
    poly.coeff[POLY_NUM_CHECK_DIGITS] ^= coin;

    /* checksum */
    if (!gf_poly_check(&poly)) {
        res = POLYSEED_ERR_CHECKSUM;
        goto cleanup;
    }

    /* alocate memory */
    seed = ALLOC(sizeof(polyseed_data));

    if (seed == NULL) {
        res = POLYSEED_ERR_MEMORY;
        goto cleanup;
    }

    /* decode polynomial into seed data */
    polyseed_poly_to_data(&poly, seed);

    /* check features */
    if (!polyseed_features_supported(seed->features)) {
        polyseed_free(seed);
        res = POLYSEED_ERR_UNSUPPORTED;
        goto cleanup;
    }

    *seed_out = seed;
    res = POLYSEED_OK;

cleanup:
    MEMZERO_LOC(str_tmp);
    MEMZERO_LOC(words);
    MEMZERO_LOC(poly);
    return res;
}

polyseed_status polyseed_decode_explicit(const char* str, polyseed_coin coin,
    const polyseed_lang* lang, polyseed_data** seed_out) {

    assert(str != NULL);
    assert((gf_elem)coin < GF_SIZE);
    assert(lang != NULL);
    assert(seed_out != NULL);
    CHECK_DEPS();

    polyseed_str str_tmp;
    polyseed_phrase words;
    gf_poly poly = { 0 };
    polyseed_status res;
    polyseed_data* seed;

    /* canonical decomposition */
    size_t str_size = UTF8_DECOMPOSE(str, str_tmp);
    assert(str_size < POLYSEED_STR_SIZE);

    /* split into words */
    if (str_split(str_tmp, words) != POLYSEED_NUM_WORDS) {
        res = POLYSEED_ERR_NUM_WORDS;
        goto cleanup;
    }

    /* decode words into polynomial coefficients */
    res = polyseed_phrase_decode_explicit(words, lang, poly.coeff);

    if (res != POLYSEED_OK) {
        goto cleanup;
    }

    /* finalize polynomial */
    poly.coeff[POLY_NUM_CHECK_DIGITS] ^= coin;

    /* checksum */
    if (!gf_poly_check(&poly)) {
        res = POLYSEED_ERR_CHECKSUM;
        goto cleanup;
    }

    /* alocate memory */
    seed = ALLOC(sizeof(polyseed_data));

    if (seed == NULL) {
        res = POLYSEED_ERR_MEMORY;
        goto cleanup;
    }

    /* decode polynomial into seed data */
    polyseed_poly_to_data(&poly, seed);

    /* check features */
    if (!polyseed_features_supported(seed->features)) {
        polyseed_free(seed);
        res = POLYSEED_ERR_UNSUPPORTED;
        goto cleanup;
    }

    *seed_out = seed;
    res = POLYSEED_OK;

cleanup:
    MEMZERO_LOC(str_tmp);
    MEMZERO_LOC(words);
    MEMZERO_LOC(poly);
    return res;
}

static inline void store32(uint8_t* p, uint32_t u) {
    *p++ = (uint8_t)u;
    u >>= 8;
    *p++ = (uint8_t)u;
    u >>= 8;
    *p++ = (uint8_t)u;
    u >>= 8;
    *p++ = (uint8_t)u;
}

void polyseed_keygen(const polyseed_data* seed, polyseed_coin coin,
    size_t key_size, uint8_t* key_out) {

    assert(seed != NULL);
    assert((gf_elem)coin < GF_SIZE);
    assert(key_out != NULL);
    CHECK_DEPS();

    uint8_t salt[32] = "POLYSEED key";
    salt[13] = 0xff;
    salt[14] = 0xff;
    salt[15] = 0xff;
    store32(&salt[16], coin);           /* domain separate by coin */
    store32(&salt[20], seed->birthday); /* domain separate by birthday */
    store32(&salt[24], seed->features); /* domain separate by features */
    
    PBKDF2_SHA256(seed->secret, SECRET_BUFFER_SIZE, salt, sizeof(salt),
        KDF_NUM_ITERATIONS, key_out, key_size);
}

void polyseed_store(const polyseed_data* seed, polyseed_storage storage) {
    assert(seed != NULL);
    assert(storage != NULL);

    polyseed_data_store(seed, storage);
}

polyseed_status polyseed_load(const polyseed_storage storage,
    polyseed_data** seed_out) {

    assert(storage != NULL);
    assert(seed_out != NULL);

    gf_poly poly = { 0 };
    polyseed_status res;
    polyseed_data* seed;

    /* alocate memory */
    seed = ALLOC(sizeof(polyseed_data));

    if (seed == NULL) {
        return POLYSEED_ERR_MEMORY;
    }

    /* deserialize data */
    res = polyseed_data_load(storage, seed);
    if (res != POLYSEED_OK) {
        polyseed_free(seed);
        return res;
    }

    /* encode polynomial with the existing checksum */
    poly.coeff[0] = seed->checksum;
    polyseed_data_to_poly(seed, &poly);

    /* checksum */
    if (!gf_poly_check(&poly)) {
        polyseed_free(seed);
        res = POLYSEED_ERR_CHECKSUM;
        goto cleanup;
    }

    /* check features */
    if (!polyseed_features_supported(seed->features)) {
        polyseed_free(seed);
        res = POLYSEED_ERR_UNSUPPORTED;
        goto cleanup;
    }

    res = POLYSEED_OK;
    *seed_out = seed;

cleanup:
    MEMZERO_LOC(poly);
    return res;
}

void polyseed_crypt(polyseed_data* seed, const char* password) {
    assert(seed != NULL);
    assert(password != NULL);

    polyseed_str pass_norm;

    /* normalize password */
    size_t str_size = UTF8_DECOMPOSE(password, pass_norm);
    assert(str_size < POLYSEED_STR_SIZE);

    /* derive an encryption mask */
    uint8_t mask[32];

    char salt[16] = "POLYSEED mask";
    salt[14] = 0xff;
    salt[15] = 0xff;

    PBKDF2_SHA256(pass_norm, str_size, salt, sizeof(salt),
        KDF_NUM_ITERATIONS, mask, sizeof(mask));

    /* apply mask */
    for (int i = 0; i < SECRET_SIZE; ++i) {
        seed->secret[i] ^= mask[i];
    }
    seed->secret[SECRET_SIZE - 1] &= CLEAR_MASK;

    seed->features ^= ENCRYPTED_MASK;

    gf_poly poly = { 0 };

    /* encode polynomial */
    polyseed_data_to_poly(seed, &poly);

    /* calculate new checksum */
    gf_poly_encode(&poly);

    seed->checksum = poly.coeff[0];

    MEMZERO_LOC(poly);
    MEMZERO_LOC(mask);
    MEMZERO_LOC(pass_norm);
}

int polyseed_is_encrypted(const polyseed_data* seed) {
    assert(seed != NULL);
    return is_encrypted(seed->features) ? 1 : 0;
}
