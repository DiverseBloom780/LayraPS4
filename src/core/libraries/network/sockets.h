// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#ifdef WIN32
#define WINSOCKDEPRECATEDNOWARNINGS
#include <Ws2tcpip.h>
#include <afunix.h>
#include <iphlpapi.h>
#include <mstcpip.h>
#include <winsock2.h>
typedef SOCKET netsocket; 
typedef int socklent;
#ifndef LPFNWSASENDMSG
typedef INT(PASCAL LPFNWSASENDMSG)(SOCKET s, LPWSAMSG lpMsg, DWORD dwFlags,
 LPDWORD lpNumberOfBytesSent, LPWSAOVERLAPPED lpOverlapped,
 LPWSAOVERLAPPEDCOMPLETIONROUTINE lpCompletionRoutine); 
#endif
#ifndef WSAIDWSASENDMSG
static const GUID WSAIDWSASENDMSG = {
 0xa441e712, 0x754f, 0x43ca, {0x84, 0xa7, 0x0d, 0xee, 0x44, 0xcf, 0x60, 0x6d}}; 
#endif
#ifndef LPFNWSARECVMSG
typedef INT(PASCAL LPFNWSARECVMSG)(SOCKET s, LPWSAMSG lpMsg, LPDWORD lpdwNumberOfBytesRecvd, 
 LPWSAOVERLAPPED lpOverlapped,
 LPWSAOVERLAPPEDCOMPLETIONROUTINE lpCompletionRoutine); 
#endif

#ifndef WSAIDWSARECVMSG
static const GUID WSAIDWSARECVMSG = {
 0xf689d7c8, 0x6f1f, 0x436b, {0x8a, 0x53, 0xe5, 0x4f, 0xe3, 0x51, 0xc3, 0x22}};
#endif
#else
#include <cerrno>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h> 
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int netsocket; 
#endif
#include <map>
#include <memory>
#include <mutex>
#include "net.h"

namespace Libraries::Kernel {
struct OrbisKernelStat;
}

namespace Libraries::Net {

// Socket class
struct Socket {
    // Constructor
    explicit Socket(int domain, int type, int protocol) {}

    // Destructor
    virtual ~Socket() = default;

    // Check if the socket is valid
    virtual bool IsValid() const = 0;

    // Close the socket
    virtual int Close() = 0;

    // Set socket options
    virtual int SetSocketOptions(int level, int optname, const void optval, u32 optlen) = 0;

    // Get socket options
    virtual int GetSocketOptions(int level, int optname, void optval, u32 optlen) = 0;

    // Bind the socket to a address
    virtual int Bind(const OrbisNetSockaddr addr, u32 addrlen) = 0;

    // Listen for incoming connections
    virtual int Listen(int backlog) = 0;

    // Send a message over the socket
    virtual int SendMessage(const OrbisNetMsghdr msg, int flags) = 0;

    // Send a packet over the socket
    virtual int SendPacket(const void msg, u32 len, int flags, const OrbisNetSockaddr to,
                           u32 tolen) = 0;

    // Accept an incoming connection
    virtual SocketPtr Accept(OrbisNetSockaddr addr, u32 addrlen) = 0;

    // Receive a message from the socket
    virtual int ReceiveMessage(OrbisNetMsghdr msg, int flags) = 0;

    // Receive a packet from the socket
    virtual int ReceivePacket(void buf, u32 len, int flags, OrbisNetSockaddr from,
                              u32 fromlen) = 0;

    // Connect to a remote address
    virtual int Connect(const OrbisNetSockaddr addr, u32 namelen) = 0;

    // Get the socket address
    virtual int GetSocketAddress(OrbisNetSockaddr name, u32 namelen) = 0;

    // Get the peer name
    virtual int GetPeerName(OrbisNetSockaddr addr, u32 namelen) = 0;

    // Get file status
    virtual int fstat(Libraries::Kernel::OrbisKernelStat* stat) = 0;

    // Get the native socket handle
    virtual std::optional<net_socket> Native() = 0
