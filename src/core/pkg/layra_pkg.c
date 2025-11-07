#include "layra_pkg.h"
#include "layra_vfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

// Placeholder for crypto functions - will be implemented later
// For now, assume content is not encrypted or use dummy decryption
static void dummy_decrypt(uint8_t* data, size_t size) {
    // No-op for now
}

bool layra_pkg_parse_header(FILE* pkg_file, layra_pkg_header_t* header) {
    if (fread(header, sizeof(layra_pkg_header_t), 1, pkg_file) != 1) {
        fprintf(stderr, "Error reading PKG header.\n");
        return false;
    }

    // Convert big-endian fields to host endianness
    header->pkg_magic = BE32_TO_HOST(header->pkg_magic);
    header->pkg_type = BE32_TO_HOST(header->pkg_type);
    header->pkg_0x008 = BE32_TO_HOST(header->pkg_0x008);
    header->pkg_file_count = BE32_TO_HOST(header->pkg_file_count);
    header->pkg_entry_count = BE32_TO_HOST(header->pkg_entry_count);
    header->pkg_sc_entry_count = BE32_TO_HOST(header->pkg_sc_entry_count);
    header->pkg_entry_count_2 = BE32_TO_HOST(header->pkg_entry_count_2);
    header->pkg_table_offset = BE32_TO_HOST(header->pkg_table_offset);
    header->pkg_entry_data_size = BE32_TO_HOST(header->pkg_entry_data_size);
    header->pkg_body_offset = BE64_TO_HOST(header->pkg_body_offset);
    header->pkg_body_size = BE64_TO_HOST(header->pkg_body_size);
    header->pkg_content_offset = BE64_TO_HOST(header->pkg_content_offset);
    header->pkg_content_size = BE64_TO_HOST(header->pkg_content_size);
    // header->pkg_content_id is char array, no endian conversion
    // header->pkg_padding is char array, no endian conversion
    header->pkg_drm_type = BE32_TO_HOST(header->pkg_drm_type);
    header->pkg_content_type = BE32_TO_HOST(header->pkg_content_type);
    header->pkg_content_flags = BE32_TO_HOST(header->pkg_content_flags);
    header->pkg_promote_size = BE32_TO_HOST(header->pkg_promote_size);
    header->pkg_version_date = BE32_TO_HOST(header->pkg_version_date);
    header->pkg_version_hash = BE32_TO_HOST(header->pkg_version_hash);
    header->pkg_0x088 = BE32_TO_HOST(header->pkg_0x088);
    header->pkg_0x08C = BE32_TO_HOST(header->pkg_0x08C);
    header->pkg_0x090 = BE32_TO_HOST(header->pkg_0x090);
    header->pkg_0x094 = BE32_TO_HOST(header->pkg_0x094);
    header->pkg_iro_tag = BE32_TO_HOST(header->pkg_iro_tag);
    header->pkg_drm_type_version = BE32_TO_HOST(header->pkg_drm_type_version);
    // digest_entries1, digest_entries2, digest_table_digest, digest_body_digest are char arrays
    header->pkg_0x400 = BE32_TO_HOST(header->pkg_0x400);
    header->pfs_image_count = BE32_TO_HOST(header->pfs_image_count);
    header->pfs_image_flags = BE64_TO_HOST(header->pfs_image_flags);
    header->pfs_image_offset = BE64_TO_HOST(header->pfs_image_offset);
    header->pfs_image_size = BE64_TO_HOST(header->pfs_image_size);
    header->mount_image_offset = BE64_TO_HOST(header->mount_image_offset);
    header->mount_image_size = BE64_TO_HOST(header->mount_image_size);
    header->pkg_size = BE64_TO_HOST(header->pkg_size);
    header->pfs_signed_size = BE32_TO_HOST(header->pfs_signed_size);
    header->pfs_cache_size = BE32_TO_HOST(header->pfs_cache_size);
    // pfs_image_digest, pfs_signed_digest, pkg_digest are char arrays
    header->pfs_split_size_nth_0 = BE64_TO_HOST(header->pfs_split_size_nth_0);
    header->pfs_split_size_nth_1 = BE64_TO_HOST(header->pfs_split_size_nth_1);

    if (header->pkg_magic != 0x7F434E54) { // 'CNTP'
        fprintf(stderr, "Invalid PKG magic: 0x%X\n", header->pkg_magic);
        return false;
    }

    return true;
}

