/* Copyright (c) 2020-2021 tevador <tevador@gmail.com> */
/* See LICENSE for licensing information */

#ifdef NDEBUG
#undef NDEBUG 
#endif

#include <polyseed.h>

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

typedef bool test_func(void);
typedef void multitest_func(void);

static int g_test_no = 0;

#define SEED_TIME1 ((uint64_t)1638446400) /* Dec 2021 */
#define SEED_TIME2 ((uint64_t)3118651200) /* Oct 2068 */
#define SEED_TIME3 ((uint64_t)4305268800) /* Jun 2106 */

#define FEATURE_FOO 1
#define FEATURE_BAR 2
#define FEATURE_QUX 4

static int g_num_langs;
static int g_num_allocs;

static polyseed_data* g_seed1;
static polyseed_data* g_seed2;
static polyseed_data* g_seed3;

static polyseed_storage g_store1;
static polyseed_storage g_store2;
static polyseed_storage g_store3;

static const char* g_phrase_en1 =
    "raven tail swear infant grief assist regular lamp "
    "duck valid someone little harsh puppy airport language";
static const char* g_phrase_en2 =
    "rave tail swea infan grie assi regul lamp "
    "duck vali some litt hars pupp airp langua";
static const char* g_phrase_en3 =
    "ravexxx tail swea infan grie assi regul lamp "
    "duck vali some litt hars pupp airp langua";
static const char* g_phrase_en4 =
    "raven tail swear infantile grief assist regular lamp "
    "duck valid someone little harsh puppy airport language";
static const char* g_phrase_en5 =
    "raven tail swear infant grief assist regular lamp "
    "duck valid someone little harsh puppy airport language ";

static const char* g_phrase_es1 =
    u8"eje fin parte célebre tabú pestaña lienzo puma "
    u8"prisión hora regalo lengua existir lápiz lote sonoro";
static const char* g_phrase_es2 =
    "eje fin parte celebre tabu pestana lienzo puma "
    "prision hora regalo lengua existir lapiz lote sonoro";
static const char* g_phrase_es3 =
    "eje fin part cele tabu pest lien puma "
    "pris hora rega leng exis lapi lote sono";
static const char* g_phrase_es4 =
    "ejexxx fin part cele tabu pest lien puma "
    "pris hora rega leng exis lapi lote sono";
static const char* g_phrase_es5 =
    "eje fin part cele tab pest lien puma "
    "pris hora rega leng exis lapi lote sono";

static const char* g_phrase_es_mult =
    "impo sort usua cabi venu nobl oliv clim "
    "cont barr marc auto prod vaca torn fati";

static const char* g_phrase_garbage1 = "xxx xxx";

static const char* g_phrase_garbage2 =
"xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx "
"xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx "
"xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx "
"xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx "
"xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx "
"xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx "
"xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx "
"xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx xxx ";

static const char* g_test_pass = "password";

static const uint8_t g_test_mask[] = {
    0x54, 0x4a, 0x88, 0x95, 0xff, 0xc0, 0x45, 0x1c,
    0x9b, 0x8e, 0x28, 0x1e, 0x18, 0x2d, 0x0d, 0x73,
    0x63, 0x7d, 0x1b, 0xd7, 0xcb, 0x6e, 0xed, 0x8f,
    0x84, 0x35, 0xb3, 0x13, 0x8c, 0x0c, 0xf0, 0x4e,
};

static polyseed_str g_phrase_out;

static const polyseed_lang* g_lang_en;
static const polyseed_lang* g_lang_es;

#define RUN_TEST(x) run_test(#x, &x)
#define RUN_MULT(x) run_multitest(#x, &x)

static void run_test(const char* name, test_func* func) {
    printf("[%2i] %-40s ... ", ++g_test_no, name);
    printf(func() ? "PASSED\n" : "SKIPPED\n");
}

static void run_multitest(const char* name, multitest_func* func) {
    printf("[%2i] %s\n", ++g_test_no, name);
    func();
}

