// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#ifdef WIN32
#define WINSOCKDEPRECATEDNOWARNINGS
#include <Ws2tcpip.h>
#include <iphlpapi.h>
#include <winsock2.h>
typedef SOCKET netsocket; 
typedef int socklent;
#else
#include <cerrno>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/ socket.h>
#include <unistd.h>
typedef int netsocket; 
#endif
#if defined(APPLE)
#include <net/ifdl.h>
#include <net/route.h>
#endif
#if linux #include <fstream>
#include <iostream>
#include <sstream>
#endif

#include <map>
#include <memory>
#include <mutex>
#include <vector>
#include <string.h>
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/errorcodes.h"
#include "net.h"
#include "neterror.h"
#include "netutil.h"
#include "core/networking/networking.h"
#include "common/config.h"
#include " core/networking/networking.h"

namespace NetUtil {

const std::array<u8, 6>& NetUtilInternal::GetEthernetAddr() const {
    return etheraddress;
}

bool NetUtilInternal::RetrieveEthernetAddr() {
    std::scopedlock lock{mmutex};
#ifdef WIN32
    std::vector<u8> adapterinfos(sizeof(IPADAPTERINFO));
    ULONG sizeinfos = sizeof(IPADAPTERINFO);

 if (GetAdaptersInfo(reinterpretcast<PIPADAPTERINFO>(adapterinfos.data()), &sizeinfos) ==
 ERRORBUFFEROVERFLOW) 
 adapterinfos.resize(sizeinfos);

 if (GetAdaptersInfo(reinterpretcast<PIPADAPTERINFO>(adapterinfos.data()), &sizeinfos) ==
 NOERROR &&
 sizeinfos) {
 PIPADAPTERINFO info = reinterpretcast<PIPADAPTERINFO>(adapterinfos.data());
 memcpy( etheraddress.data(), info[0]. Address, 6); 
 return true; 
 }
#elif defined(APPLE)
 ifaddrs ifap;

    if (getifaddrs(&ifap) == 0) {
        ifaddrs p;
        for (p = ifap; p; p = p->ifanext) {
            if (p->ifaaddr->safamily == AFLINK) {
                sockaddrdl sdp = reinterpretcast<sockaddrdl>(p->ifaaddr);
                memcpy(etheraddress.data(), sdp->sdldata + sdp->sdlnlen, 6);
                freeifaddrs(ifap);
                return true;
            }
        }
        freeifaddrs(ifap);
    }
#else
    ifreq ifr;
    ifconf ifc;
    char buf[1024];
    int success = 0;

    int sock = socket(AFINET, SOCKDGRAM, IPPROTOIP);
    if (sock == -1)
        return false;

    ifc.ifclen = sizeof(buf);
    ifc.ifcbuf = buf;
    if (ioctl(sock, SIOCGIFCONF, &ifc) == -1)
        return false;

    ifreq it = ifc.ifcreq;
    const ifreq const end = it + (ifc.ifclen / sizeof(ifreq));

    for (; it != end; ++it) {
        strcpy(ifr.ifrname, it->ifrname);
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
            if (!(ifr.ifrflags & IFFLOOPBACK)) {
                if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
                    success = 1;
                    break;
                }
            }
        }
    }

    close(sock);

    if (success) {
        memcpy(etheraddress.data(), ifr.ifrhwaddr.sadata, 6);
        return true;
    }
#endif
    return false;
}

const std::string& NetUtilInternal::GetDefaultGateway() const {
    if (Common::Singleton<Core::Networking::NetworkingCore>::Instance()->getNetworkingConfig().mode ==
        Core::Networking::Mode::LANPlay) {
        return "10.13.37.1";
    }
    return defaultgateway;
}

bool NetUtilInternal::RetrieveDefaultGateway() {
    std::scopedlock lock{mmutex};

#ifdef WIN32
    ULONG flags = GAAFLAGINCLUDEGATEWAYS;
    ULONG family = AFINET; // Only IPv4
    ULONG buffersize = 15000;

    std::vector<BYTE> buffer(buffersize);
    PIPADAPTERADDRESSES adapteraddresses =
        reinterpret_cast
