#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H
class MemoryManager {
public:
    void init();
    uint8_t* allocateMemory(uint32_t size);
    void update();
private:
    uint8_t* memory;
};

#endif  // MEMORY_MANAGER_H
