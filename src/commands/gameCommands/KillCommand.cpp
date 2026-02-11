#include "commands/gameCommands/KillCommand.h"
#include "network/packet/outbound/play/ChatMessagePacket.h"
#include "game/player/Player.h"
#include "game/player/PlayerList.h"
#include "console/Logger.h"
#include "other/ChatColor.h"

void KillCommand::execute(std::shared_ptr<Player> player, const std::vector<std::string>& args) {
    if (player) {
        if (!args.empty()) {
            std::string target_name = args[0];
            auto target = PlayerList::getInstance().getPlayer(target_name);
            if (!target) {
                LOG_INFO("Player not found: " + target_name);
                return;
            }
            target->kill();
        }
        else {
            player->sendChatMessage("Usage: /kill <player>");
        }
    }
    else {
        if (args.empty()) {
            std::string target_name = args[0];
            auto target = PlayerList::getInstance().getPlayer(target_name);
            if (!target) {
                LOG_INFO("Player not found: " + target_name);
                return;
            }
            target->kill();
        }
        else {
            LOG_INFO("Usage: /kill <player>");
        }
    }
}

std::string KillCommand::getName() const {
    return "kill";
}

std::string KillCommand::getDescription() const {
    return "Kills a player.";
}
