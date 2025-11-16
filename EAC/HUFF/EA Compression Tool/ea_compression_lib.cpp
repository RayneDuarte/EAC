// EA Compression Library Wrapper
// This file provides a C-compatible API for building a shared library

#if defined __unix__ || defined __MINGW32__ || defined __MINGW64__
	#define EAC_UNIX
#endif

#ifdef EAC_UNIX
	#include "eac_unix.cpp"
#endif

#include <cstring>
#include <cstdlib>
#include "codex.h"
#include "huffcodex.h"
#include "refcodex.h"
#include "btreecodex.h"
#include "jdlz_compression.h"
#include "ea_comp.h"

// Export symbols for shared library
#ifdef _WIN32
    #define EA_EXPORT __declspec(dllexport)
#else
    #define EA_EXPORT __attribute__((visibility("default")))
#endif

extern "C" {

// Compression format types
typedef enum {
    EA_FORMAT_HUFF = 0,
    EA_FORMAT_JDLZ = 1,
    EA_FORMAT_REF = 2,
    EA_FORMAT_BTREE = 3,
    EA_FORMAT_COMP = 4,
    EA_FORMAT_UNKNOWN = -1
} ea_format_t;

// Return codes
typedef enum {
    EA_OK = 0,
    EA_ERROR_INVALID_FORMAT = -1,
    EA_ERROR_DECOMPRESS = -2,
    EA_ERROR_COMPRESS = -3,
    EA_ERROR_NULL_POINTER = -4,
    EA_ERROR_BUFFER_TOO_SMALL = -5
} ea_result_t;

/**
 * Detect compression format from compressed data
 * @param data Pointer to compressed data
 * @param size Size of compressed data
 * @return Format type or EA_FORMAT_UNKNOWN
 */
EA_EXPORT ea_format_t ea_detect_format(const unsigned char *data, int size) {
    if (!data || size < 4) {
        return EA_FORMAT_UNKNOWN;
    }

    // Check for JDLZ header
    if (size >= 4 && memcmp(data, "JDLZ", 4) == 0) {
        return EA_FORMAT_JDLZ;
    }

    // Check for HUFF header
    if (size >= 4 && memcmp(data, "HUFF", 4) == 0) {
        return EA_FORMAT_HUFF;
    }

    // Check for COMP header
    if (size >= 4 && memcmp(data, "COMP", 4) == 0) {
        return EA_FORMAT_COMP;
    }

    // Check for REF format
    if (REF_is(data)) {
        return EA_FORMAT_REF;
    }

    // Check for BTREE format
    if (BTREE_is(data)) {
        return EA_FORMAT_BTREE;
    }

    return EA_FORMAT_UNKNOWN;
}

/**
 * Get decompressed size from compressed data
 * @param data Pointer to compressed data
 * @param size Size of compressed data
 * @return Decompressed size or -1 on error
 */
EA_EXPORT int ea_get_decompressed_size(const unsigned char *data, int size) {
    if (!data || size < 4) {
        return -1;
    }

    ea_format_t format = ea_detect_format(data, size);

    switch (format) {
        case EA_FORMAT_HUFF:
        case EA_FORMAT_JDLZ:
        case EA_FORMAT_COMP:
            // These formats have the size at offset 8 (32-bit little-endian)
            if (size >= 12) {
                return (data[8] | (data[9] << 8) | (data[10] << 16) | (data[11] << 24));
            }
            return -1;

        case EA_FORMAT_REF:
            return REF_size(data);

        case EA_FORMAT_BTREE:
            return BTREE_size(data);

        default:
            return -1;
    }
}

/**
 * Decompress data
 * @param compressed_data Input compressed data
 * @param compressed_size Size of compressed data
 * @param decompressed_data Output buffer for decompressed data
 * @param decompressed_size Size of output buffer
 * @return Number of bytes decompressed or negative error code
 */
EA_EXPORT int ea_decompress(
    const unsigned char *compressed_data,
    int compressed_size,
    unsigned char *decompressed_data,
    int decompressed_size)
{
    if (!compressed_data || !decompressed_data) {
        return EA_ERROR_NULL_POINTER;
    }

    if (compressed_size < 4) {
        return EA_ERROR_INVALID_FORMAT;
    }

    ea_format_t format = ea_detect_format(compressed_data, compressed_size);
    int result = 0;
    int expected_size = ea_get_decompressed_size(compressed_data, compressed_size);

    if (expected_size > decompressed_size) {
        return EA_ERROR_BUFFER_TOO_SMALL;
    }

    switch (format) {
        case EA_FORMAT_HUFF: {
            if (HUFF_is(compressed_data + 16)) {
                int z_size = compressed_size - 16;
                result = HUFF_decode(decompressed_data, compressed_data + 16, &z_size);
            } else {
                return EA_ERROR_INVALID_FORMAT;
            }
            break;
        }

        case EA_FORMAT_JDLZ: {
            int z_size = (compressed_data[12] | (compressed_data[13] << 8) | 
                         (compressed_data[14] << 16) | (compressed_data[15] << 24));
            result = JDLZ_Decompress(
                (unsigned char*)(compressed_data + 16),
                z_size,
                decompressed_data,
                decompressed_size
            );
            break;
        }

        case EA_FORMAT_COMP: {
            int z_size = (compressed_data[12] | (compressed_data[13] << 8) | 
                         (compressed_data[14] << 16) | (compressed_data[15] << 24)) - 16;
            result = COMP_Decompress(
                (unsigned char*)(compressed_data + 16),
                z_size,
                decompressed_data,
                decompressed_size
            );
            break;
        }

        case EA_FORMAT_REF: {
            int z_size = compressed_size;
            result = REF_decode(decompressed_data, compressed_data, &z_size);
            break;
        }

        case EA_FORMAT_BTREE: {
            int z_size = compressed_size;
            result = BTREE_decode(decompressed_data, compressed_data, &z_size);
            break;
        }

        default:
            return EA_ERROR_INVALID_FORMAT;
    }

    if (result <= 0 || (expected_size > 0 && result != expected_size)) {
        return EA_ERROR_DECOMPRESS;
    }

    return result;
}

/**
 * Compress data with HUFF format
 * @param source Source data to compress
 * @param source_size Size of source data
 * @param dest Destination buffer (should be at least source_size * 2 + 16)
 * @param dest_size Size of destination buffer
 * @param huff_type HUFF compression type (0, 1, or 2)
 * @return Compressed size (including 16-byte header) or negative error code
 */
EA_EXPORT int ea_compress_huff(
    const unsigned char *source,
    int source_size,
    unsigned char *dest,
    int dest_size,
    int huff_type)
{
    if (!source || !dest) {
        return EA_ERROR_NULL_POINTER;
    }

    if (dest_size < source_size * 2 + 16) {
        return EA_ERROR_BUFFER_TOO_SMALL;
    }

    if (huff_type < 0 || huff_type > 2) {
        return EA_ERROR_INVALID_FORMAT;
    }

    int compressed_size = HUFF_encode(dest + 16, source, source_size, &huff_type);
    
    if (compressed_size <= 0) {
        return EA_ERROR_COMPRESS;
    }

    // Create HUFF header
    memset(dest, 0, 16);
    memcpy(dest, "HUFF", 4);
    dest[4] = 0x01;
    dest[5] = 0x10;
    dest[8] = source_size;
    dest[9] = source_size >> 8;
    dest[10] = source_size >> 16;
    dest[11] = source_size >> 24;
    dest[12] = compressed_size;
    dest[13] = compressed_size >> 8;
    dest[14] = compressed_size >> 16;
    dest[15] = compressed_size >> 24;

    return compressed_size + 16;
}

/**
 * Compress data with JDLZ format
 * @param source Source data to compress
 * @param source_size Size of source data
 * @param dest Destination buffer (should be at least source_size * 2)
 * @param dest_size Size of destination buffer
 * @return Compressed size or negative error code
 */
EA_EXPORT int ea_compress_jdlz(
    const unsigned char *source,
    int source_size,
    unsigned char *dest,
    int dest_size)
{
    if (!source || !dest) {
        return EA_ERROR_NULL_POINTER;
    }

    if (dest_size < source_size * 2) {
        return EA_ERROR_BUFFER_TOO_SMALL;
    }

    int compressed_size = JDLZ_Compress((unsigned char*)source, source_size, dest);
    
    if (compressed_size <= 0) {
        return EA_ERROR_COMPRESS;
    }

    return compressed_size;
}

/**
 * Compress data with REF format
 * @param source Source data to compress
 * @param source_size Size of source data
 * @param dest Destination buffer (should be at least source_size * 2)
 * @param dest_size Size of destination buffer
 * @return Compressed size or negative error code
 */
EA_EXPORT int ea_compress_ref(
    const unsigned char *source,
    int source_size,
    unsigned char *dest,
    int dest_size)
{
    if (!source || !dest) {
        return EA_ERROR_NULL_POINTER;
    }

    if (dest_size < source_size * 2) {
        return EA_ERROR_BUFFER_TOO_SMALL;
    }

    int opts = 0;
    int compressed_size = REF_encode(dest, source, source_size, &opts);
    
    if (compressed_size <= 0) {
        return EA_ERROR_COMPRESS;
    }

    return compressed_size;
}

/**
 * Compress data with BTREE format
 * @param source Source data to compress
 * @param source_size Size of source data
 * @param dest Destination buffer (should be at least source_size * 2)
 * @param dest_size Size of destination buffer
 * @return Compressed size or negative error code
 */
EA_EXPORT int ea_compress_btree(
    const unsigned char *source,
    int source_size,
    unsigned char *dest,
    int dest_size)
{
    if (!source || !dest) {
        return EA_ERROR_NULL_POINTER;
    }

    if (dest_size < source_size * 2) {
        return EA_ERROR_BUFFER_TOO_SMALL;
    }

    int opts = 0;
    int compressed_size = BTREE_encode(dest, source, source_size, &opts);
    
    if (compressed_size <= 0) {
        return EA_ERROR_COMPRESS;
    }

    return compressed_size;
}

/**
 * Get version string
 * @return Version string
 */
EA_EXPORT const char* ea_version() {
    return "EA Compression Library 1.0.0";
}

} // extern "C"
