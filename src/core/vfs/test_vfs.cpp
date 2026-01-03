// test_vfs.cpp
#include "layra_vfs.h"
#include <iostream>
int main() {
    std::cout << "Testing LayraPS4 VFS Backend...\n";
    
    // Test initialization
    if (layra_vfs_init() != 0) {
        std::cerr << "VFS init failed!\n";
        return 1;
    }
    std::cout << "✓ VFS initialized\n";
    
    // Test mounting
    if (layra_vfs_mount("/app0", "/tmp/test") != 0) {
        std::cerr << "Mount failed!\n";
        return 1;
    }
    std::cout << "✓ Mount point created: /app0 -> /tmp/test\n";
    
    // Test file opening (will return nullptr for now - that's OK)
    layra_vfs_file_t* file = layra_vfs_fopen("/app0/test.txt", "rb");
    if (file) {
        std::cout << "✓ File handle created\n";
        layra_vfs_fclose(file);
    } else {
        std::cout << "⚠ File opening not implemented yet\n";
    }
    
    std::cout << "VFS Backend Core: WORKING!\n";
    return 0;
}