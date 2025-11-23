/* Copyright (c) 2020-2021 tevador <tevador@gmail.com> */
/* See LICENSE for licensing information */

#include "polyseed.h"
#include "storage.h"
#include "birthday.h"
#include "features.h"
#include "gf.h"

#include <stdint.h>
#include <string.h>

#define STORAGE_HEADER "POLYSEED"
#define HEADER_SIZE 8
#define EXTRA_BYTE 0xFF
#define STORAGE_FOOTER 0x7000

static inline void store16(uint8_t* p, uint16_t u) {
    *p++ = (uint8_t)u;
    u >>= 8;
    *p++ = (uint8_t)u;
}

static inline uint16_t load16(const uint8_t* p) {
    uint16_t val = *p++;
    val |= ((uint16_t)*p) << 8;
    return val;
}

void polyseed_data_store(const polyseed_data* data, polyseed_storage storage) {
    uint8_t* pos = storage;
    memcpy(pos, STORAGE_HEADER, HEADER_SIZE);
    pos += HEADER_SIZE;
    store16(pos, data->features << DATE_BITS | data->birthday);
    pos += 2;
    memcpy(pos, data->secret, SECRET_SIZE);
    pos += SECRET_SIZE;
    pos[0] = EXTRA_BYTE;
    ++pos;
    store16(pos, STORAGE_FOOTER | data->checksum);
}

polyseed_status polyseed_data_load(const polyseed_storage storage,
    polyseed_data* data) {

    const uint8_t* pos = storage;
    if (0 != memcmp(pos, STORAGE_HEADER, HEADER_SIZE)) {
        /* Wrong header */
        return POLYSEED_ERR_FORMAT;
    }
    pos += HEADER_SIZE;
    uint16_t v1 = load16(pos);
    data->birthday = v1 & DATE_MASK;
    v1 >>= DATE_BITS;
    if (v1 > FEATURE_MASK) {
        /* Top bit of v1 was not zero */
        return POLYSEED_ERR_FORMAT;
    }
    data->features = v1;
    pos += 2;
    memset(data->secret, 0, sizeof(data->secret));
    memcpy(data->secret, pos, SECRET_SIZE);
    if (pos[SECRET_SIZE - 1] & ~CLEAR_MASK) {
        /* Secret is more than SECRET_BITS long */
        return POLYSEED_ERR_FORMAT;
    }
    pos += SECRET_SIZE;
    if (pos[0] != EXTRA_BYTE) {
        /* Wrong extra byte */
        return POLYSEED_ERR_FORMAT;
    }
    ++pos;
    uint16_t v2 = load16(pos);
    data->checksum = v2 & GF_MASK;
    v2 &= ~GF_MASK;
    if (v2 != STORAGE_FOOTER) {
        /* Wrong footer */
        return POLYSEED_ERR_FORMAT;
    }
    return POLYSEED_OK;
}
