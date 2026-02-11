// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Type definitions for PS4 compatibility
typedef int64_t  s64;
typedef int32_t  s32;
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

// Address types
typedef uint64_t VAddr;  // Virtual Address
typedef uint64_t PAddr;  // Physical Address

// Orbis error codes
#define ORBIS_OK                              0
#define ORBIS_KERNEL_ERROR_EINVAL             0x80020016
#define ORBIS_KERNEL_ERROR_EAGAIN             0x80020023
#define ORBIS_KERNEL_ERROR_ENOMEM             0x8002000C
#define ORBIS_KERNEL_ERROR_EFAULT             0x8002000E
#define ORBIS_KERNEL_ERROR_EACCES             0x8002000D
#define ORBIS_KERNEL_ERROR_ENAMETOOLONG       0x8002003F

// Memory constants
#define ORBIS_KERNEL_MAXIMUM_NAME_LENGTH      32
#define PAGE_SIZE_16KB                        (16 * 1024)
#define PAGE_SIZE_64KB                        (64 * 1024)
#define PAGE_SIZE_2MB                         (2 * 1024 * 1024)

// Memory operation types
typedef enum {
    ORBIS_KERNEL_MAP_OP_MAP_DIRECT = 0,
    ORBIS_KERNEL_MAP_OP_UNMAP = 1,
    ORBIS_KERNEL_MAP_OP_PROTECT = 2,
    ORBIS_KERNEL_MAP_OP_MAP_FLEXIBLE = 3,
    ORBIS_KERNEL_MAP_OP_TYPE_PROTECT = 4,
} OrbisMemoryOpType;

// Memory flags
typedef enum {
    ORBIS_KERNEL_MAP_FIXED = 0x0010,
    ORBIS_KERNEL_MAP_NO_OVERWRITE = 0x0080,
    ORBIS_KERNEL_MAP_NO_COALESCE = 0x00400000,
} OrbisMemoryFlags;

// Memory pool opcodes
typedef enum {
    ORBIS_MEMORY_POOL_COMMIT = 0,
    ORBIS_MEMORY_POOL_DECOMMIT = 1,
    ORBIS_MEMORY_POOL_PROTECT = 2,
    ORBIS_MEMORY_POOL_TYPE_PROTECT = 3,
    ORBIS_MEMORY_POOL_MOVE = 4,
} OrbisMemoryPoolOpcode;

// Virtual query info structure
typedef struct {
    void* start;
    void* end;
    s64 offset;
    s32 protection;
    s32 memory_type;
    u32 is_flexible;
    u32 is_direct;
    u32 is_stack;
    u32 is_pooled;
    u32 is_committed;
    char name[ORBIS_KERNEL_MAXIMUM_NAME_LENGTH];
} OrbisVirtualQueryInfo;

// Direct memory query info
typedef struct {
    u64 start;
    u64 end;
    s32 memory_type;
} OrbisQueryInfo;

// Batch map entry
typedef struct {
    void* start;
    u64 offset;
    u64 length;
    s32 protection;
    s32 type;
    s32 operation;
} OrbisKernelBatchMapEntry;

// Memory pool batch entry
typedef struct {
    s32 opcode;
    s32 flags;
    union {
        struct {
            void* addr;
            u64 len;
            s32 type;
            s32 prot;
        } commit_params;
        struct {
            void* addr;
            u64 len;
        } decommit_params;
        struct {
            void* addr;
            u64 len;
            s32 prot;
        } protect_params;
        struct {
            void* addr;
            u64 len;
            s32 type;
            s32 prot;
        } type_protect_params;
    };
} OrbisKernelMemoryPoolBatchEntry;

// Memory pool block stats
typedef struct {
    u64 size;
    u64 free_size;
    u64 committed_size;
} OrbisKernelMemoryPoolBlockStats;

// Core PS4 syscalls
u64 sceKernelGetDirectMemorySize(void);
s32 sceKernelEnableDmemAliasing(void);
s32 sceKernelAllocateDirectMemory(s64 searchStart, s64 searchEnd, u64 len,
                                  u64 alignment, s32 memoryType, s64* physAddrOut);
