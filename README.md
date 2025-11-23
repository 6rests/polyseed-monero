# Polyseed Monero

## Features

* 16 mnemonic words (36% shorter than the original 25-word seed)
* embedded wallet birthday to optimize restoring from the seed
* supports encryption by a passphrase
* can store up to 3 custom bits
* advanced checksum based on a polynomial code
* seeds are incompatible between different coins

Supported languages:

1. English
2. Japanese
3. Korean
4. Spanish
5. French
6. Italian
7. Czech
8. Portuguese
9. Chinese (Simplified)
10. Chinese (Traditional)


For languages based on the latin alphabet, just the first 4 characters of each word need to be provided when restoring a seed. French and Spanish seeds can be input with or without accents. Wordlists are based on [BIP-39](https://github.com/bitcoin/bips/blob/master/bip-0039/bip-0039-wordlists.md) with a few minor changes.

## Encoding

Each word contains 11 bits of information. The data are encoded as follows:

|word #| contents |
|----|----------|
|1   | checksum (11 bits) |
|2-6 | secret seed (10 bits) + features (1 bit) |
|7-16| secret seed (10 bits) + birthday (1 bit) |

In total, there are 11 bits for the checksum, 150 bits for the secret seed, 5 feature bits and 10 birthday bits. Because the feature and birthday bits are non-random, they are spread over the 15 data words so that two different mnemonic phrases are unlikely to have the same word in the same position.

### Checksum

The mnemonic phrase can be treated as a polynomial over GF(2048), which enables the use of an efficient Reed-Solomon error correction code with one check word. All single-word errors can be detected and all single-word erasures can be corrected without false positives.

To prevent the seed from being accidentally used with a different cryptocurrency, a coin flag is XORed with the second word after the checksum is calculated. Checksum validation will fail unless the wallet software XORs the same coin flag with the second word when restoring.

### Feature bits

There are 5 feature bits in the phrase. The first 2 bits are for internal use (one bit is used to indicate a seed encrypted by a passphrase and the other bit is reserved for a future update of the key derivation function). The remaining 3 bits are reserved for library users and can be enabled and accessed through the API. The library requires reserved bits to be zero (if not, `POLYSEED_ERR_UNSUPPORTED` is returned).

Note: 3rd party wallet software using polyseed should never define their own feature bits for compatibility reasons.

### Wallet birthday

The mnemonic phrase stores the approximate date when the wallet was created. This allows the seed to be generated offline without access to the blockchain. Wallet software can easily convert a date to the corresponding block height when restoring a seed.

The wallet birthday has a resolution of 2629746 seconds (1/12 of the average Gregorian year). All dates between November 2021 and February 2107 can be represented.

### Secret seed

Polyseed was designed for the 128-bit security level. This corresponds to the security of the ed25519 elliptic curve, which requires [about 2<sup>126</sup> operations](https://safecurves.cr.yp.to/rho.html) to break a key.

The private key is derived from the 150-bit secret seed using PBKDF2-HMAC-SHA256 with 10000 iterations. The KDF parameters were selected to allow for the key to be derived by hardware wallets. Key generation is domain-separated by the wallet birthday month, seed features and the coin flag.

The size of the secret seed and the domain separation parameters provide a comfortable security margin against [multi-target attacks](https://blog.cr.yp.to/20151120-batchattacks.html).

## Build

```
git clone https://github.com/payp4k/polyseed-monero
cd src
python build_files.py
```

This will build a static library, a dynamic library and an executable with functional tests.

## API

The API is documented in the public header file [polyseed.h](include/polyseed.h). The [polyseed-examples](https://github.com/payp4k/polyseed-examples) repository contains language bindings and examples for C, C++ and C#.

## Dependency injection

Polyseed uses dependency injection. The following 5 functions must be provided by calling `polyseed_inject`:

| dependency | description | implemented in |
|------------|-------------|----------------|
| randbytes  | Function to generate cryptographically secure random bytes | [libsodium](https://github.com/jedisct1/libsodium), [OpenSSL](https://github.com/openssl/openssl) |
| pbkdf2_sha256 | Function to calculate PBKDF2 based on HMAC-SHA256 | [libsodium](https://github.com/jedisct1/libsodium), [OpenSSL](https://github.com/openssl/openssl) |
| memzero | Function to securely erase memory | [libsodium](https://github.com/jedisct1/libsodium), [OpenSSL](https://github.com/openssl/openssl) |
| u8_nfc | Function to convert a UTF8 string to the composed canonical form. | [Boost.Locale](https://www.boost.org/doc/libs/1_77_0/libs/locale/doc/html/), [utf8proc](https://github.com/JuliaStrings/utf8proc) |
| u8_nfkd | Function to convert a UTF8 string to the decomposed canonical form. | [Boost.Locale](https://www.boost.org/doc/libs/1_77_0/libs/locale/doc/html/), [utf8proc](https://github.com/JuliaStrings/utf8proc) |

These functions are implemented in widely used and tested libraries and it would be out of the scope of this library to implement them. It also reduces the security risks (polyseed doesn't contain any cryptographic code). The [polyseed-examples](https://github.com/payp4k/polyseed-examples) repository contains examples how to inject the dependencies for C, C++ and C# projects.

Additional 3 functions are optional dependencies. If they are not provided (the corresponding function pointer is `NULL`), polyseed will use the default implementation from the Standard C Library.

| dependency | description | libc function |
|------------|-------------|----------------|
| time  | Function to get the current unix time | `uint64_t time(void);` |
| alloc | Function to allocate memory | `void* malloc(size_t size);` |
| free | Function to free memory | `void free(void* ptr)` |

These are mostly needed for testing purposes, but can be also used to provide a custom memory allocator.

## License

The library is released under the LGPLv3 license. No restrictions are placed on software that just links to the library.