static void subtest_begin(const char* name) {
    printf("     %-40s ... ", name);
}

static void subtest_end(bool passed) {
    printf(passed ? "PASSED\n" : "SKIPPED\n");
}

static void output_hex(const char* data, size_t length) {
    for (unsigned i = 0; i < length; ++i)
        printf("%02x", data[i] & 0xff);
}

static inline char parse_nibble(char hex) {
    hex &= ~0x20;
    return (hex & 0x40) ? hex - ('A' - 10) : hex & 0xf;
}

static inline bool equals_hex(const void* bin, const char* hex) {
    size_t hexlen = strlen(hex);
    assert((hexlen % 2) == 0);
    const char* pbin = bin;
    for (int i = 0; i < hexlen; i += 2) {
        char nibble1 = parse_nibble(*hex++);
        char nibble2 = parse_nibble(*hex++);
        uint8_t val = (uint8_t)*pbin++;
        uint8_t ref = (uint8_t)nibble1 << 4 | nibble2;
        if (val != ref) {
            return false;
        }
    }
    return true;
}

static const polyseed_lang* get_lang(const char* name) {
    int count = polyseed_get_num_langs();
    for (int i = 0; i < count; ++i) {
        const polyseed_lang* lang = polyseed_get_lang(i);
        if (0 == strcmp(polyseed_get_lang_name_en(lang), name)) {
            return lang;
        }
    }
    return NULL;
}

#define TEST_KEYLEN 32

static const char rand_bytes1[] = {
    0xdd, 0x76, 0xe7, 0x35, 0x9a, 0x0d, 0xed, 0x37,
    0xcd, 0x0f, 0xf0, 0xf3, 0xc8, 0x29, 0xa5, 0xae,
    0x01, 0x67, 0xf3,
};

static void gen_rand_bytes1(void* result, size_t n) {
    assert(n == sizeof(rand_bytes1));
    memcpy(result, rand_bytes1, sizeof(rand_bytes1));
}

static const char rand_bytes2[] = {
    0x5a, 0x2b, 0x02, 0xdf, 0x7d, 0xb2, 0x1f, 0xcb,
    0xe6, 0xec, 0x6d, 0xf1, 0x37, 0xd5, 0x4c, 0x7b,
    0x20, 0xfd, 0x2b,
};

static void gen_rand_bytes2(void* result, size_t n) {
    assert(n == sizeof(rand_bytes2));
    memcpy(result, rand_bytes2, sizeof(rand_bytes2));
}

static const char rand_bytes3[] = {
    0x67, 0xb9, 0x36, 0xdf, 0xa4, 0xda, 0x6a, 0xe8,
    0xd3, 0xb3, 0xcd, 0xb3, 0xb9, 0x37, 0xf4, 0x02,
    0x7b, 0x0e, 0x3b,
};

static void gen_rand_bytes3(void* result, size_t n) {
    assert(n == sizeof(rand_bytes3));
    memcpy(result, rand_bytes3, sizeof(rand_bytes3));
}

static void pbkdf2_dummy1(const uint8_t* pw, size_t pwlen,
    const uint8_t* salt, size_t saltlen, uint64_t iterations,
    uint8_t* key, size_t keylen) {

    assert(equals_hex(pw,
        "dd76e7359a0ded37cd0ff0f3c829a5ae01673300000000000000000000000000"));
    assert(equals_hex(salt,
        "504f4c5953454544206b657900ffffff00000000010000000000000000000000"));
}

static void pbkdf2_dummy2(const uint8_t* pw, size_t pwlen,
    const uint8_t* salt, size_t saltlen, uint64_t iterations,
    uint8_t* key, size_t keylen) {

    assert(equals_hex(pw,
        "5a2b02df7db21fcbe6ec6df137d54c7b20fd2b00000000000000000000000000"));
    assert(equals_hex(salt,
        "504f4c5953454544206b657900ffffff00000000330200000000000000000000"));
}

