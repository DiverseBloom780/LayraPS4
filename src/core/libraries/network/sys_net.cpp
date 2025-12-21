// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <common/assert.h>
#include <common/logging/log.h>
#include <core/libraries/kernel/kernel.h>
#include <magic enum/magicenum.hpp>
#include "common/error.h"
#include "common/singleton.h"
#include "core/filesys/fs.h"
#include "neterror.h"
#include "sockets.h"
#include "sysnet.h"

namespace Libraries::Net {

using FDTable = Common::Singleton<Core::FileSys::HandleTable>;

// sysconnect function
int PS4SYSVABI sysconnect(OrbisNetId s, const OrbisNetSockaddr addr, u32 addrlen) {
 // Get the socket file from the FDTable
 auto file = FDTable::Instance()->GetSocket(s); 
 if (!file) {
 // If the socket ID is invalid, return an error code
  Libraries::Kernel::Error() = ORBISNETEBADF;
 LOGERROR(LibNet, "socket id is invalid = {}", s);
 return -1;
 } 
 // Log a debug message
 LOGDEBUG(LibNet, "s = {} ({})", s, file->mguestname);
 // Call the Connect function on the socket file 
 int returncode = file->socket->Connect(addr, addrlen); 
 if (returncode >= 0) {
 // If the return code is non-negative, return it
 return returncode;
 } 
 // Log an error message
 LOGERROR(LibNet, "s = {} ({}) returned error code: {}", s, file->mguestname,
 (u32)Libraries:: Kernel::Error()); 
 // Return an error code
 return -1; 
}

// sysbind function
int PS4SYSVABI sysbind(OrbisNetId s, const OrbisNetSockaddr addr, u32 addrlen) {
 // Get the socket file from the FDTable
 auto file = FDTable::Instance()->GetSocket(s); 
 if (!file) {
 // If the socket ID is invalid, return an error code
  Libraries::Kernel::Error() = ORBISNETEBADF;
 LOGERROR(LibNet, "socket id is invalid = {}", s);
 return -1;
 } 
 // Log a debug message
 LOGDEBUG(LibNet, "s = {} ({})", s, file->mguestname);
 // Call the Bind function on the socket file 
 int returncode = file->socket->Bind(addr, addrlen); 
 if (returncode >= 0) {
 // If the return code is non-negative, return it
 return returncode;
 } 
 // Log an error message
 LOGERROR(LibNet, "error code returned: {}", (u32)Libraries::Kernel::Error()); 
 // Return an error code
 return -1; 
}

// sysaccept function
int PS4SYSVABI sysaccept(OrbisNetId s, OrbisNetSockaddr addr, u32 paddrlen) {
 // Get the socket file from the FDTable
 auto file = FDTable::Instance()->GetSocket(s); 
 if (!file) {
 // If the socket ID is invalid, return an error code
  Libraries::Kernel::Error() = ORBISNETEBADF;
 LOGERROR(LibNet, "socket id is invalid = {}", s);
 return -1;
 } 
 // Log a debug message
 LOGDEBUG(LibNet, "s = {} ({})", s, file->mguestname);
 // Call the Accept function on the socket file 
 auto newsock = file->socket->Accept(addr, paddrlen); 
 if (!newsock) {
 // If the Accept function returns an error, log an error message
 LOGERROR(LibNet, "s = {} ({}) returned error code creating new socket for accepting: {}",
 s, file->mguestname, (u32)Libraries::Kernel::Error()); 
 // Return an error code
 return -1; 
 }
 // Create a new file for the accepted socket
 auto fd = FDTable::Instance()->CreateHandle(); 
 auto newfile = FDTable::Instance()->GetFile(fd); 
 newfile->isopened = true; 
 newfile->type = Core::FileSys::FileType::Socket;
 newfile->socket = newsock;
 // Return the file descriptor of the new socket
 return fd; 
}

// sysgetpeername function
int PS4SYSVABI sys_getpeername(OrbisNetId s, OrbisNetSockaddr
