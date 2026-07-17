// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "process.h"
#include <cstdio>

namespace Core {
namespace Libraries {
namespace Kernel {

s32 PS4_SYSV_ABI sceKernelIsNeoMode() {
    printf("[Kernel] sceKernelIsNeoMode called\n");
    return 0; // Not neo
}

s32 PS4_SYSV_ABI sceKernelHasNeoMode() {
    printf("[Kernel] sceKernelHasNeoMode called\n");
    return 0; // Not neo
}

s32 PS4_SYSV_ABI sceKernelGetMainSocId() {
    printf("[Kernel] sceKernelGetMainSocId called\n");
    return 0x710f10; // Standard PS4 id
}

s32 PS4_SYSV_ABI sceKernelGetCompiledSdkVersion(s32* ver) {
    if (!ver) return 0x8002000E; // ORBIS_KERNEL_ERROR_EFAULT
    
    // Return a dummy SDK version for now to let games boot
    *ver = 0x05000000;
    
    printf("[Kernel] sceKernelGetCompiledSdkVersion called, SDK = %08X\n", *ver);
    return 0; // ORBIS_OK
}

s32 PS4_SYSV_ABI sceKernelGetCpumode() {
    printf("[Kernel] sceKernelGetCpumode called\n");
    return 0; // Standard CPU mode
}

void RegisterProcess(::Core::Kernel::ModuleManager *module_manager) {
    printf("[Kernel] Registering Process Syscalls\n");
    
    module_manager->RegisterHLEExport("libkernel", "WslcK1FQcGI", "sceKernelIsNeoMode",
                                     (uint64_t)&sceKernelIsNeoMode);
    module_manager->RegisterHLEExport("libkernel", "rNRtm1uioyY", "sceKernelHasNeoMode",
                                     (uint64_t)&sceKernelHasNeoMode);
    module_manager->RegisterHLEExport("libkernel", "0vTn5IDMU9A", "sceKernelGetMainSocId",
                                     (uint64_t)&sceKernelGetMainSocId);
    module_manager->RegisterHLEExport("libkernel", "WB66evu8bsU", "sceKernelGetCompiledSdkVersion",
                                     (uint64_t)&sceKernelGetCompiledSdkVersion);
    module_manager->RegisterHLEExport("libkernel", "VOx8NGmHXTs", "sceKernelGetCpumode",
                                     (uint64_t)&sceKernelGetCpumode);
}

} // namespace Kernel
} // namespace Libraries
} // namespace Core