static void pbkdf2_dummy3(const uint8_t* pw, size_t pwlen,
    const uint8_t* salt, size_t saltlen, uint64_t iterations,
    uint8_t* key, size_t keylen) {

    if (saltlen == 32) {
        assert(equals_hex(pw,
            "67b936dfa4da6ae8d3b3cdb3b937f4027b0e3b00000000000000000000000000"));
        assert(equals_hex(salt,
            "504f4c5953454544206b657900ffffff01000000f70300000100000000000000"));
    }
    else { /* encryption mask */
        assert(equals_hex(pw,
            "70617373776f7264"));
        assert(equals_hex(salt,
            "504f4c5953454544206d61736b00ffff"));
        assert(keylen == sizeof(g_test_mask));
        memcpy(key, g_test_mask, keylen);
    }
}

static size_t u8_nfc_donothing(const char* str, polyseed_str norm) {
    /* do not compose */
    strncpy(norm, str, POLYSEED_STR_SIZE);
    return POLYSEED_STR_SIZE - 1;
}

static size_t u8_nfkd_spaces(const char* str, polyseed_str norm) {
    /* normalize only ideographic spaces to allow Japanese phrases to roundtrip */
    int i = 0;
    for (; i < POLYSEED_STR_SIZE - 1 && *str != '\0'; ++i) {
        if (str[0] == '\xe3' && str[1] == '\x80' && str[2] == '\x80') {
            norm[i] = ' ';
            str += 3;
        }
        else {
            norm[i] = *str;
            ++str;
        }
    }
    norm[i] = '\0';
    return i;
}

static uint64_t time1() {
    return SEED_TIME1;
}

static uint64_t time2() {
    return SEED_TIME2;
}

static uint64_t time3() {
    return SEED_TIME3;
}

static void do_not_zero(void* const ptr, const size_t n) {
    /* do nothing */
}

static void* count_alloc(size_t n) {
    g_num_allocs++;
    return malloc(n);
}

static void count_free(void* ptr) {
    g_num_allocs--;
    free(ptr);
}

static void* alloc_fail(size_t n) {
    return NULL;
}

static void check_key(polyseed_data* data, polyseed_coin coin) {
    char key[TEST_KEYLEN];
    polyseed_keygen(data, coin, TEST_KEYLEN, key);
}

static void check_features(polyseed_data* seed, bool foo, bool bar, bool qux) {
    unsigned foo1 = polyseed_get_feature(seed, FEATURE_FOO);
    assert((foo1 != 0) == foo);
    unsigned bar1 = polyseed_get_feature(seed, FEATURE_BAR);
    assert((bar1 != 0) == bar);
    unsigned qux1 = polyseed_get_feature(seed, FEATURE_QUX);
    assert((qux1 != 0) == qux);
}

static bool test_inject1(void) {
    const polyseed_dependency deps = {
        .randbytes = &gen_rand_bytes1,
        .pbkdf2_sha256 = &pbkdf2_dummy1,
        .u8_nfc = &u8_nfc_donothing,
        .u8_nfkd = &u8_nfkd_spaces,
        .time = &time1,
        .memzero = &do_not_zero,
        .alloc = &count_alloc,
        .free = &count_free,
    };
    polyseed_inject(&deps);
    return true;
}

static bool test_num_langs(void) {
    g_num_langs = polyseed_get_num_langs();
    assert(g_num_langs > 0);
    return true;
}

static bool test_get_lang(void) {
    for (int i = 0; i < g_num_langs; ++i) {
        const polyseed_lang* lang = polyseed_get_lang(i);
        assert(lang != NULL);
    }
    return true;
}

static void test_get_lang_name(void) {
    for (int i = 0; i < g_num_langs; ++i) {
        const polyseed_lang* lang = polyseed_get_lang(i);
        subtest_begin(polyseed_get_lang_name_en(lang));
        assert(polyseed_get_lang_name(lang) != NULL);
        assert(polyseed_get_lang_name_en(lang) != NULL);
        subtest_end(true);
    }
}

