# LayraPS4 Emulator - PKG Parsing and Mounting System Design

## 1. Introduction

This document outlines the design for the PlayStation 4 (PS4) PKG file parsing and mounting system within the LayraPS4 emulator. The goal is to accurately parse PKG files, decrypt their contents, and integrate them into a Virtual File System (VFS) to allow the emulator to access game data as if it were on a physical disc or installed application.

## 2. PKG File Format Overview

PS4 PKG files are containers for game data, updates, and DLC. They are based on the PSP, PS3, and PS Vita PKG formats, utilizing a big-endian header structure. The contents within the PKG are encrypted, primarily using XTS-AES with a block size of 0x1000 [1].

### 2.1 Key Structures

Based on the PS4 Developer Wiki [1], the primary structures are `pkg_header` and `pkg_table_entry`.

#### 2.1.1 `pkg_header` Structure

```c
typedef struct {
  uint32_t pkg_magic;                      // 0x000 - 0x7F434E54
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
} pkg_header;                              // Total size: 0x1000 (4096 bytes)
```

#### 2.1.2 `pkg_table_entry` Structure

```c
typedef struct {
  uint32_t id;               // File ID, useful for files without a filename entry
  uint32_t filename_offset;  // Offset into the filenames table (ID 0x200) where this file's name is located
  uint32_t flags1;           // Flags including encrypted flag, etc
  uint32_t flags2;           // Flags including encryption key index, etc
  uint32_t offset;           // Offset into PKG to find the file
  uint32_t size;             // Size of the file
  uint64_t padding;          // blank padding
} pkg_table_entry;
```

### 2.2 Important Fields

*   `pkg_magic`: Always `0x7F434E54` (big-endian representation of `CNTP`).
*   `pkg_file_count`: Number of files contained within the PKG.
*   `pkg_table_offset`: Offset to the file table, which lists all files and their metadata.
*   `pkg_body_offset` and `pkg_body_size`: Define the location and size of the encrypted content section.
*   `pkg_content_id`: A 36-byte string identifying the package content.
*   `pfs_image_offset` and `pfs_image_size`: Define the location and size of the PFS (PlayStation File System) image within the PKG.

## 3. PKG Parsing Process

The parsing process will involve several steps:

1.  **File Opening and Header Reading**: Open the `.pkg` file and read the `pkg_header`. Validate the `pkg_magic` to ensure it's a valid PS4 PKG file.
2.  **Endianness Conversion**: Convert all multi-byte fields from big-endian to the host system's endianness.
3.  **File Table Parsing**: Seek to `pkg_table_offset` and read `pkg_file_count` number of `pkg_table_entry` structures. Store these entries in an in-memory data structure.
4.  **Content ID Extraction**: Extract the `pkg_content_id` for identification and potential use in decryption key derivation.
5.  **Decryption Key Derivation**: This is the most complex part. PS4 PKG files use XTS-AES encryption. The keys are derived from various factors, including a developer-specified passcode and the Content ID [2]. This will require a dedicated crypto module.
6.  **PFS Image Extraction and Decryption**: The actual game data is contained within a PFS image, which is itself encrypted. The `pfs_image_offset` and `pfs_image_size` fields in the header point to this image. This image needs to be extracted and decrypted using the derived keys.
7.  **PFS Mounting**: Once the PFS image is decrypted, it needs to be mounted into the VFS. The PFS contains its own file system structure (inodes, dirents, etc.) which will need to be parsed to expose the individual files and directories to the emulator.

## 4. Virtual File System (VFS) Integration

The VFS will provide a unified interface for the emulator to access files, regardless of whether they originate from a PKG, a physical disc, or a host file system. For PKG files, the process will be:

1.  **Temporary Extraction**: The decrypted contents of the PFS image will be extracted to a temporary directory on the host file system. This simplifies file access for the emulator, as it can then use standard file I/O operations on the temporary directory.
2.  **VFS Mount Point**: The temporary directory will be mounted to a specific virtual path within the VFS, typically `/app0`, which is where PS4 applications expect to find their executable (`eboot.bin`) and data.
3.  **File Access Redirection**: When the emulator requests a file from `/app0`, the VFS will redirect the request to the corresponding file in the temporary host directory.

## 5. Encryption and Decryption Module

A dedicated `crypto` module will be developed to handle the encryption and decryption aspects. This module will implement:

*   **XTS-AES**: The primary encryption algorithm used for PKG contents.
*   **Key Derivation Functions**: Functions to derive the necessary encryption keys from the PKG metadata (e.g., Content ID, passcodes).
*   **Hashing Algorithms**: SHA-256 for integrity checks and key derivation.

## 6. Documentation

Throughout the implementation, comprehensive documentation will be maintained, covering:

*   Detailed C structures for PKG header and entries.
*   Flowcharts and pseudocode for parsing and decryption logic.
*   API documentation for the PKG and VFS modules.
*   Explanation of key derivation and encryption processes.

## 7. References

[1] PS4 Developer Wiki. (n.d.). *PKG files*. Retrieved from [https://www.psdevwiki.com/ps4/PKG_files](https://www.psdevwiki.com/ps4/PKG_files)
[2] PSXHAX. (2020, January 20). *PS4 PKG Information on PlayStation 4 Packages and Keys via Maxton*. Retrieved from [https://www.psxhax.com/threads/ps4-pkg-information-on-playstation-4-packages-and-keys-via-maxton.7280/](https://www.psxhax.com/threads/ps4-pkg-information-on-playstation-4-packages-and-keys-via-maxton.7280/)