s32 sceKernelAllocateMainDirectMemory(u64 len, u64 alignment, s32 memoryType, s64* physAddrOut);
s32 sceKernelCheckedReleaseDirectMemory(u64 start, u64 len);
s32 sceKernelReleaseDirectMemory(u64 start, u64 len);
s32 sceKernelAvailableDirectMemorySize(u64 searchStart, u64 searchEnd, u64 alignment,
                                      u64* physAddrOut, u64* sizeOut);
s32 sceKernelVirtualQuery(const void* addr, s32 flags, OrbisVirtualQueryInfo* info, u64 infoSize);
s32 sceKernelReserveVirtualRange(void** addr, u64 len, s32 flags, u64 alignment);
s32 sceKernelMapNamedDirectMemory(void** addr, u64 len, s32 prot, s32 flags,
                                  s64 phys_addr, u64 alignment, const char* name);
s32 sceKernelMapDirectMemory(void** addr, u64 len, s32 prot, s32 flags, s64 phys_addr, u64 alignment);
s32 sceKernelMapDirectMemory2(void** addr, u64 len, s32 type, s32 prot, s32 flags,
                              s64 phys_addr, u64 alignment);
s32 sceKernelMapNamedFlexibleMemory(void** addr_in_out, u64 len, s32 prot, s32 flags, const char* name);
s32 sceKernelMapFlexibleMemory(void** addr_in_out, u64 len, s32 prot, s32 flags);
s32 sceKernelQueryMemoryProtection(void* addr, void** start, void** end, u32* prot);
s32 sceKernelMprotect(const void* addr, u64 size, s32 prot);
s32 sceKernelMtypeprotect(const void* addr, u64 size, s32 mtype, s32 prot);
s32 sceKernelDirectMemoryQuery(u64 offset, s32 flags, OrbisQueryInfo* query_info, u64 infoSize);
s32 sceKernelAvailableFlexibleMemorySize(u64* out_size);
s32 sceKernelGetDirectMemoryType(u64 addr, s32* directMemoryTypeOut,
                                void** directMemoryStartOut, void** directMemoryEndOut);
s32 sceKernelIsStack(void* addr, void** start, void** end);
u32 sceKernelIsAddressSanitizerEnabled(void);
s32 sceKernelBatchMap(OrbisKernelBatchMapEntry* entries, s32 numEntries, s32* numEntriesOut);
s32 sceKernelBatchMap2(OrbisKernelBatchMapEntry* entries, s32 numEntries, s32* numEntriesOut, s32 flags);
s32 sceKernelSetVirtualRangeName(const void* addr, u64 len, const char* name);
s32 sceKernelConfiguredFlexibleMemorySize(u64* sizeOut);
s32 sceKernelMunmap(void* addr, u64 len);
s32 sceKernelMmap(void* addr, u64 len, s32 prot, s32 flags, s32 fd, s64 phys_addr, void** res);

// Memory pool functions
s32 sceKernelMemoryPoolExpand(u64 searchStart, u64 searchEnd, u64 len, u64 alignment, u64* physAddrOut);
s32 sceKernelMemoryPoolReserve(void* addr_in, u64 len, u64 alignment, s32 flags, void** addr_out);
s32 sceKernelMemoryPoolCommit(void* addr, u64 len, s32 type, s32 prot, s32 flags);
s32 sceKernelMemoryPoolDecommit(void* addr, u64 len, s32 flags);
s32 sceKernelMemoryPoolBatch(const OrbisKernelMemoryPoolBatchEntry* entries, s32 count,
                            s32* num_processed, s32 flags);
s32 sceKernelMemoryPoolGetBlockStats(OrbisKernelMemoryPoolBlockStats* stats, u64 size);

// POSIX compatibility
void* posix_mmap(void* addr, u64 len, s32 prot, s32 flags, s32 fd, s64 phys_addr);
s32 posix_mprotect(const void* addr, u64 size, s32 prot);
s32 posix_munmap(void* addr, u64 len);

// PRT (Page Remapping Table) memory
s32 sceKernelSetPrtAperture(s32 id, VAddr address, u64 size);
s32 sceKernelGetPrtAperture(s32 id, VAddr* address, u64* size);

// Internal function for RTLD
void _sceKernelRtldSetApplicationHeapAPI(void* func[]);

// Registration function
void RegisterKernelMemory(void* symbol_resolver);

#ifdef __cplusplus
}
#endif
