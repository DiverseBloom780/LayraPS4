#include <cstring>
#include <ctime>
#include "discord_rpc_handler.h"

namespace DiscordRPCHandler {

void RPC::init() {
    // Stubbed out to fix build error
    rpcEnabled = false;
}

void RPC::setStatusIdling() {
    // Stubbed out to fix build error
}

void RPC::setStatusPlaying(const std::string& game_name, const std::string& game_id) {
    // Stubbed out to fix build error
}

void RPC::shutdown() {
    // Stubbed out to fix build error
    rpcEnabled = false;
}

bool RPC::getRPCEnabled() {
    return rpcEnabled;
}

} // namespace DiscordRPCHandler
