#pragma once

// Stub for netctl.h
#define SCE_NET_CTL_STATE_DISCONNECTED 0
#define SCE_NET_CTL_STATE_CONNECTED 1
#define SCE_NET_CTL_STATE_CONNECTING 2

struct SceNetCtlInfo {
  int state;
};

inline int sceNetCtlInit() { return 0; }
inline void sceNetCtlTerm() {}
inline int sceNetCtlGetInfo(int code, SceNetCtlInfo *info) {
  if (info)
    info->state = SCE_NET_CTL_STATE_CONNECTED;
  return 0;
}