static bool test_create1(void) {
    polyseed_status status = polyseed_create(0, &g_seed1);
    assert(status == POLYSEED_OK);
    assert(g_seed1 != NULL);
    return true;
}

static bool test_birthday1(void) {
    uint64_t birthday = polyseed_get_birthday(g_seed1);
    assert(birthday <= SEED_TIME1);
    assert(birthday + 2630000 > SEED_TIME1);
    return true;
}

static bool test_features1(void) {
    check_features(g_seed1, false, false, false);
    polyseed_data* seed;
    polyseed_status status;
    status = polyseed_create(FEATURE_FOO, &seed);
    assert(status == POLYSEED_ERR_UNSUPPORTED);
    status = polyseed_create(FEATURE_BAR, &seed);
    assert(status == POLYSEED_ERR_UNSUPPORTED);
    status = polyseed_create(FEATURE_QUX, &seed);
    assert(status == POLYSEED_ERR_UNSUPPORTED);
    return true;
}

static bool test_keygen1(void) {
    check_key(g_seed1, POLYSEED_MONERO);
    return true;
}

static bool test_store_load1(void) {
    polyseed_store(g_seed1, g_store1);
    polyseed_data* seed;
    polyseed_status res = polyseed_load(g_store1, &seed);
    assert(res == POLYSEED_OK);
    check_key(seed, POLYSEED_MONERO);
    polyseed_free(seed);
    return true;
}

static bool test_format(void) {
    for (int i = 0; i < POLYSEED_SIZE; ++i) {
        for (int j = 0; j < CHAR_BIT; ++j) {
            /* flip j-th bit of the i-th byte */
            uint8_t mask = 1u << j;
            g_store1[i] ^= mask;
            polyseed_data* seed;
            polyseed_status res = polyseed_load(g_store1, &seed);
            assert(res != POLYSEED_OK);
            g_store1[i] ^= mask;
        }
    }
    return true;
}

static bool test_encode_en(void) {
    g_lang_en = get_lang("English");
    if (g_lang_en == NULL) {
        return false;
    }
    polyseed_encode(g_seed1, g_lang_en, POLYSEED_MONERO, g_phrase_out);
    assert(0 == strcmp(g_phrase_out, g_phrase_en1));
    return true;
}

static bool test_load_encode_en(void) {
    g_lang_en = get_lang("English");
    if (g_lang_en == NULL) {
        return false;
    }
    polyseed_data* seed;
    polyseed_status res = polyseed_load(g_store1, &seed);
    assert(res == POLYSEED_OK);
    polyseed_encode(seed, g_lang_en, POLYSEED_MONERO, g_phrase_out);
    assert(0 == strcmp(g_phrase_out, g_phrase_en1));
    polyseed_free(seed);
    return true;
}

static bool test_decode_en(void) {
    if (g_lang_en == NULL) {
        return false;
    }
    const polyseed_lang* lang;
    polyseed_data* seed;
    polyseed_status res = polyseed_decode(g_phrase_en1, POLYSEED_MONERO, &lang, &seed);
    assert(res == POLYSEED_OK);
    assert(lang == g_lang_en);
    check_key(seed, POLYSEED_MONERO);
    polyseed_free(seed);
    return true;
}

static bool test_decode_en_prefix(void) {
    if (g_lang_en == NULL) {
        return false;
    }
    const polyseed_lang* lang;
    polyseed_data* seed;
    polyseed_status res = polyseed_decode(g_phrase_en2, POLYSEED_MONERO, &lang, &seed);
    assert(res == POLYSEED_OK);
    assert(lang == g_lang_en);
    check_key(seed, POLYSEED_MONERO);
    polyseed_free(seed);
    return true;
}

