// EA Compression Library C API Header
// This header provides a C-compatible interface for the EA compression library

#ifndef EA_COMPRESSION_LIB_H
#define EA_COMPRESSION_LIB_H

#ifdef __cplusplus
extern "C" {
#endif

// Export symbols for shared library
#ifdef _WIN32
    #define EA_EXPORT __declspec(dllexport)
#else
    #define EA_EXPORT __attribute__((visibility("default")))
#endif

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
EA_EXPORT ea_format_t ea_detect_format(const unsigned char *data, int size);

/**
 * Get decompressed size from compressed data
 * @param data Pointer to compressed data
 * @param size Size of compressed data
 * @return Decompressed size or -1 on error
 */
EA_EXPORT int ea_get_decompressed_size(const unsigned char *data, int size);

/**
 * Decompress data (automatically detects format)
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
    int decompressed_size);

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
    int huff_type);

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
    int dest_size);

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
    int dest_size);

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
    int dest_size);

/**
 * Get version string
 * @return Version string
 */
EA_EXPORT const char* ea_version();

#ifdef __cplusplus
}
#endif

#endif // EA_COMPRESSION_LIB_H
