#ifndef LAYRA_PKG_H
#define LAYRA_PKG_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

// Define byte-swapping macros for big-endian to host conversion
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define BE32_TO_HOST(x) __builtin_bswap32(x)
#define BE64_TO_HOST(x) __builtin_bswap64(x)
#else
#define BE32_TO_HOST(x) (x)
#define BE64_TO_HOST(x) (x)
#endif

// PKG Header Structure (Big-Endian)
typedef struct {
    uint32_t pkg_magic;                      // 0x000 - 0x7F434E54 ('CNTP')
    uint32_t pkg_type;                       // 0x004
    uint32_t pkg_0x008;                      // 0x008 - unknown field
    uint32_t pkg_file_count;                 // 0x00C
    uint32_t pkg_entry_count;                // 0x010
    uint16_t pkg_sc_entry_count;             // 0x014
    uint16_t pkg_entry_count_2;              // 0x016 - same as pkg_entry_count
    uint32_t pkg_table_offset;               // 0x018 - file table offset
    uint32_t pkg_entry_data_size;            // 0x01C
    uint64_t pkg_body_offset;                // 0x020 - offset of PKG entries
    uint64_t pkg_body_size;                  // 0x028 - length of all PKG entries
    uint64_t pkg_content_offset;             // 0x030
    uint64_t pkg_content_size;               // 0x038
    unsigned char pkg_content_id[0x24];      // 0x040 - packages' content ID as a 36-byte string
    unsigned char pkg_padding[0xC];          // 0x064 - padding
    uint32_t pkg_drm_type;                   // 0x070 - DRM type
    uint32_t pkg_content_type;               // 0x074 - Content type
    uint32_t pkg_content_flags;              // 0x078 - Content flags
    uint32_t pkg_promote_size;               // 0x07C
    uint32_t pkg_version_date;               // 0x080
    uint32_t pkg_version_hash;               // 0x084
    uint32_t pkg_0x088;                      // 0x088
    uint32_t pkg_0x08C;                      // 0x08C
    uint32_t pkg_0x090;                      // 0x090
    uint32_t pkg_0x094;                      // 0x094
    uint32_t pkg_iro_tag;                    // 0x098
    uint32_t pkg_drm_type_version;           // 0x09C
    unsigned char pkg_zeroes_1[0x60];        // 0x0A0
    unsigned char digest_entries1[0x20];     // 0x100 - sha256 digest for main entry 1
    unsigned char digest_entries2[0x20];     // 0x120 - sha256 digest for main entry 2
    unsigned char digest_table_digest[0x20]; // 0x140 - sha256 digest for digest table
    unsigned char digest_body_digest[0x20];  // 0x160 - sha256 digest for main table
    unsigned char pkg_zeroes_2[0x280];       // 0x180
    uint32_t pkg_0x400;                      // 0x400
    uint32_t pfs_image_count;                // 0x404 - count of PFS images
    uint64_t pfs_image_flags;                // 0x408 - PFS flags
    uint64_t pfs_image_offset;               // 0x410 - offset to start of external PFS image
    uint64_t pfs_image_size;                 // 0x418 - size of external PFS image
    uint64_t mount_image_offset;             // 0x420
    uint64_t mount_image_size;               // 0x428
    uint64_t pkg_size;                       // 0x430
    uint32_t pfs_signed_size;                // 0x438
    uint32_t pfs_cache_size;                 // 0x43C
    unsigned char pfs_image_digest[0x20];    // 0x440
    unsigned char pfs_signed_digest[0x20];   // 0x460
    uint64_t pfs_split_size_nth_0;           // 0x480
    uint64_t pfs_split_size_nth_1;           // 0x488
    unsigned char pkg_zeroes_3[0xB50];       // 0x490
    unsigned char pkg_digest[0x20];          // 0xFE0
} layra_pkg_header_t;                        // Total size: 0x1000 (4096 bytes)

// PKG Table Entry Structure (Big-Endian)
typedef struct {
    uint32_t id;               // File ID, useful for files without a filename entry
    uint32_t filename_offset;  // Offset into the filenames table (ID 0x200) where this file's name is located
    uint32_t flags1;           // Flags including encrypted flag, etc
    uint32_t flags2;           // Flags including encryption key index, etc
    uint32_t offset;           // Offset into PKG to find the file
    uint32_t size;             // Size of data
    uint64_t padding;          // blank padding
} layra_pkg_entry_t;

// Function to parse PKG header
bool layra_pkg_parse_header(FILE* pkg_file, layra_pkg_header_t* header);

// Function to parse PKG file entries
bool layra_pkg_parse_entries(FILE* pkg_file, layra_pkg_header_t* header, layra_pkg_entry_t** entries);

// Function to open, parse, extract, and mount a PKG file
bool layra_pkg_open_and_mount(const char* pkg_filepath, const char* mount_point);

#endif // LAYRA_PKG_H

