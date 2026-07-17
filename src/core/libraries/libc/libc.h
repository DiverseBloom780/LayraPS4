// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/kernel/module_manager.h"
#include <cstddef>
#include <cstdint>

namespace Core {
namespace Libraries {
namespace Libc {

void RegisterLibc(::Core::Kernel::ModuleManager *module_manager);

// Memory
void *internal_memset(void *s, int c, size_t n);
void *internal_memcpy(void *dest, const void *src, size_t n);
int32_t internal_memcpy_s(void *dest, size_t destsz, const void *src,
                          size_t count);
int32_t internal_memcmp(const void *s1, const void *s2, size_t n);

// String
int32_t internal_strcpy_s(char *dest, size_t dest_size, const char *src);
int32_t internal_strcat_s(char *dest, size_t dest_size, const char *src);
int32_t internal_strcmp(const char *str1, const char *str2);
int32_t internal_strncmp(const char *str1, const char *str2, size_t num);
size_t internal_strlen(const char *str);
char *internal_strncpy(char *dest, const char *src, size_t count);
int32_t internal_strncpy_s(char *dest, size_t destsz, const char *src,
                           size_t count);
char *internal_strcat(char *dest, const char *src);
const char *internal_strchr(const char *str, int c);

// Math
double internal_sin(double x);
float internal_sinf(float x);
double internal_cos(double x);
float internal_cosf(float x);
void internal_sincos(double x, double *sinp, double *cosp);
void internal_sincosf(float x, float *sinp, float *cosp);
double internal_tan(double x);
float internal_tanf(float x);
double internal_asin(double x);
float internal_asinf(float x);
double internal_acos(double x);
float internal_acosf(float x);
double internal_atan(double x);
float internal_atanf(float x);
double internal_atan2(double y, double x);
float internal_atan2f(float y, float x);
double internal_exp(double x);
float internal_expf(float x);
double internal_exp2(double x);
float internal_exp2f(float x);
double internal_pow(double x, double y);
float internal_powf(float x, float y);
double internal_log(double x);
float internal_logf(float x);
double internal_log10(double x);
float internal_log10f(float x);

// I/O
// snprintf is tricky with varargs in HLE, usually handled by a wrapper that
// pulls from registers For now we will stub it or use a simple version if
// possible. internal_snprintf(char* s, size_t n, ...)

} // namespace Libc
} // namespace Libraries
} // namespace Core
