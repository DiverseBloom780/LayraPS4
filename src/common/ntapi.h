#pragma once
#include <cstdint>

// Stub definitions for NTAPI
typedef struct _IO_STATUS_BLOCK {
  union {
    long Status;
    void *Pointer;
  };
  unsigned long long Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef void (*PIO_APC_ROUTINE)(void *ApcContext,
                                PIO_STATUS_BLOCK IoStatusBlock,
                                unsigned long Reserved);

// Add other necessary valid definitions or stubs here
