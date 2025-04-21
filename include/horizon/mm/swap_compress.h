/**
 * swap_compress.h - Horizon kernel swap compression definitions
 * 
 * This file contains definitions for the swap compression subsystem.
 */

#ifndef _HORIZON_MM_SWAP_COMPRESS_H
#define _HORIZON_MM_SWAP_COMPRESS_H

#include <horizon/types.h>

/* Compression algorithms */
typedef enum swap_compress_algo {
    SWAP_COMPRESS_NONE,    /* No compression */
    SWAP_COMPRESS_LZ4,     /* LZ4 compression */
    SWAP_COMPRESS_ZLIB,    /* ZLIB compression */
    SWAP_COMPRESS_ZSTD     /* ZSTD compression */
} swap_compress_algo_t;

/* Initialize the swap compression subsystem */
void swap_compress_init(void);

/* Set the compression algorithm */
int swap_compress_set_algo(swap_compress_algo_t algo);

/* Get the compression algorithm */
swap_compress_algo_t swap_compress_get_algo(void);

/* Compress a page */
ssize_t swap_compress_page(void *in, void *out, size_t in_size, size_t out_size);

/* Decompress a page */
ssize_t swap_decompress_page(void *in, void *out, size_t in_size, size_t out_size);

/* Compress a page using LZ4 */
ssize_t swap_compress_lz4(void *in, void *out, size_t in_size, size_t out_size);

/* Decompress a page using LZ4 */
ssize_t swap_decompress_lz4(void *in, void *out, size_t in_size, size_t out_size);

/* Compress a page using ZLIB */
ssize_t swap_compress_zlib(void *in, void *out, size_t in_size, size_t out_size);

/* Decompress a page using ZLIB */
ssize_t swap_decompress_zlib(void *in, void *out, size_t in_size, size_t out_size);

/* Compress a page using ZSTD */
ssize_t swap_compress_zstd(void *in, void *out, size_t in_size, size_t out_size);

/* Decompress a page using ZSTD */
ssize_t swap_decompress_zstd(void *in, void *out, size_t in_size, size_t out_size);

/* Print compression statistics */
void swap_compress_print_stats(void);

#endif /* _HORIZON_MM_SWAP_COMPRESS_H */
