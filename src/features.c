/* Copyright (c) 2020-2021 tevador <tevador@gmail.com> */
/* See LICENSE for licensing information */

#include "polyseed.h"
#include "features.h"

static unsigned reserved_features = FEATURE_MASK ^ ENCRYPTED_MASK;

POLYSEED_PRIVATE bool polyseed_features_supported(unsigned features) {
    return (features & reserved_features) == 0;
}

int polyseed_enable_features(unsigned mask) {
    int num_enabled = 0;
    reserved_features = FEATURE_MASK ^ ENCRYPTED_MASK;
    for (int i = 0; i < USER_FEATURES; ++i) {
        unsigned fmask = 1u << i;
        if (mask & fmask) {
            reserved_features ^= fmask;
            num_enabled++;
        }
    }
    return num_enabled;
}