bool layra_pkg_parse_entries(FILE* pkg_file, layra_pkg_header_t* header, layra_pkg_entry_t** entries) {
    if (fseek(pkg_file, header->pkg_table_offset, SEEK_SET) != 0) {
        fprintf(stderr, "Error seeking to PKG table offset.\n");
        return false;
    }

    uint32_t num_entries = header->pkg_file_count; // Assuming pkg_file_count is the number of entries
    *entries = (layra_pkg_entry_t*)malloc(num_entries * sizeof(layra_pkg_entry_t));
    if (*entries == NULL) {
        fprintf(stderr, "Memory allocation failed for PKG entries.\n");
        return false;
    }

    for (uint32_t i = 0; i < num_entries; ++i) {
        if (fread(&(*entries)[i], sizeof(layra_pkg_entry_t), 1, pkg_file) != 1) {
            fprintf(stderr, "Error reading PKG entry %u.\n", i);
            free(*entries);
            *entries = NULL;
            return false;
        }
        // Convert big-endian fields to host endianness
        (*entries)[i].id = BE32_TO_HOST((*entries)[i].id);
        (*entries)[i].filename_offset = BE32_TO_HOST((*entries)[i].filename_offset);
        (*entries)[i].flags1 = BE32_TO_HOST((*entries)[i].flags1);
        (*entries)[i].flags2 = BE32_TO_HOST((*entries)[i].flags2);
        (*entries)[i].offset = BE32_TO_HOST((*entries)[i].offset);
        (*entries)[i].size = BE32_TO_HOST((*entries)[i].size);
        (*entries)[i].padding = BE64_TO_HOST((*entries)[i].padding);
    }

    return true;
}

// Function to extract and decrypt a single file from PKG
bool layra_pkg_extract_file(FILE* pkg_file, const layra_pkg_entry_t* entry, const char* output_path) {
    // Create parent directories if they don't exist
    char* dir_path = strdup(output_path);
    char* last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0';
        // Check if directory exists, if not, create it
        struct stat st = {0};
        if (stat(dir_path, &st) == -1) {
            // Directory does not exist, create it
            // This is a simplified version, real implementation might need recursive creation
            if (mkdir(dir_path, 0777) == -1 && errno != EEXIST) {
                fprintf(stderr, "Error creating directory %s: %s\n", dir_path, strerror(errno));
                free(dir_path);
                return false;
            }
        }
    }
    free(dir_path);

    FILE* out_file = fopen(output_path, "wb");
    if (!out_file) {
        fprintf(stderr, "Error opening output file %s: %s\n", output_path, strerror(errno));
        return false;
    }

    if (fseek(pkg_file, entry->offset, SEEK_SET) != 0) {
        fprintf(stderr, "Error seeking to file data in PKG.\n");
        fclose(out_file);
        return false;
    }

    uint8_t* buffer = (uint8_t*)malloc(entry->size);
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed for file buffer.\n");
        fclose(out_file);
        return false;
    }

    if (fread(buffer, 1, entry->size, pkg_file) != entry->size) {
        fprintf(stderr, "Error reading file data from PKG.\n");
        free(buffer);
        fclose(out_file);
        return false;
    }

    // Dummy decryption for now
    dummy_decrypt(buffer, entry->size);

    if (fwrite(buffer, 1, entry->size, out_file) != entry->size) {
        fprintf(stderr, "Error writing data to output file.\n");
        free(buffer);
        fclose(out_file);
        return false;
    }

    free(buffer);
    fclose(out_file);
    return true;
}

// Main function to open, parse, extract, and mount a PKG file
bool layra_pkg_open_and_mount(const char* pkg_filepath, const char* mount_point) {
    FILE* pkg_file = fopen(pkg_filepath, "rb");
    if (!pkg_file) {
        fprintf(stderr, "Error opening PKG file %s.\n", pkg_filepath);
        return false;
    }

    layra_pkg_header_t header;
    if (!layra_pkg_parse_header(pkg_file, &header)) {
        fclose(pkg_file);
        return false;
    }

    layra_pkg_entry_t* entries = NULL;
    if (!layra_pkg_parse_entries(pkg_file, &header, &entries)) {
        fclose(pkg_file);
        return false;
    }

    // Create a temporary directory for extraction
    char temp_dir_template[] = "/tmp/layra_pkg_XXXXXX";
    char* temp_dir = mkdtemp(temp_dir_template);
    if (!temp_dir) {
        fprintf(stderr, "Error creating temporary directory: %s\n", strerror(errno));
        free(entries);
        fclose(pkg_file);
        return false;
    }
    fprintf(stdout, "Created temporary extraction directory: %s\n", temp_dir);

    // Extract all files (for now, without proper filename resolution)
    for (uint32_t i = 0; i < header.pkg_file_count; ++i) {
        char output_path[MAX_VFS_PATH];
        // Simplified filename for now, real implementation needs filename table lookup
        snprintf(output_path, MAX_VFS_PATH, "%s/file_%u.bin", temp_dir, i);
        if (!layra_pkg_extract_file(pkg_file, &entries[i], output_path)) {
            fprintf(stderr, "Error extracting file %u.\n", i);
            // Clean up temp directory and allocated memory
            // TODO: Implement proper cleanup for temp_dir
            free(entries);
            fclose(pkg_file);
            return false;
        }
    }

    free(entries);
    fclose(pkg_file);

    // Mount the extracted content to the VFS
    if (!layra_vfs_mount(mount_point, temp_dir)) {
        fprintf(stderr, "Error mounting %s to VFS.\n", temp_dir);
        // TODO: Implement proper cleanup for temp_dir
        return false;
    }

    return true;
}