static bool test_decode_en_suffix1(void) {
    if (g_lang_en == NULL) {
        return false;
    }
    const polyseed_lang* lang;
    polyseed_data* seed;
    polyseed_status res = polyseed_decode(g_phrase_en3, POLYSEED_MONERO, &lang, &seed);
    assert(res == POLYSEED_ERR_LANG);
    return true;
}

static bool test_decode_en_suffix2(void) {
    if (g_lang_en == NULL) {
        return false;
    }
    const polyseed_lang* lang;
    polyseed_data* seed;
    polyseed_status res = polyseed_decode(g_phrase_en4, POLYSEED_MONERO, &lang, &seed);
    assert(res == POLYSEED_ERR_LANG);
    return true;
}

static bool test_decode_en_space(void) {
    if (g_lang_en == NULL) {
        return false;
    }
    const polyseed_lang* lang;
    polyseed_data* seed;
    polyseed_status res = polyseed_decode(g_phrase_en5, POLYSEED_MONERO, &lang, &seed);
    assert(res == POLYSEED_OK);
    assert(lang == g_lang_en);
    check_key(seed, POLYSEED_MONERO);
    polyseed_free(seed);
    return true;
}

static bool test_decode_en_coin(void) {
    if (g_lang_en == NULL) {
        return false;
    }
    const polyseed_lang* lang;
    polyseed_data* seed;
    polyseed_status res = polyseed_decode(g_phrase_en1, POLYSEED_AEON, &lang, &seed);
    assert(res == POLYSEED_ERR_CHECKSUM);
    return true;
}

static void test_roundtrip1(void) {
    for (int i = 0; i < g_num_langs; ++i) {
        const polyseed_lang* lang = polyseed_get_lang(i);
        subtest_begin(polyseed_get_lang_name_en(lang));
        const polyseed_lang* lang_out;
        polyseed_str phrase;
        polyseed_data* seed;
        polyseed_encode(g_seed1, lang, POLYSEED_MONERO, phrase);
        polyseed_status res = polyseed_decode(phrase, POLYSEED_MONERO, &lang_out, &seed);
        assert(res == POLYSEED_OK);
        assert(lang == lang_out);
        check_features(seed, false, false, false);
        check_key(seed, POLYSEED_MONERO);
        polyseed_free(seed);
        subtest_end(true);
    }
}

static bool test_free1(void) {
    polyseed_free(g_seed1);
    return true;
}

static bool test_free_null(void) {
    polyseed_free(NULL);
    return true;
}

static bool test_inject2(void) {
    const polyseed_dependency deps = {
        .randbytes = &gen_rand_bytes2,
        .pbkdf2_sha256 = &pbkdf2_dummy2,
        .u8_nfc = &u8_nfc_donothing,
        .u8_nfkd = &u8_nfkd_spaces,
        .time = &time2,
        .memzero = &do_not_zero,
        .alloc = &count_alloc,
        .free = &count_free,
    };
    polyseed_inject(&deps);
    return true;
}

static bool test_create2(void) {
    polyseed_status status = polyseed_create(0, &g_seed2);
    assert(status == POLYSEED_OK);
    assert(g_seed2 != NULL);
    return true;
}

static bool test_birthday2(void) {
    uint64_t birthday = polyseed_get_birthday(g_seed2);
    assert(birthday <= SEED_TIME2);
    assert(birthday + 2630000 > SEED_TIME2);
    return true;
}

static bool test_features2(void) {
    check_features(g_seed2, false, false, false);
    int num_features;
    polyseed_data* seed;
    polyseed_status status;
    num_features = polyseed_enable_features(FEATURE_FOO | FEATURE_BAR | FEATURE_QUX);
    assert(num_features == 3);
    status = polyseed_create(FEATURE_FOO, &seed);
    assert(status == POLYSEED_OK);
    check_features(seed, true, false, false);
    polyseed_free(seed);
    status = polyseed_create(FEATURE_BAR, &seed);
    assert(status == POLYSEED_OK);
    check_features(seed, false, true, false);
    polyseed_free(seed);
    status = polyseed_create(FEATURE_QUX, &seed);
    assert(status == POLYSEED_OK);
    check_features(seed, false, false, true);
    polyseed_free(seed);
    num_features = polyseed_enable_features(0);
    assert(num_features == 0);
    return true;
}

