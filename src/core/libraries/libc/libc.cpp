// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "libc.h"
#include <cmath>
#include <cstdio>
#include <cstring>

namespace Core {
namespace Libraries {
namespace Libc {

// Memory
void *internal_memset(void *s, int c, size_t n) { return std::memset(s, c, n); }
void *internal_memcpy(void *dest, const void *src, size_t n) {
  return std::memcpy(dest, src, n);
}
int32_t internal_memcpy_s(void *dest, size_t destsz, const void *src,
                          size_t count) {
#ifdef _WIN64
  return memcpy_s(dest, destsz, src, count);
#else
  std::memcpy(dest, src, count);
  return 0;
#endif
}
int32_t internal_memcmp(const void *s1, const void *s2, size_t n) {
  return std::memcmp(s1, s2, n);
}

// String
int32_t internal_strcpy_s(char *dest, size_t dest_size, const char *src) {
#ifdef _WIN64
  return strcpy_s(dest, dest_size, src);
#else
  std::strcpy(dest, src);
  return 0;
#endif
}
int32_t internal_strcat_s(char *dest, size_t dest_size, const char *src) {
#ifdef _WIN64
  return strcat_s(dest, dest_size, src);
#else
  std::strcat(dest, src);
  return 0;
#endif
}
int32_t internal_strcmp(const char *str1, const char *str2) {
  return std::strcmp(str1, str2);
}
int32_t internal_strncmp(const char *str1, const char *str2, size_t num) {
  return std::strncmp(str1, str2, num);
}
size_t internal_strlen(const char *str) { return std::strlen(str); }
char *internal_strncpy(char *dest, const char *src, size_t count) {
  return std::strncpy(dest, src, count);
}
int32_t internal_strncpy_s(char *dest, size_t destsz, const char *src,
                           size_t count) {
#ifdef _WIN64
  return strncpy_s(dest, destsz, src, count);
#else
  std::strncpy(dest, src, count);
  return 0;
#endif
}
char *internal_strcat(char *dest, const char *src) {
  return std::strcat(dest, src);
}
const char *internal_strchr(const char *str, int c) {
  return std::strchr(str, c);
}

// Math
double internal_sin(double x) { return std::sin(x); }
float internal_sinf(float x) { return std::sinf(x); }
double internal_cos(double x) { return std::cos(x); }
float internal_cosf(float x) { return std::cosf(x); }
void internal_sincos(double x, double *sinp, double *cosp) {
  *sinp = std::sin(x);
  *cosp = std::cos(x);
}
void internal_sincosf(float x, float *sinp, float *cosp) {
  *sinp = std::sinf(x);
  *cosp = std::cosf(x);
}
double internal_tan(double x) { return std::tan(x); }
float internal_tanf(float x) { return std::tanf(x); }
double internal_asin(double x) { return std::asin(x); }
float internal_asinf(float x) { return std::asinf(x); }
double internal_acos(double x) { return std::acos(x); }
float internal_acosf(float x) { return std::acosf(x); }
double internal_atan(double x) { return std::atan(x); }
float internal_atanf(float x) { return std::atanf(x); }
double internal_atan2(double y, double x) { return std::atan2(y, x); }
float internal_atan2f(float y, float x) { return std::atan2f(y, x); }
double internal_exp(double x) { return std::exp(x); }
float internal_expf(float x) { return std::expf(x); }
double internal_exp2(double x) { return std::exp2(x); }
float internal_exp2f(float x) { return std::exp2f(x); }
double internal_pow(double x, double y) { return std::pow(x, y); }
float internal_powf(float x, float y) { return std::powf(x, y); }
double internal_log(double x) { return std::log(x); }
float internal_logf(float x) { return std::logf(x); }
double internal_log10(double x) { return std::log10(x); }
float internal_log10f(float x) { return std::log10f(x); }

uint32_t internal_setjmp(void* env) {
    printf("[libc] setjmp called\n");
    return 0;
}

// C++ ABI guard functions
int __cxa_guard_acquire(uint64_t* guard_object) {
    uint8_t* guard = reinterpret_cast<uint8_t*>(guard_object);
    if (*guard == 0) {
        return 1; // Needs initialization
    }
    return 0; // Already initialized
}

void __cxa_guard_release(uint64_t* guard_object) {
    uint8_t* guard = reinterpret_cast<uint8_t*>(guard_object);
    *guard = 1; // Mark as initialized
}

#define LIB_FUNCTION(nid, library, version, module, function)                  \
  module_manager->RegisterHLEExport(module, nid, #function,                    \
                                    reinterpret_cast<uint64_t>(function));

void RegisterLibc(::Core::Kernel::ModuleManager *module_manager) {
  printf("[libc] Registering HLE functions...\n");

  // Memory
  LIB_FUNCTION("8zTFvBIAIN8", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_memset);
  LIB_FUNCTION("Q3VBxCXhUHs", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_memcpy);
  LIB_FUNCTION("NFLs+dRJGNg", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_memcpy_s);
  LIB_FUNCTION("DfivPArhucg", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_memcmp);

  // String
  LIB_FUNCTION("5Xa2ACNECdo", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_strcpy_s);
  LIB_FUNCTION("K+gcnFFJKVc", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_strcat_s);
  LIB_FUNCTION("aesyjrHVWy4", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_strcmp);
  LIB_FUNCTION("Ovb2dSJOAuE", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_strncmp);
  LIB_FUNCTION("j4ViWNHEgww", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_strlen);
  LIB_FUNCTION("6sJWiWSRuqk", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_strncpy);
  LIB_FUNCTION("YNzNkJzYqEg", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_strncpy_s);
  LIB_FUNCTION("Ls4tzzhimqQ", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_strcat);
  LIB_FUNCTION("ob5xAW4ln-0", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_strchr);

  // Math
  LIB_FUNCTION("H8ya2H00jbI", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_sin);
  LIB_FUNCTION("Q4rRL34CEeE", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_sinf);
  LIB_FUNCTION("2WE3BTYVwKM", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_cos);
  LIB_FUNCTION("-P6FNMzk2Kc", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_cosf);
  LIB_FUNCTION("jMB7EFyu30Y", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_sincos);
  LIB_FUNCTION("pztV4AF18iI", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_sincosf);
  LIB_FUNCTION("T7uyNqP7vQA", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_tan);
  LIB_FUNCTION("ZE6RNL+eLbk", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_tanf);
  LIB_FUNCTION("7Ly52zaL44Q", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_asin);
  LIB_FUNCTION("GZWjF-YIFFk", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_asinf);
  LIB_FUNCTION("JBcgYuW8lPU", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_acos);
  LIB_FUNCTION("QI-x0SL8jhw", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_acosf);
  LIB_FUNCTION("OXmauLdQ8kY", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_atan);
  LIB_FUNCTION("weDug8QD-lE", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_atanf);
  LIB_FUNCTION("HUbZmOnT-Dg", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_atan2);
  LIB_FUNCTION("EH-x713A99c", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_atan2f);
  LIB_FUNCTION("NVadfnzQhHQ", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_exp);
  LIB_FUNCTION("8zsu04XNsZ4", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_expf);
  LIB_FUNCTION("dnaeGXbjP6E", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_exp2);
  LIB_FUNCTION("wuAQt-j+p4o", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_exp2f);
  LIB_FUNCTION("9LCjpWyQ5Zc", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_pow);
  LIB_FUNCTION("1D0H2KNjshE", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_powf);
  LIB_FUNCTION("rtV7-jWC6Yg", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_log);
  LIB_FUNCTION("RQXLbdT2lc4", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_logf);
  LIB_FUNCTION("WuMbPBKN1TU", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_log10);
  LIB_FUNCTION("lhpd6Wk6ccs", "libSceLibcInternal", 1, "libSceLibcInternal",
               internal_log10f);

  // C++ ABI guard functions
  LIB_FUNCTION("3GPpjQdAMTw", "libSceLibcInternal", 1, "libSceLibcInternal",
               __cxa_guard_acquire);
  LIB_FUNCTION("9rAeANT2tyE", "libSceLibcInternal", 1, "libSceLibcInternal",
               __cxa_guard_release);

  printf("[libc] Registration complete.\n");
}

} // namespace Libc
} // namespace Libraries
} // namespace Core
