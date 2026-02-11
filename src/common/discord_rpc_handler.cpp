#include <cstring>
#include <ctime>
#include "discord_rpc_handler.h"
namespace DiscordRPCHandler {
    void RPC::init() { rpcEnabled = false; }
    void RPC::setStatusIdling() {}
    void RPC::setStatusPlaying(const std::string& game_name, const std::string& game_id) {}
    void RPC::shutdown() { rpcEnabled = false; }
    bool RPC::getRPCEnabled() { return rpcEnabled; }
}