static bool test_keygen2(void) {
    check_key(g_seed2, POLYSEED_MONERO);
    return true;
}

static bool test_store_load2(void) {
    polyseed_store(g_seed2, g_store2);
    polyseed_data* seed;
    polyseed_status res = polyseed_load(g_store2, &seed);
    assert(res == POLYSEED_OK);
    check_key(seed, POLYSEED_MONERO);
    polyseed_free(seed);
    return true;
}

static bool test_encode_es(void) {
    g_lang_es = get_lang("Spanish");
    if (g_lang_es == NULL) {
        return false;
    }
    polyseed_encode(g_seed2, g_lang_es, POLYSEED_MONERO, g_phrase_out);
    assert(0 == strcmp(g_phrase_out, g_phrase_es1));
    return true;
}

static bool test_decode_es(void) {
    if (g_lang_es == NULL) {
        return false;
    }
    const polyseed_lang* lang;
    polyseed_data* seed;
    polyseed_status res = polyseed_decode(g_phrase_out, POLYSEED_MONERO, &lang, &seed);
    assert(res == POLYSEED_OK);
    assert(lang == g_lang_es);
    check_key(seed, POLYSEED_MONERO);
    polyseed_free(seed);
    return true;
}

static bool test_decode_es_noaccent(void) {
    if (g_lang_es == NULL) {
        return false;
    }
    const polyseed_lang* lang;
    polyseed_data* seed;
    polyseed_status res = polyseed_decode(g_phrase_es2, POLYSEED_MONERO, &lang, &seed);
    assert(res == POLYSEED_OK);
    assert(lang == g_lang_es);
    check_key(seed, POLYSEED_MONERO);
    polyseed_free(seed);
    return true;
}

static bool test_decode_es_prefix1(void) {
    if (g_lang_es == NULL) {
        return false;
    }
    const polyseed_lang* lang;
    polyseed_data* seed;
    polyseed_status res = polyseed_decode(g_phrase_es3, POLYSEED_MONERO, &lang, &seed);
    assert(res == POLYSEED_OK);
    assert(lang == g_lang_es);
    check_key(seed, POLYSEED_MONERO);
    polyseed_free(seed);
    return true;
}

static bool test_decode_es_suffix(void) {
    if (g_lang_es == NULL) {
        return false;
    }
    const polyseed_lang* lang;
    polyseed_data* seed;
    polyseed_status res = polyseed_decode(g_phrase_es4, POLYSEED_MONERO, &lang, &seed);
    assert(res == POLYSEED_ERR_LANG);
    return true;
}

static bool test_decode_es_prefix2(void) {
    if (g_lang_es == NULL) {
        return false;
    }
    const polyseed_lang* lang;
    polyseed_data* seed;
    polyseed_status res = polyseed_decode(g_phrase_es5, POLYSEED_MONERO, &lang, &seed);
    assert(res == POLYSEED_ERR_LANG);
    return true;
}

static bool test_decode_es_mult1(void) {
    const polyseed_lang* lang;
    polyseed_data* seed;
    polyseed_status res = polyseed_decode(g_phrase_es_mult, POLYSEED_MONERO, &lang, &seed);
    assert(res == POLYSEED_ERR_MULT_LANG);
    return true;
}

static bool test_decode_es_mult2(void) {
    if (g_lang_es == NULL) {
        return false;
    }
    polyseed_data* seed;
    polyseed_status res = polyseed_decode_explicit(g_phrase_es_mult, POLYSEED_MONERO, g_lang_es, &seed);
    assert(res == POLYSEED_OK);
    polyseed_free(seed);
    return true;
}

static bool test_free2(void) {
    polyseed_free(g_seed2);
    return true;
}

