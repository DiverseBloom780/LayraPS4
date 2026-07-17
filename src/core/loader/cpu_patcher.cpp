// src/core/loader/cpu_patcher.cpp
// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "cpu_patcher.h"
#include <cstdio>
#include <cstring>
#include <vector>

namespace Core::Loader {

struct PatchPattern {
  const char *name;
  std::vector<uint8_t> signature;
  std::vector<uint8_t> replacement;
};

void ApplyLitePatches(uint8_t *base, size_t size) {
  if (!base || size == 0)
    return;

  printf("[CpuPatcher] Scanning %llu bytes for problematic instructions...\n",
         (unsigned long long)size);

  int patches_count = 0;

  // Patterns for FS segment redirection (Lite)
  // These are common patterns emitted by the PS4's Clang-based toolchain.
  
  // Pattern 1: mov rax, fs:[0] (9 bytes: 64 48 8B 04 25 00 00 00 00)
  // This is typical for getting the TCB (Thread Control Block) base.
  // We'll replace it with a nop-sled or a zero-out for now to avoid the FS violation on Windows.
  // Proper fix involves a trampoline to a host-safe TLS getter.
  const uint8_t pat_mov_rax_fs0[] = {0x64, 0x48, 0x8B, 0x04, 0x25, 0x00, 0x00, 0x00, 0x00};
  const uint8_t rep_mov_rax_0[]   = {0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90}; // mov rax, 0 + nops

  // Pattern 2: mov rax, fs:[0x28] (9 bytes: 64 48 8B 04 25 28 00 00 00)
  // Typical for stack canary check.
  const uint8_t pat_mov_rax_fs28[] = {0x64, 0x48, 0x8B, 0x04, 0x25, 0x28, 0x00, 0x00, 0x00};
  const uint8_t rep_mov_rax_can[]  = {0x48, 0xC7, 0xC0, 0xEF, 0xBE, 0xAD, 0xDE, 0x90, 0x90}; // mov rax, 0xDEADBEEF + nops

  for (size_t i = 0; i < size - 9; ++i) {
    if (memcmp(base + i, pat_mov_rax_fs0, 9) == 0) {
      memcpy(base + i, rep_mov_rax_0, 9);
      patches_count++;
    } else if (memcmp(base + i, pat_mov_rax_fs28, 9) == 0) {
      memcpy(base + i, rep_mov_rax_can, 9);
      patches_count++;
    }
    // TODO: Add more registers (rcx, rdx, etc.) if needed.
  }

  if (patches_count > 0) {
    printf("[CpuPatcher] Successfully applied %d lite patches.\n", patches_count);
  } else {
    printf("[CpuPatcher] No matching patterns found.\n");
  }
}

} // namespace Core::Loader
