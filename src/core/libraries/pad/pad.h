#pragma once

#include "common/types.h"

namespace Libraries::Pad {
struct OrbisPadData {
  u32 buttons;
  u8 leftStickX;
  u8 leftStickY;
  u8 rightStickX;
  u8 rightStickY;
  u8 analogButtons[12];
  u32 reserved;
};

inline s32 PadInit() { return 0; }
inline s32 PadOpen(s32 userId, s32 type, s32 index, void *param) { return 0; }
} // namespace Libraries::Pad
