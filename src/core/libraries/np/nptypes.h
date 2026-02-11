#pragma once
#include <cstdint>

typedef uint32_t SceNpId;
typedef uint64_t SceNpOnlineId;

struct SceNpSignInCode {
  uint8_t code[16];
};

struct OrbisNpAuthorizationCode {
  uint8_t data[128];
};

struct OrbisNpIdToken {
  uint8_t data[1024];
};

struct OrbisNpClientId {
  char data[64];
};

struct OrbisNpClientSecret {
  char data[64];
};

using OrbisNpOnlineId = SceNpOnlineId;
