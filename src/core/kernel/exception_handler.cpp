// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "exception_handler.h"
#include "syscall_handler.h"
#include <cstdio>
#include <cstdint>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace Core::Kernel {

#if defined(_WIN32)
static SyscallHandler *g_syscall_handler = nullptr;
static PVOID g_exception_handler_handle = nullptr;

static LONG WINAPI VehExceptionHandler(PEXCEPTION_POINTERS exception_pointers) noexcept {
  if (!g_syscall_handler || !exception_pointers || !exception_pointers->ContextRecord) {
    return EXCEPTION_CONTINUE_SEARCH;
  }

  const auto *record = exception_pointers->ExceptionRecord;
  auto *context = exception_pointers->ContextRecord;

  if (record->ExceptionCode != EXCEPTION_ILLEGAL_INSTRUCTION) {
    return EXCEPTION_CONTINUE_SEARCH;
  }

  const auto *rip = reinterpret_cast<const uint8_t *>(context->Rip);
  if (!rip) {
    return EXCEPTION_CONTINUE_SEARCH;
  }

  // Guest Linux-like syscall intercept: syscall or int 0x80
  if (rip[0] == 0x0F && rip[1] == 0x05) {
    uint64_t args[6] = {context->Rdi, context->Rsi, context->Rdx,
                        context->R10, context->R8, context->R9};
    int syscall_id = static_cast<int>(context->Rax);
    uint64_t result = g_syscall_handler->Dispatch(syscall_id, context, args);
    context->Rax = result;
    context->Rip += 2;
    return EXCEPTION_CONTINUE_EXECUTION;
  }

  if (rip[0] == 0xCD && rip[1] == 0x80) {
    uint64_t args[6] = {context->Rbx, context->Rcx, context->Rdx,
                        context->Rsi, context->Rdi, context->Rbp};
    int syscall_id = static_cast<int>(context->Rax);
    uint64_t result = g_syscall_handler->Dispatch(syscall_id, context, args);
    context->Rax = result;
    context->Rip += 2;
    return EXCEPTION_CONTINUE_EXECUTION;
  }

  return EXCEPTION_CONTINUE_SEARCH;
}
#endif

void InstallExceptionHandler(SyscallHandler *handler) {
#if defined(_WIN32)
  g_syscall_handler = handler;
  if (!g_exception_handler_handle) {
    g_exception_handler_handle = AddVectoredExceptionHandler(0, VehExceptionHandler);
    if (!g_exception_handler_handle) {
      fprintf(stderr, "[ExceptionHandler] Failed to install VEH handler\n");
    }
  }
#else
  (void)handler;
#endif
}

void RemoveExceptionHandler() {
#if defined(_WIN32)
  if (g_exception_handler_handle) {
    RemoveVectoredExceptionHandler(g_exception_handler_handle);
    g_exception_handler_handle = nullptr;
  }
  g_syscall_handler = nullptr;
#endif
}

} // namespace Core::Kernel
