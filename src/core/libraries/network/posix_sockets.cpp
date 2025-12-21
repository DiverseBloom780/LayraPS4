// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <vector>
#include <common/assert.h>
#include "common/error.h"
#include "core/libraries/kernel/filesystem.h"
#include "core/libraries/kernel/kernel.h"
#include "net.h"
#ifndef WIN32
#include <sys/stat.h>
#endif
#include "neterror.h"
#include "sockets.h"
#include "core/networking/networking.h"
#include "common/singleton.h"

namespace Libraries::Net {

#ifdef WIN32
#define ERRORCASE(errname) \
 case (WSA##errname): \
  Libraries::Kernel::Error() = ORBISNET##errname; \ 
 return -1; 
#else
#define ERRORCASE(errname) \
 case (errname): \
  Libraries::Kernel::_Error() = ORBISNET##errname; \ 
 return -1; 
#endif

 Function to convert return error code
static int ConvertReturnErrorCode(int retval) {
 if (retval < 0) {
#ifdef WIN32
 switch ( WSAGetLastError()) {
#else
 switch (errno) {
#endif
#ifndef WIN32 // These errorcodes don't exist in WinSock
 ERRORCASE(EPERM)
 ERRORCASE(ENOENT)
 // ERRORCASE(ESRCH)
 // ERRORCASE(EIO)
 // ERRORCASE(ENXIO)
 // ERRORCASE(E2BIG)
 // ERRORCASE(ENOEXEC)
 // ERRORCASE(EDEADLK)
 ERRORCASE(ENOMEM)
 // ERRORCASE( ECHILD)
 // ERRORCASE(EBUSY)
 ERRORCASE(EEXIST)
 // ERRORCASE(EXDEV)
 ERRORCASE(ENODEV)
 // ERRORCASE(ENOTDIR)
 // ERRORCASE(EISDIR)
 ERRORCASE(ENFILE)
 // ERRORCASE(ENOTTY)
 // ERRORCASE( ETXTBSY)
 // ERRORCASE(EFBIG)
 ERRORCASE(ENOSPC)
 // ERRORCASE(ESPIPE)
 // ERRORCASE(EROFS)
 // ERRORCASE(EMLINK)
 ERRORCASE(EPIPE)
 // ERRORCASE(EDOM)
 // ERRORCASE(ERANGE)
 // ERRORCASE(ENOLCK)
 // ERRORCASE(ENOSYS)
 // ERRORCASE(EIDRM)
 // ERRORCASE(EOVERFLOW)
 // ERRORCASE(EILSEQ) 
 // ERRORCASE(ENOTSUP)
 ERRORCASE(ECANCELED)
 // ERRORCASE(EBADMSG)
 ERRORCASE(ENODATA)
 // ERRORCASE(ENOSR)
 // ERRORCASE(ENOSTR)
 // ERRORCASE(ETIME)
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
#if defined(APPLE) || defined(WIN32)
 ERRORCASE(EOPNOTSUPP)
#endif
 ERRORCASE(EAFNOSUPPORT)
 ERRORCASE(EADDRINUSE)
 ERRORCASE(EADDRNOTAVAIL)
 ERRORCASE(ENETDOWN)
 ERROR CASE(ENETUNREACH)
 ERRORCASE(ENETRESET)
 ERRORCASE(ECONNABORTED)
 ERRORCASE(ECONNRESET)
 ERROR CASE(ENOBUFS)
 ERRORCASE(EISCONN)
 ERRORCASE(ENOTCONN)
 ERRORCASE(ETIMEDOUT)
 ERRORCASE( ECONNREFUSED)
 ERRORCASE(ELOOP)
 ERRORCASE(ENAMETOOLONG)
 ERRORCASE(EHOSTUNREACH)
 ERRORCASE( ENOTEMPTY)
 }
 *Libraries::Kernel::_Error() = ORBISNETEINTERNAL;
 return -1;
 } 
 // if it is 0 or positive return it as it is
 return retval; 
}

// Function to convert levels
static int ConvertLevels(int level) {
 switch (level) {
 case ORBISNETSOLSOCKET:
 return SOLSOCKET;
 case ORBISNETIPPROTOIP:
 return IPPROTOIP;
 case ORBISNETIPPROTOTCP:
 return IPPROTOTCP;
 case ORBISNETIPPROTOIPV6:
 return IPPROTO_IPV6; 
 }
 return -1;
}

// Function to convert OrbisNetSockaddr to Posix
