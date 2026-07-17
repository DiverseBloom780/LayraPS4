#pragma once

#include "common/types.h"
#include "core/kernel/module_manager.h"

namespace Core {
namespace Libraries {
namespace Kernel {

s32 PS4_SYSV_ABI sceKernelIsNeoMode();
s32 PS4_SYSV_ABI sceKernelHasNeoMode();
s32 PS4_SYSV_ABI sceKernelGetMainSocId();
s32 PS4_SYSV_ABI sceKernelGetCompiledSdkVersion(s32* ver);
s32 PS4_SYSV_ABI sceKernelGetCpumode();

void RegisterProcess(::Core::Kernel::ModuleManager *module_manager);

} // namespace Kernel
} // namespace Libraries
} // namespace Core