static bool test_inject3(void) {
    const polyseed_dependency dep = {
        .randbytes = &gen_rand_bytes3,
        .pbkdf2_sha256 = &pbkdf2_dummy3,
        .u8_nfc = &u8_nfc_donothing,
        .u8_nfkd = &u8_nfkd_spaces,
        .time = &time3,
        .memzero = &do_not_zero,
        .alloc = &count_alloc,
        .free = &count_free,
    };
    polyseed_inject(&dep);
    return true;
}

static bool test_features3a(void) {
    int num_features = polyseed_enable_features(FEATURE_FOO | FEATURE_QUX);
    assert(num_features == 2);
    polyseed_data* seed;
    polyseed_status status;
    status = polyseed_create(FEATURE_FOO, &seed);
    assert(status == POLYSEED_OK);
    polyseed_free(seed);
    status = polyseed_create(FEATURE_BAR, &seed);
    assert(status == POLYSEED_ERR_UNSUPPORTED);
    status = polyseed_create(FEATURE_QUX, &seed);
    assert(status == POLYSEED_OK);
    polyseed_free(seed);
    return true;
}

static bool test_create3(void) {
    polyseed_status status = polyseed_create(FEATURE_FOO, &g_seed3);
    assert(status == POLYSEED_OK);
    assert(g_seed3 != NULL);
    return true;
}

static bool test_features3b(void) {
    check_features(g_seed3, true, false, false);
    return true;
}

static bool test_birthday3(void) {
    uint64_t birthday = polyseed_get_birthday(g_seed3);
    assert(birthday <= SEED_TIME3);
    assert(birthday + 2630000 > SEED_TIME3);
    return true;
}

static bool test_keygen3(void) {
    check_key(g_seed3, POLYSEED_AEON);
    return true;
}

static bool test_store_load3(void) {
    polyseed_store(g_seed3, g_store3);
    polyseed_data* seed;
    polyseed_status res = polyseed_load(g_store3, &seed);
    assert(res == POLYSEED_OK);
    check_key(seed, POLYSEED_AEON);
    polyseed_free(seed);
    return true;
}

static void test_roundtrip3(void) {
    for (int i = 0; i < g_num_langs; ++i) {
        const polyseed_lang* lang = polyseed_get_lang(i);
        subtest_begin(polyseed_get_lang_name_en(lang));
        const polyseed_lang* lang_out;
        polyseed_str phrase;
        polyseed_data* seed;
        polyseed_encode(g_seed3, lang, POLYSEED_AEON, phrase);
        polyseed_status res = polyseed_decode(phrase, POLYSEED_AEON, &lang_out, &seed);
        assert(res == POLYSEED_OK);
        assert(lang == lang_out);
        check_features(seed, true, false, false);
        check_key(seed, POLYSEED_AEON);
        polyseed_free(seed);
        subtest_end(true);
    }
}

static bool test_encrypt(void) {
    if (g_lang_en == NULL) {
        return false;
    }
    assert(!polyseed_is_encrypted(g_seed3));
    polyseed_crypt(g_seed3, g_test_pass);
    assert(polyseed_is_encrypted(g_seed3));
    polyseed_encode(g_seed3, g_lang_en, POLYSEED_AEON, g_phrase_out);
    return true;
}

static bool test_decrypt(void) {
    if (g_lang_en == NULL) {
        return false;
    }
    polyseed_data* seed;
    polyseed_status res = polyseed_decode(g_phrase_out, POLYSEED_AEON, NULL, &seed);
    assert(res == POLYSEED_OK);
    assert(polyseed_is_encrypted(seed));
    polyseed_crypt(seed, g_test_pass);
    assert(!polyseed_is_encrypted(seed));
    check_key(seed, POLYSEED_AEON);
    polyseed_free(seed);
    return true;
}

static bool test_free3(void) {
    polyseed_free(g_seed3);
    return true;
}

