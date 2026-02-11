// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#include "common/log.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/network/neterror.h"
#include "core/libraries/network/sockets.h"
#include <cstring>
#include <vector>

namespace Libraries::Net {

#ifdef _WIN32
#define ERRORCASE(errname)                                                     \
  case (WSA##errname):                                                         \
    Libraries::Kernel::Error() = ORBIS_NET_##errname;                          \
    return -1;
#else
#define ERRORCASE(errname)                                                     \
  case (errname):                                                              \
    Libraries::Kernel::Error() = ORBIS_NET_##errname;                          \
    return -1;
#endif

// ----  Convert native errno → Orbis error code  ----
static int ConvertReturnErrorCode(int retval) {
  if (retval < 0) {
#ifdef _WIN32
    switch (WSAGetLastError()) {
#else
    switch (errno) {
#endif
      ERRORCASE(EINTR)
      ERRORCASE(EBADF)
      ERRORCASE(EACCES)
      ERRORCASE(EFAULT)
      ERRORCASE(EINVAL)
      ERRORCASE(EMFILE)
      ERRORCASE(EWOULDBLOCK)
      ERRORCASE(EINPROGRESS)
      ERRORCASE(EALREADY)
      ERRORCASE(ENOTSOCK)
      ERRORCASE(EDESTADDRREQ)
      ERRORCASE(EMSGSIZE)
      ERRORCASE(EPROTOTYPE)
      ERRORCASE(ENOPROTOOPT)
      ERRORCASE(EPROTONOSUPPORT)
#if defined(__APPLE__) || defined(_WIN32)
      ERRORCASE(EOPNOTSUPP)
#endif
      ERRORCASE(EAFNOSUPPORT)
      ERRORCASE(EADDRINUSE)
      ERRORCASE(EADDRNOTAVAIL)
      ERRORCASE(ENETDOWN)
      ERRORCASE(ENETUNREACH)
      ERRORCASE(ENETRESET)
      ERRORCASE(ECONNABORTED)
      ERRORCASE(ECONNRESET)
      ERRORCASE(ENOBUFS)
      ERRORCASE(EISCONN)
      ERRORCASE(ENOTCONN)
      ERRORCASE(ETIMEDOUT)
      ERRORCASE(ECONNREFUSED)
      ERRORCASE(ELOOP)
      ERRORCASE(ENAMETOOLONG)
      ERRORCASE(EHOSTUNREACH)
      ERRORCASE(ENOTEMPTY)
    }
    // Unknown error → internal
    Libraries::Kernel::Error() = ORBIS_NET_EINTERNAL;
    return -1;
  }
  // Success or positive value → return as-is
  return retval;
}

// ----  Convert Orbis option level → native level  ----
static int ConvertLevels(int level) {
  switch (level) {
  case ORBIS_NET_SOL_SOCKET:
    return SOL_SOCKET;
  case ORBIS_NET_IPPROTO_IP:
    return IPPROTO_IP;
  case ORBIS_NET_IPPROTO_TCP:
    return IPPROTO_TCP;
  case ORBIS_NET_IPPROTO_IPV6:
    return IPPROTO_IPV6;
  }
  return -1;
}

// ----  Convert Orbis sockaddr → native sockaddr  ----
static int ConvertSockAddr(const OrbisNetSockaddr *in, struct sockaddr *out,
                           u32 *len) {
  // Stub: just copy and hope sizes match
  if (!in

      !out !len)
    return -1;
  std::memcpy(out, in, *len);
  return 0;
}
// ----  POSIX socket stubs  ----
int PS4_SYSV_ABI sys_socket(int domain, int type, int protocol) {
  LOG_ERROR(LibNet, "(STUBBED) sys_socket domain={} type={} proto={}", domain,
            type, protocol);
  return 42; // fake fd
}

int PS4_SYSV_ABI sys_bind(int sockfd, const OrbisNetSockaddr *addr,
                          u32 addrlen) {
  LOG_ERROR(LibNet, "(STUBBED) sys_bind fd={}", sockfd);
  return ConvertReturnErrorCode(0);
}

int PS4_SYSV_ABI sys_connect(int sockfd, const OrbisNetSockaddr *addr,
                             u32 addrlen) {
  LOG_ERROR(LibNet, "(STUBBED) sys_connect fd={}", sockfd);
  return ConvertReturnErrorCode(0);
}

int PS4_SYSV_ABI sys_accept(int sockfd, OrbisNetSockaddr *addr, u32 *addrlen) {
  LOG_ERROR(LibNet, "(STUBBED) sys_accept fd={}", sockfd);
  return ConvertReturnErrorCode(-ORBIS_NET_EAGAIN); // retry
}

int PS4_SYSV_ABI sys_setsockopt(int sockfd, int level, int optname,
                                const void *optval, u32 optlen) {
  LOG_ERROR(LibNet, "(STUBBED) sys_setsockopt fd={} level={} opt={}", sockfd,
            level, optname);
  return ConvertReturnErrorCode(0);
}

} // namespace Libraries::Net