// Example usage of EA Compression Library
// Compile: gcc -o example example.c -lea_compression
// Or: gcc -o example example.c -L. -lea_compression

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ea_compression_lib.h"

void print_format(ea_format_t format) {
    switch (format) {
        case EA_FORMAT_HUFF:   printf("HUFF\n"); break;
        case EA_FORMAT_JDLZ:   printf("JDLZ\n"); break;
        case EA_FORMAT_REF:    printf("REF\n"); break;
        case EA_FORMAT_BTREE:  printf("BTREE\n"); break;
        case EA_FORMAT_COMP:   printf("COMP\n"); break;
        default:               printf("UNKNOWN\n"); break;
    }
}

int main(int argc, char *argv[]) {
    printf("EA Compression Library Example\n");
    printf("Version: %s\n\n", ea_version());

    if (argc != 3) {
        printf("Usage: %s <compress|decompress> <file>\n", argv[0]);
        return 1;
    }

    const char *mode = argv[1];
    const char *filename = argv[2];

    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Error: Cannot open file '%s'\n", filename);
        return 1;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    int file_size = ftell(file);
    rewind(file);

    // Read file
    unsigned char *file_data = malloc(file_size);
    if (!file_data) {
        printf("Error: Cannot allocate memory\n");
        fclose(file);
        return 1;
    }
    fread(file_data, 1, file_size, file);
    fclose(file);

    if (strcmp(mode, "decompress") == 0) {
        // Decompress mode
        printf("Detecting format...\n");
        ea_format_t format = ea_detect_format(file_data, file_size);
        printf("Format: ");
        print_format(format);

        if (format == EA_FORMAT_UNKNOWN) {
            printf("Error: Unknown compression format\n");
            free(file_data);
            return 1;
        }

        int decompressed_size = ea_get_decompressed_size(file_data, file_size);
        if (decompressed_size <= 0) {
            printf("Error: Cannot determine decompressed size\n");
            free(file_data);
            return 1;
        }

        printf("Compressed size: %d bytes\n", file_size);
        printf("Decompressed size: %d bytes\n", decompressed_size);

        unsigned char *decompressed = malloc(decompressed_size);
        if (!decompressed) {
            printf("Error: Cannot allocate memory for decompression\n");
            free(file_data);
            return 1;
        }

        printf("Decompressing...\n");
        int result = ea_decompress(file_data, file_size, decompressed, decompressed_size);
        
        if (result > 0) {
            printf("Success! Decompressed %d bytes\n", result);
            
            // Write output
            char outname[256];
            snprintf(outname, sizeof(outname), "%s.decompressed", filename);
            FILE *outfile = fopen(outname, "wb");
            if (outfile) {
                fwrite(decompressed, 1, result, outfile);
                fclose(outfile);
                printf("Output written to: %s\n", outname);
            }
        } else {
            printf("Error: Decompression failed with code %d\n", result);
        }

        free(decompressed);

    } else if (strcmp(mode, "compress") == 0) {
        // Compress mode (example with HUFF)
        printf("Compressing with HUFF format...\n");
        printf("Input size: %d bytes\n", file_size);

        int max_compressed = file_size * 2 + 16;
        unsigned char *compressed = malloc(max_compressed);
        if (!compressed) {
            printf("Error: Cannot allocate memory for compression\n");
            free(file_data);
            return 1;
        }

        int result = ea_compress_huff(file_data, file_size, compressed, max_compressed, 0);
        
        if (result > 0) {
            printf("Success! Compressed to %d bytes (%.2f%% of original)\n", 
                   result, (result * 100.0) / file_size);
            
            // Write output
            char outname[256];
            snprintf(outname, sizeof(outname), "%s.huff", filename);
            FILE *outfile = fopen(outname, "wb");
            if (outfile) {
                fwrite(compressed, 1, result, outfile);
                fclose(outfile);
                printf("Output written to: %s\n", outname);
            }
        } else {
            printf("Error: Compression failed with code %d\n", result);
        }

        free(compressed);

    } else {
        printf("Error: Invalid mode. Use 'compress' or 'decompress'\n");
        free(file_data);
        return 1;
    }

    free(file_data);
    return 0;
}