static bool test_decode_garbage1(void) {
    const polyseed_lang* lang;
    polyseed_data* seed;
    polyseed_status res = polyseed_decode(g_phrase_garbage1, POLYSEED_MONERO, &lang, &seed);
    assert(res == POLYSEED_ERR_NUM_WORDS);
    return true;
}

static bool test_decode_garbage2(void) {
    const polyseed_lang* lang;
    polyseed_data* seed;
    polyseed_status res = polyseed_decode(g_phrase_garbage2, POLYSEED_MONERO, &lang, &seed);
    assert(res == POLYSEED_ERR_NUM_WORDS);
    return true;
}

static bool test_memleak(void) {
    assert(g_num_allocs == 0);
    return true;
}

static bool test_inject4(void) {
    const polyseed_dependency dep = {
        .randbytes = &gen_rand_bytes3,
        .pbkdf2_sha256 = &pbkdf2_dummy3,
        .u8_nfc = &u8_nfc_donothing,
        .u8_nfkd = &u8_nfkd_spaces,
        .memzero = &do_not_zero,
        .alloc = &alloc_fail,
    };
    polyseed_inject(&dep);
    return true;
}

static bool test_out_of_memory1(void) {
    polyseed_data* seed;
    polyseed_status res = polyseed_create(0, &seed);
    assert(res == POLYSEED_ERR_MEMORY);
    return true;
}

static bool test_out_of_memory2(void) {
    polyseed_data* seed;
    polyseed_status res = polyseed_load(g_store3, &seed);
    assert(res == POLYSEED_ERR_MEMORY);
    return true;
}

int main() {
    RUN_TEST(test_inject1);
    RUN_TEST(test_num_langs);
    RUN_TEST(test_get_lang);
    RUN_MULT(test_get_lang_name);
    RUN_TEST(test_create1);
    RUN_TEST(test_birthday1);
    RUN_TEST(test_features1);
    RUN_TEST(test_keygen1);
    RUN_TEST(test_store_load1);
    RUN_TEST(test_format);
    RUN_TEST(test_encode_en);
    RUN_TEST(test_load_encode_en);
    RUN_TEST(test_decode_en);
    RUN_TEST(test_decode_en_prefix);
    RUN_TEST(test_decode_en_suffix1);
    RUN_TEST(test_decode_en_suffix2);
    RUN_TEST(test_decode_en_space);
    RUN_TEST(test_decode_en_coin);
    RUN_MULT(test_roundtrip1);
    RUN_TEST(test_free1);
    RUN_TEST(test_free_null);
    RUN_TEST(test_inject2);
    RUN_TEST(test_create2);
    RUN_TEST(test_birthday2);
    RUN_TEST(test_features2);
    RUN_TEST(test_keygen2);
    RUN_TEST(test_store_load2);
    RUN_TEST(test_encode_es);
    RUN_TEST(test_decode_es);
    RUN_TEST(test_decode_es_noaccent);
    RUN_TEST(test_decode_es_prefix1);
    RUN_TEST(test_decode_es_suffix);
    RUN_TEST(test_decode_es_prefix2);
    RUN_TEST(test_decode_es_mult1);
    RUN_TEST(test_decode_es_mult2);
    RUN_TEST(test_free2);
    RUN_TEST(test_inject3);
    RUN_TEST(test_features3a);
    RUN_TEST(test_create3);
    RUN_TEST(test_features3b);
    RUN_TEST(test_birthday3);
    RUN_TEST(test_keygen3);
    RUN_TEST(test_store_load3);
    RUN_MULT(test_roundtrip3);
    RUN_TEST(test_encrypt);
    RUN_TEST(test_decrypt);
    RUN_TEST(test_free3);
    RUN_TEST(test_decode_garbage1);
    RUN_TEST(test_decode_garbage2);
    RUN_TEST(test_memleak);
    RUN_TEST(test_inject4);
    RUN_TEST(test_out_of_memory1);
    RUN_TEST(test_out_of_memory2);

    printf("\nAll tests were successful\n");
    return 0;
}
