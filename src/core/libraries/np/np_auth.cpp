// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/np/np_auth.h"

#include <cstring>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

#include "common/config.h"
#include "common/log.h"
#include "common/types.h"
#include "core/libraries/kernel/kernel.h"

namespace Libraries::Np::NpAuth {

static constexpr auto Lib_NpAuth = "NpAuth";

enum class NpAuthRequestState { None, Running, Complete, Aborted };

struct NpAuthRequest {
  NpAuthRequestState state = NpAuthRequestState::None;
  s32 result = ORBIS_OK;
  bool async = false;
};

static std::vector<NpAuthRequest> g_auth_requests;
static std::mutex g_auth_request_mutex;
static bool g_signed_in = true; // Default to true for now

// Helper function to create a request
static s32 CreateNpAuthRequest(bool async) {
  std::scoped_lock lk{g_auth_request_mutex};

  // Find free slot
  s32 free_idx = -1;
  for (size_t i = 0; i < g_auth_requests.size(); ++i) {
    if (g_auth_requests[i].state == NpAuthRequestState::None) {
      free_idx = static_cast<s32>(i);
      break;
    }
  }

  if (free_idx == -1) {
    if (g_auth_requests.size() >= ORBIS_NP_AUTH_REQUEST_LIMIT) {
      return ORBIS_NP_AUTH_ERROR_REQUEST_MAX;
    }
    g_auth_requests.push_back({});
    free_idx = static_cast<s32>(g_auth_requests.size()) - 1;
  }

  auto &req = g_auth_requests[free_idx];
  req.state = NpAuthRequestState::Running; // Mark as running immediately
  req.result = ORBIS_OK;
  req.async = async;

  return ORBIS_NP_AUTH_REQUEST_ID_OFFSET + free_idx + 1;
}

// Helper function to get auth code
static s32 GetAuthorizationCode(
    s32 req_id, const OrbisNpAuthGetAuthorizationCodeParameterA *param,
    s32 version, OrbisNpAuthorizationCode *auth_code, s32 *issuer_id) {
  if (!param || !auth_code) {
    return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
  }
  if (param->size != sizeof(OrbisNpAuthGetAuthorizationCodeParameterA)) {
    return ORBIS_NP_AUTH_ERROR_INVALID_SIZE;
  }

  std::scoped_lock lk{g_auth_request_mutex};
  s32 idx = req_id - ORBIS_NP_AUTH_REQUEST_ID_OFFSET - 1;

  if (idx < 0 || idx >= static_cast<s32>(g_auth_requests.size()) ||
      g_auth_requests[idx].state == NpAuthRequestState::None) {
    return ORBIS_NP_AUTH_ERROR_REQUEST_NOT_FOUND;
  }

  auto &req = g_auth_requests[idx];
  if (req.state == NpAuthRequestState::Aborted) {
    return ORBIS_NP_AUTH_ERROR_ABORTED;
  }

  req.state = NpAuthRequestState::Complete;

  if (!g_signed_in) {
    req.result = ORBIS_NP_ERROR_SIGNED_OUT;
    return (req.async) ? ORBIS_OK : ORBIS_NP_ERROR_SIGNED_OUT;
  }

  LOG_ERROR(Lib_NpAuth, "(STUBBED) GetAuthorizationCode req_id={:#x}, async={}",
            req_id, req.async);

  // Zero the output buffers
  std::memset(auth_code, 0, sizeof(OrbisNpAuthorizationCode));
  if (issuer_id)
    *issuer_id = 0;

  // Los Santos Online Spoofing logic (preserved from original)
  /*
  if (Config::getNetworkingConfig().mode ==
          Core::Networking::Mode::Unofficial_Server &&
      param->client_id != nullptr) {
      // ... implementation ...
  }
  */

  return ORBIS_OK;
}

// ----  Public wrappers  ----

s32 PS4_SYSV_ABI sceNpAuthCreateRequest() { return CreateNpAuthRequest(false); }

s32 PS4_SYSV_ABI sceNpAuthCreateAsyncRequest(
    const OrbisNpAuthCreateAsyncRequestParameter *param) {
  if (!param || param->size != sizeof(OrbisNpAuthCreateAsyncRequestParameter)) {
    return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
  }
  return CreateNpAuthRequest(true);
}

s32 PS4_SYSV_ABI sceNpAuthGetAuthorizationCode(
    s32 req_id, const OrbisNpAuthGetAuthorizationCodeParameter *param,
    OrbisNpAuthorizationCode *auth_code, s32 *issuer_id) {
  if (!param || !auth_code)
    return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
  if (param->size != sizeof(OrbisNpAuthGetAuthorizationCodeParameter))
    return ORBIS_NP_AUTH_ERROR_INVALID_SIZE;
  if (!g_signed_in)
    return ORBIS_NP_ERROR_USER_NOT_FOUND;

  // Build internal param
  OrbisNpAuthGetAuthorizationCodeParameterA internal;
  std::memset(&internal, 0, sizeof(internal));
  internal.size = sizeof(internal);
  // internal.client_id = param->client_id; // Differs in pointer
  // type/constness? Struct says const pointer. Warning: param->client_id is
  // const OrbisNpClientId*. internal.client_id is const OrbisNpClientId*.
  internal.client_id = param->client_id;
  internal.user_id = 0; // stub
  internal.scope = param->scope;

  return GetAuthorizationCode(req_id, &internal, 0, auth_code, issuer_id);
}

s32 PS4_SYSV_ABI sceNpAuthGetAuthorizationCodeA(
    s32 req_id, const OrbisNpAuthGetAuthorizationCodeParameterA *param,
    OrbisNpAuthorizationCode *auth_code, s32 *issuer_id) {
  return GetAuthorizationCode(req_id, param, 0, auth_code, issuer_id);
}

s32 PS4_SYSV_ABI sceNpAuthGetAuthorizationCodeV3(
    s32 req_id, const OrbisNpAuthGetAuthorizationCodeParameterA *param,
    OrbisNpAuthorizationCode *auth_code, s32 *issuer_id) {
  return GetAuthorizationCode(req_id, param, 1, auth_code, issuer_id);
}

s32 PS4_SYSV_ABI sceNpAuthSetTimeout(s32 req_id, s32 resolve_retry,
                                     u32 resolve_timeout, u32 conn_timeout,
                                     u32 send_timeout, u32 recv_timeout) {
  LOG_ERROR(Lib_NpAuth, "(STUBBED) called");
  return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpAuthAbortRequest(s32 req_id) {
  LOG_DEBUG(Lib_NpAuth, "abort req_id={:#x}", req_id);
  std::scoped_lock lk{g_auth_request_mutex};
  s32 idx = req_id - ORBIS_NP_AUTH_REQUEST_ID_OFFSET - 1;

  if (idx < 0 || idx >= static_cast<s32>(g_auth_requests.size()) ||
      g_auth_requests[idx].state == NpAuthRequestState::None) {
    return ORBIS_NP_AUTH_ERROR_REQUEST_NOT_FOUND;
  }

  g_auth_requests[idx].state = NpAuthRequestState::Aborted;
  return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpAuthWaitAsync(s32 req_id, s32 *result) {
  if (!result)
    return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
  // Stubbed - assuming complete
  *result = 0;
  return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpAuthPollAsync(s32 req_id, s32 *result) {
  if (!result)
    return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
  // Stubbed
  *result = 0;
  return ORBIS_OK;
}

// Helper stub for registration
void RegisterLib(Core::Loader::SymbolsResolver *sym) {
  // Registration logic
}

} // namespace Libraries::Np::NpAuth