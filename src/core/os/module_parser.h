#pragma once
#include <cstdint>
#include <vector>

namespace PS4 {
namespace OS {

// Simplified PS4 executable header
struct SELFHeader {
    uint32_t magic;          // 0x1D3D154F
    uint32_t version;
    uint64_t flags;
    uint64_t file_size;
    uint32_t segment_count;
    uint32_t header_size;
    uint64_t metadata_offset;
};

struct SegmentInfo {
    uint64_t offset;
    uint64_t size;
    uint64_t compressed_size;
    uint64_t memory_size;
    uint64_t memory_address;
    uint32_t type;
    uint32_t attributes;
};

class ModuleParser {
public:
    ModuleParser();
    
    // Parse SELF file from memory buffer
    bool ParseSELF(const uint8_t* data, size_t size);
    
    // Load segments into memory
    bool LoadSegments(class MemoryManager* memory);
    
    // Get entry point
    uint64_t GetEntryPoint() const { return entry_point; }
    
    // Get needed libraries
    const std::vector<std::string>& GetNeededLibraries() const { return needed_libs; }
    
private:
    SELFHeader header;
    std::vector<SegmentInfo> segments;
    uint64_t entry_point;
    std::vector<std::string> needed_libs;
    
    bool ParseMetadata(const uint8_t* data);
    bool DecryptSegment(const uint8_t* src, uint8_t* dst, 
                       const SegmentInfo& seg);
};

} // namespace OS
} // namespace PS4
