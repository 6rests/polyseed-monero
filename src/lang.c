/* Copyright (c) 2020-2021 tevador <tevador@gmail.com> */
/* See LICENSE for licensing information */

#include "polyseed.h"
#include "dependency.h"
#include "lang.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

extern const polyseed_lang polyseed_lang_en;
extern const polyseed_lang polyseed_lang_jp;
extern const polyseed_lang polyseed_lang_ko;
extern const polyseed_lang polyseed_lang_es;
extern const polyseed_lang polyseed_lang_zh_s;
extern const polyseed_lang polyseed_lang_zh_t;
extern const polyseed_lang polyseed_lang_fr;
extern const polyseed_lang polyseed_lang_it;
extern const polyseed_lang polyseed_lang_cs;
extern const polyseed_lang polyseed_lang_pt;

static const polyseed_lang* languages[] = {
    /* sorted wordlists first */
    /* https://github.com/bitcoin/bips/blob/master/bip-0039/bip-0039-wordlists.md */
    &polyseed_lang_en,
    &polyseed_lang_jp,
    &polyseed_lang_ko,
    &polyseed_lang_es,
    &polyseed_lang_fr,
    &polyseed_lang_it,
    &polyseed_lang_cs,
    &polyseed_lang_pt,
    &polyseed_lang_zh_s,
    &polyseed_lang_zh_t,
};

#define NUM_LANGS sizeof(languages) / sizeof(uintptr_t)

int polyseed_get_num_langs(void) {
    return NUM_LANGS;
}

const polyseed_lang* polyseed_get_lang(int i) {
    assert(i >= 0 && i < NUM_LANGS);
    return languages[i];
}

const char* polyseed_get_lang_name(const polyseed_lang* lang) {
    assert(lang != NULL);
    return lang->name;
}

const char* polyseed_get_lang_name_en(const polyseed_lang* lang) {
    assert(lang != NULL);
    return lang->name_en;
}

#define NUM_CHARS_PREFIX 4

typedef int polyseed_cmp(const void* a, const void* b);

static int lang_search(const polyseed_lang* lang, const char* word,
    polyseed_cmp* cmp) {
    if (lang->is_sorted) {
        const char** match = bsearch(&word, &lang->words[0],
            POLYSEED_LANG_SIZE, sizeof(const char*), cmp);
        if (match != NULL) {
            return match - &lang->words[0];
        }
        return -1;
    }
    else {
        for (int j = 0; j < POLYSEED_LANG_SIZE; ++j) {
            if (0 == cmp(&word, &lang->words[j])) {
                return j;
            }
        }
        return -1;
    }
}

static int compare_str(const char* key, const char* elm) {
    for (;;) {
        if (*key == '\0' || *key != *elm) {
            break;
        }
        ++key;
        ++elm;
    }
    return (*key > *elm) - (*key < *elm);
}

static int compare_str_wrap(const void* a, const void* b) {
    const char* key = *(const char**)a;
    const char* elm = *(const char**)b;
    return compare_str(key, elm);
}

static int compare_prefix(const char* key, const char* elm, int n) {
    for (int i = 1; ; ++i) {
        if (*key == '\0') {
            break;
        }
        if (i >= n && key[1] == '\0') {
            break;
        }
        if (*key != *elm) {
            break;
        }
        ++key;
        ++elm;
    }
    return (*key > *elm) - (*key < *elm);
}

static int compare_prefix_wrap(const void* a, const void* b) {
    const char* key = *(const char**)a;
    const char* elm = *(const char**)b;
    return compare_prefix(key, elm, NUM_CHARS_PREFIX);
}

static int compare_str_noaccent(const char* key, const char* elm) {
    for (;;) {
        while (*key < 0) { /* skip non-ASCII */
            ++key;
        }
        while (*elm < 0) { /* skip non-ASCII */
            ++elm;
        }
        if (*key == '\0' || *key != *elm) {
            break;
        }
        ++key;
        ++elm;
    }
    return (*key > *elm) - (*key < *elm);
}

static int compare_str_noaccent_wrap(const void* a, const void* b) {
    const char* key = *(const char**)a;
    const char* elm = *(const char**)b;
    return compare_str_noaccent(key, elm);
}

