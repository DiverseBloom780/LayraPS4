#include "../../layra_pkg.h"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>


namespace fs = std::filesystem;

// Placeholder for crypto functions - will be implemented later
static void dummy_decrypt(uint8_t *data, size_t size) {
  // No-op for now
}

bool layra_pkg_parse_header(FILE *pkg_file, layra_pkg_header_t *header) {
  if (fread(header, sizeof(layra_pkg_header_t), 1, pkg_file) != 1) {
    std::cerr << "Error reading PKG header.\n";
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
  header->pkg_drm_type = BE32_TO_HOST(header->pkg_drm_type);
  header->pkg_content_type = BE32_TO_HOST(header->pkg_content_type);
  header->pkg_content_flags = BE32_TO_HOST(header->pkg_content_flags);
  header->pkg_promote_size = BE32_TO_HOST(header->pkg_promote_size);
  header->pkg_version_date = BE32_TO_HOST(header->pkg_version_date);
  header->pkg_version_hash = BE32_TO_HOST(header->pkg_version_hash);
  header->pkg_iro_tag = BE32_TO_HOST(header->pkg_iro_tag);
  header->pkg_drm_type_version = BE32_TO_HOST(header->pkg_drm_type_version);
  header->pkg_size = BE64_TO_HOST(header->pkg_size);
  header->pfs_image_offset = BE64_TO_HOST(header->pfs_image_offset);
  header->pfs_image_size = BE64_TO_HOST(header->pfs_image_size);

  if (header->pkg_magic != 0x7F434E54) { // 'CNTP'
    std::cerr << "Invalid PKG magic: 0x" << std::hex << header->pkg_magic
              << "\n";
    return false;
  }

  return true;
}

bool layra_pkg_parse_entries(FILE *pkg_file, layra_pkg_header_t *header,
                             layra_pkg_entry_t **entries) {
  if (fseek(pkg_file, header->pkg_table_offset, SEEK_SET) != 0) {
    std::cerr << "Error seeking to PKG table offset.\n";
    return false;
  }

  uint32_t num_entries = header->pkg_file_count;
  *entries =
      (layra_pkg_entry_t *)malloc(num_entries * sizeof(layra_pkg_entry_t));
  if (*entries == NULL) {
    std::cerr << "Memory allocation failed for PKG entries.\n";
    return false;
  }

  for (uint32_t i = 0; i < num_entries; ++i) {
    if (fread(&(*entries)[i], sizeof(layra_pkg_entry_t), 1, pkg_file) != 1) {
      std::cerr << "Error reading PKG entry " << i << ".\n";
      free(*entries);
      *entries = nullptr;
      return false;
    }
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

bool layra_pkg_extract_file(FILE *pkg_file, const layra_pkg_entry_t *entry,
                            const char *output_path) {
  fs::path out_path(output_path);
  try {
    if (out_path.has_parent_path()) {
      fs::create_directories(out_path.parent_path());
    }
  } catch (const fs::filesystem_error &e) {
    std::cerr << "Error creating directory: " << e.what() << "\n";
    return false;
  }

  std::ofstream out(output_path, std::ios::binary);
  if (!out) {
    std::cerr << "Error opening output file " << output_path << "\n";
    return false;
  }

  if (fseek(pkg_file, entry->offset, SEEK_SET) != 0) {
    std::cerr << "Error seeking to file data in PKG.\n";
    return false;
  }

  std::vector<uint8_t> buffer(entry->size);
  if (fread(buffer.data(), 1, entry->size, pkg_file) != entry->size) {
    std::cerr << "Error reading file data from PKG.\n";
    return false;
  }

  dummy_decrypt(buffer.data(), entry->size);
  out.write(reinterpret_cast<const char *>(buffer.data()), entry->size);
  return out.good();
}

bool layra_pkg_extract_to_directory(const char *pkg_filepath,
                                     const char *output_dir) {
  FILE *pkg_file = fopen(pkg_filepath, "rb");
  if (!pkg_file) {
    std::cerr << "Error opening PKG file " << pkg_filepath << "\n";
    return false;
  }

  layra_pkg_header_t header;
  if (!layra_pkg_parse_header(pkg_file, &header)) {
    fclose(pkg_file);
    return false;
  }

  layra_pkg_entry_t *entries = nullptr;
  if (!layra_pkg_parse_entries(pkg_file, &header, &entries)) {
    fclose(pkg_file);
    return false;
  }

  std::vector<char> filename_table;
  for (uint32_t i = 0; i < header.pkg_file_count; ++i) {
    if (entries[i].id == PKG_ENTRY_ID_FILENAMES) {
      filename_table.resize(entries[i].size);
      if (fseek(pkg_file, entries[i].offset, SEEK_SET) != 0) {
        std::cerr << "Error seeking to filename table in PKG.\n";
        free(entries);
        fclose(pkg_file);
        return false;
      }
      if (fread(filename_table.data(), 1, entries[i].size, pkg_file) !=
          entries[i].size) {
        std::cerr << "Error reading filename table from PKG.\n";
        free(entries);
        fclose(pkg_file);
        return false;
      }
      break;
    }
  }

  fs::path dest_dir = fs::path(output_dir);
  try {
    fs::create_directories(dest_dir);
  } catch (const fs::filesystem_error &e) {
    std::cerr << "Error creating extraction directory: " << e.what() << "\n";
    free(entries);
    fclose(pkg_file);
    return false;
  }

  for (uint32_t i = 0; i < header.pkg_file_count; ++i) {
    std::string filename;
    if (!filename_table.empty() &&
        entries[i].filename_offset < filename_table.size()) {
      filename = std::string(filename_table.data() + entries[i].filename_offset);
    } else {
      filename = "file_" + std::to_string(i) + ".bin";
    }

    fs::path output_path = dest_dir / filename;
    if (!layra_pkg_extract_file(pkg_file, &entries[i],
                                output_path.string().c_str())) {
      std::cerr << "Error extracting file " << i << " (" << filename << ").\n";
    }
  }

  free(entries);
  fclose(pkg_file);
  return true;
}

bool layra_pkg_open_and_mount(const char *pkg_filepath,
                              const char *mount_point) {
  FILE *pkg_file = fopen(pkg_filepath, "rb");
  if (!pkg_file) {
    std::cerr << "Error opening PKG file " << pkg_filepath << "\n";
    return false;
  }

  layra_pkg_header_t header;
  if (!layra_pkg_parse_header(pkg_file, &header)) {
    fclose(pkg_file);
    return false;
  }

  layra_pkg_entry_t *entries = nullptr;
  if (!layra_pkg_parse_entries(pkg_file, &header, &entries)) {
    fclose(pkg_file);
    return false;
  }

  // Find filename table
  std::vector<char> filename_table;
  for (uint32_t i = 0; i < header.pkg_file_count; ++i) {
    if (entries[i].id == PKG_ENTRY_ID_FILENAMES) {
      filename_table.resize(entries[i].size);
      fseek(pkg_file, entries[i].offset, SEEK_SET);
      fread(filename_table.data(), 1, entries[i].size, pkg_file);
      break;
    }
  }

  // Use the requested mount point so callers can control extraction location.
  fs::path temp_dir = fs::path(mount_point);
  try {
    fs::create_directories(temp_dir);
  } catch (const fs::filesystem_error &e) {
    std::cerr << "Error creating temp directory: " << e.what() << "\n";
    free(entries);
    fclose(pkg_file);
    return false;
  }

  std::cout << "Extraction directory: " << temp_dir << "\n";

  // Extract all files
  for (uint32_t i = 0; i < header.pkg_file_count; ++i) {
    std::string filename;
    if (!filename_table.empty() &&
        entries[i].filename_offset < filename_table.size()) {
      filename =
          std::string(filename_table.data() + entries[i].filename_offset);
    } else {
      filename = "file_" + std::to_string(i) + ".bin";
    }

    fs::path output_path = temp_dir / filename;

    if (!layra_pkg_extract_file(pkg_file, &entries[i],
                                output_path.string().c_str())) {
      std::cerr << "Error extracting file " << i << " (" << filename << ").\n";
    }
  }

  free(entries);
  fclose(pkg_file);
  return true;
}
