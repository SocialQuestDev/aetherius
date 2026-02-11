#include "commands/gameCommands/TimeCommand.h"
#include "game/player/Player.h"
#include "game/player/PlayerList.h"
#include "Server.h"
#include "network/packet/outbound/play/TimeUpdatePacket.h"
#include "console/Logger.h"

std::string TimeCommand::getName() const {
    return "time";
}

std::string TimeCommand::getDescription() const {
    return "Set or query world time. Usage: /time set <day|night|noon|midnight|<value>>  or  /time query";
}

void TimeCommand::execute(std::shared_ptr<Player> sender, const std::vector<std::string>& args) {
    if (args.empty()) {
        sender->sendChatMessage("§cUsage: /time set <day|night|noon|midnight|<value>>  or  /time query");
        return;
    }

    World& world = Server::get_instance().get_world();

    if (args[0] == "query") {
        int64_t time = world.getTimeOfDay();
        sender->sendChatMessage("§eWorld time: " + std::to_string(time) + " ticks");
        return;
    }

    if (args[0] == "set") {
        if (args.size() < 2) {
            sender->sendChatMessage("§cUsage: /time set <day|night|noon|midnight|<value>>");
            return;
        }

        int64_t newTime = 0;

        if (args[1] == "day") {
            newTime = 1000; // Morning
        } else if (args[1] == "noon") {
            newTime = 6000;
        } else if (args[1] == "sunset") {
            newTime = 12000;
        } else if (args[1] == "night") {
            newTime = 13000; // Night start
        } else if (args[1] == "midnight") {
            newTime = 18000;
        } else if (args[1] == "sunrise") {
            newTime = 0;
        } else {
            try {
                newTime = std::stoll(args[1]);
                if (newTime < 0) {
                    sender->sendChatMessage("§cTime must be a non-negative number!");
                    return;
                }
            } catch (const std::exception&) {
                sender->sendChatMessage("§cInvalid time value! Use: day, night, noon, midnight, sunrise, sunset, or a number.");
                return;
            }
        }

        world.setTimeOfDay(newTime);

        // Broadcast time update to all players
        TimeUpdatePacket timePacket(world.getWorldAge(), world.getTimeOfDay());
        for (const auto& player : PlayerList::getInstance().getPlayers()) {
            player->getConnection()->send_packet(timePacket);
        }

        sender->sendChatMessage("§aTime set to " + std::to_string(world.getTimeOfDay()) + " ticks");
        LOG_INFO(sender->getNickname() + " set time to " + std::to_string(world.getTimeOfDay()));
    } else {
        sender->sendChatMessage("§cUsage: /time set <value>  or  /time query");
    }
}