static int compare_prefix_noaccent(const char* key, const char* elm, int n) {
    for (int i = 1; ; ++i) {
        while (*key < 0) { /* skip non-ASCII */
            ++key;
        }
        while (*elm < 0) { /* skip non-ASCII */
            ++elm;
        }
        if (*key == '\0') {
            break;
        }
        if (i >= n && key[1] == '\0') {
            break;
        }
        if (*key != *elm) {
            break;
        }
        ++key;
        ++elm;
    }
    while (*key < 0) { /* skip non-ASCII */
        ++key;
    }
    while (*elm < 0) { /* skip non-ASCII */
        ++elm;
    }
    return (*key > *elm) - (*key < *elm);
}

static int compare_prefix_noaccent_wrap(const void* a, const void* b) {
    const char* key = *(const char**)a;
    const char* elm = *(const char**)b;
    return compare_prefix_noaccent(key, elm, NUM_CHARS_PREFIX);
}

static polyseed_cmp* get_comparer(const polyseed_lang* lang) {
    if (lang->has_prefix) {
        if (lang->has_accents) {
            return &compare_prefix_noaccent_wrap;
        }
        else {
            return &compare_prefix_wrap;
        }
    }
    else {
        if (lang->has_accents) {
            return &compare_str_noaccent_wrap;
        }
        else {
            return &compare_str_wrap;
        }
    }
}

int polyseed_lang_find_word(const polyseed_lang* lang, const char* word) {
    polyseed_cmp* cmp = get_comparer(lang);
    return lang_search(lang, word, cmp);
}

polyseed_status polyseed_phrase_decode(const polyseed_phrase phrase,
    uint_fast16_t idx_out[POLYSEED_NUM_WORDS], const polyseed_lang** lang_out) {
    /* Iterate through all languages and try to find just one where
       all the words are a match. */
    uint_fast16_t idx[POLYSEED_NUM_WORDS];
    bool have_lang = false;
    for (int li = 0; li < NUM_LANGS; ++li) {
        const polyseed_lang* lang = languages[li];
        polyseed_cmp* cmp = get_comparer(lang);
        bool success = true;
        for (int wi = 0; wi < POLYSEED_NUM_WORDS; ++wi) {
            const char* word = phrase[wi];
            int value = lang_search(lang, word, cmp);
            if (value < 0) {
                success = false;
                break;
            }
            idx[wi] = value;
        }
        if (!success) {
            continue;
        }
        if (have_lang) {
            /* The phrase can decode in multiple languages.
            Use polyseed_phrase_decode_explicit. */
            return POLYSEED_ERR_MULT_LANG;
        }
        have_lang = true;
        for (int wi = 0; wi < POLYSEED_NUM_WORDS; ++wi) {
            idx_out[wi] = idx[wi];
        }
        if (lang_out != NULL) {
            *lang_out = lang;
        }
    }
    return have_lang ? POLYSEED_OK : POLYSEED_ERR_LANG;
}

polyseed_status polyseed_phrase_decode_explicit(const polyseed_phrase phrase,
    const polyseed_lang* lang, uint_fast16_t idx_out[POLYSEED_NUM_WORDS]) {

    polyseed_cmp* cmp = get_comparer(lang);
    for (int wi = 0; wi < POLYSEED_NUM_WORDS; ++wi) {
        const char* word = phrase[wi];
        int value = lang_search(lang, word, cmp);
        if (value < 0) {
            return POLYSEED_ERR_LANG;
        }
        idx_out[wi] = value;
    }
    return POLYSEED_OK;
}

void polyseed_lang_check(const polyseed_lang* lang) {
    /* check the language is sorted correctly */
    if (lang->is_sorted) {
        polyseed_cmp* cmp = get_comparer(lang);
        const char* prev = lang->words[0];
        for (int i = 1; i < POLYSEED_LANG_SIZE; ++i) {
            const char* word = lang->words[i];
            assert(("incorrectly sorted wordlist", cmp(&prev, &word) < 0));
            prev = word;
        }
    }
    /* all words must be in NFKD */
    for (int i = 0; i < POLYSEED_LANG_SIZE; ++i) {
        polyseed_str norm;
        const char* word = lang->words[i];
        UTF8_DECOMPOSE(word, norm);
        assert(("incorrectly normalized wordlist", !strcmp(word, norm)));
    }
    /* accented languages must be composed */
    assert(!lang->has_accents || lang->compose);
    /* normalized separator must be a space */
    polyseed_str separator;
    UTF8_DECOMPOSE(lang->separator, separator);
    assert(!strcmp(" ", separator));
}
