/**
 * crypto.h - Cryptography subsystem definitions
 * 
 * This file contains definitions for the cryptography subsystem.
 */

#ifndef _KERNEL_CRYPTO_H
#define _KERNEL_CRYPTO_H

#include <horizon/types.h>

/* Hash algorithm types */
typedef enum {
    HASH_MD5,
    HASH_SHA1,
    HASH_SHA256,
    HASH_SHA512
} hash_algorithm_t;

/* Cipher algorithm types */
typedef enum {
    CIPHER_AES,
    CIPHER_DES,
    CIPHER_3DES,
    CIPHER_BLOWFISH,
    CIPHER_TWOFISH,
    CIPHER_SERPENT,
    CIPHER_RC4
} cipher_algorithm_t;

/* Cipher modes */
typedef enum {
    CIPHER_MODE_ECB,
    CIPHER_MODE_CBC,
    CIPHER_MODE_CFB,
    CIPHER_MODE_OFB,
    CIPHER_MODE_CTR,
    CIPHER_MODE_XTS
} cipher_mode_t;

/* Hash context */
typedef struct hash_context {
    hash_algorithm_t algorithm;     /* Hash algorithm */
    void *context;                  /* Algorithm-specific context */
    u32 digest_size;                /* Size of the digest in bytes */
    u32 block_size;                 /* Size of the block in bytes */
} hash_context_t;

/* Cipher context */
typedef struct cipher_context {
    cipher_algorithm_t algorithm;   /* Cipher algorithm */
    cipher_mode_t mode;             /* Cipher mode */
    void *context;                  /* Algorithm-specific context */
    u32 key_size;                   /* Size of the key in bytes */
    u32 block_size;                 /* Size of the block in bytes */
    u32 iv_size;                    /* Size of the IV in bytes */
} cipher_context_t;

/* Cryptography functions */
void crypto_init(void);
int crypto_hash_init(hash_context_t *ctx, hash_algorithm_t algorithm);
int crypto_hash_update(hash_context_t *ctx, const void *data, size_t len);
int crypto_hash_final(hash_context_t *ctx, void *digest);
int crypto_cipher_init(cipher_context_t *ctx, cipher_algorithm_t algorithm, cipher_mode_t mode, const void *key, size_t key_len, const void *iv);
int crypto_cipher_encrypt(cipher_context_t *ctx, const void *in, void *out, size_t len);
int crypto_cipher_decrypt(cipher_context_t *ctx, const void *in, void *out, size_t len);
int crypto_cipher_final(cipher_context_t *ctx);
int crypto_random_bytes(void *buf, size_t len);

#endif /* _KERNEL_CRYPTO_H */
