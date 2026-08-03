// sysv_abi_wrapper.h
// ABI wrapper for converting System V AMD64 (PS4) calls to Microsoft x64 (MSVC)
//
// On MSVC, __attribute__((sysv_abi)) is not supported.
// Instead, we generate small executable trampolines at runtime.
// Each trampoline:
//   1. Loads R11 with the real MSVC host function pointer
//   2. Jumps to the shared sysv_to_msvc_thunk (in .asm)
//
// The thunk then shuffles SysV register args to MSVC register args
// and calls the target function.

#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

// Declared in sysv_abi_wrapper.asm
extern "C" void sysv_to_msvc_thunk();

namespace Core {

// Manages runtime-generated ABI wrapper trampolines.
// Each trampoline is a small piece of executable code that:
//   mov r11, <target_msvc_func_ptr>   ; 10 bytes (REX.W + MOV r11, imm64)
//   jmp <sysv_to_msvc_thunk>          ; 12 bytes (jmp [rip+0]; .quad addr)
// Total: 22 bytes per trampoline
//
// The trampoline is called by guest code with SysV ABI.
// It sets R11 to the host MSVC function pointer, then jumps to the
// shared thunk which converts the calling convention and calls via R11.
class AbiWrapperManager {
public:
    ~AbiWrapperManager() {
#ifdef _WIN32
        for (void* page : pages_) {
            VirtualFree(page, 0, MEM_RELEASE);
        }
#endif
    }

    // Create a wrapper trampoline for a given MSVC host function pointer.
    // Returns the address of the trampoline (callable with SysV ABI).
    uint64_t CreateWrapper(uint64_t msvc_host_func) {
#ifdef _WIN32
        // Ensure we have space in the current page
        if (current_page_ == nullptr || page_offset_ + kTrampolineSize > kPageSize) {
            AllocatePage();
        }

        uint8_t* tramp = static_cast<uint8_t*>(current_page_) + page_offset_;

        // Generate: mov r11, imm64   (REX.W prefix + opcode + 8-byte immediate)
        // Encoding: 49 BB <8 bytes little-endian>
        tramp[0] = 0x49;
        tramp[1] = 0xBB;
        std::memcpy(&tramp[2], &msvc_host_func, 8);

        // Generate: jmp to sysv_to_msvc_thunk (absolute indirect via RIP-relative)
        // We use: FF 25 00 00 00 00 <8 bytes address>
        // This is: jmp QWORD PTR [rip+0], followed by the 8-byte address
        uint64_t thunk_addr = reinterpret_cast<uint64_t>(&sysv_to_msvc_thunk);
        tramp[10] = 0xFF;
        tramp[11] = 0x25;
        tramp[12] = 0x00;
        tramp[13] = 0x00;
        tramp[14] = 0x00;
        tramp[15] = 0x00;
        std::memcpy(&tramp[16], &thunk_addr, 8);

        page_offset_ += kTrampolineSize;
        return reinterpret_cast<uint64_t>(tramp);
#else
        // On non-Windows (Clang/GCC), PS4_SYSV_ABI works natively
        return msvc_host_func;
#endif
    }

    // Create a stub that just prints the name of the missing function.
    // This is used for unresolved imports. The guest passes SysV args,
    // we ignore them and call our MSVC handler with the name string.
    uint64_t CreateNamedStub(const char* name) {
#ifdef _WIN32
        if (current_page_ == nullptr || page_offset_ + 32 > kPageSize) {
            AllocatePage();
        }

        uint8_t* tramp = static_cast<uint8_t*>(current_page_) + page_offset_;
        
        static const auto target_func = [](const char* n) -> uint64_t {
            printf("[HLE Stub] Called unimplemented function stub: %s\n", n ? n : "UNKNOWN");
            return 0;
        };
        uint64_t target_addr = reinterpret_cast<uint64_t>(+target_func);
        // Duplicate the string so it survives ElfLoader destruction
        uint64_t name_addr = reinterpret_cast<uint64_t>(_strdup(name));


        // mov rcx, name_addr (48 B9 <8 bytes>)
        tramp[0] = 0x48; tramp[1] = 0xB9;
        std::memcpy(&tramp[2], &name_addr, 8);
        
        // mov rax, target_addr (48 B8 <8 bytes>)
        tramp[10] = 0x48; tramp[11] = 0xB8;
        std::memcpy(&tramp[12], &target_addr, 8);
        
        // sub rsp, 40 (48 83 EC 28)
        tramp[20] = 0x48; tramp[21] = 0x83; tramp[22] = 0xEC; tramp[23] = 0x28;
        
        // call rax (FF D0)
        tramp[24] = 0xFF; tramp[25] = 0xD0;
        
        // add rsp, 40 (48 83 C4 28)
        tramp[26] = 0x48; tramp[27] = 0x83; tramp[28] = 0xC4; tramp[29] = 0x28;
        
        // ret (C3)
        tramp[30] = 0xC3;

        // pad to 32
        tramp[31] = 0xCC;

        page_offset_ += 32;
        return reinterpret_cast<uint64_t>(tramp);
#else
        return reinterpret_cast<uint64_t>(&GenericHleStub); // Basic fallback for non-Windows
#endif
    }

private:
    static constexpr size_t kPageSize = 4096;
    static constexpr size_t kTrampolineSize = 24; // 10 (mov r11) + 14 (jmp indirect) padded

    void AllocatePage() {
#ifdef _WIN32
        void* page = VirtualAlloc(nullptr, kPageSize,
                                  MEM_COMMIT | MEM_RESERVE,
                                  PAGE_EXECUTE_READWRITE);
        if (page) {
            pages_.push_back(page);
            current_page_ = page;
            page_offset_ = 0;
        }
#endif
    }

    void* current_page_ = nullptr;
    size_t page_offset_ = 0;
    std::vector<void*> pages_;
};

} // namespace Core
