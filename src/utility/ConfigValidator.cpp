#include <fstream>
#include <sstream>
#include "../../include/utility/ConfigValidator.h"

std::string ConfigValidator::table_to_string(const toml::table& tbl) {
    std::stringstream ss;
    ss << tbl;
    return ss.str();
}

toml::table ConfigValidator::load_and_validate(const std::string& path) {
    toml::table config;

    try {
        if (!std::filesystem::exists(path)) {
            create_default(path);
            return toml::parse_file(path);
        }
        config = toml::parse_file(path);
    }
    catch (const toml::parse_error& err) {
        LOG_ERROR(std::format("Parsing failed: {0}", err.description()));
        create_default(path);
        return toml::parse_file(path);
    }

    std::string before = table_to_string(config);

    validate_entry(config, "server", "ip", toml::value("0.0.0.0"));

    validate_entry(config, "server", "port", toml::value(25565), [](const toml::node& n) {
        const auto val = n.as_integer()->get();
        return val > 0 && val <= 65535;
    });

    validate_entry(config, "server", "motd", toml::value("§aAetherius §7— §bC++ Core\n§eWelcome!"));
    validate_entry(config, "server", "online_mode", toml::value(true));
    validate_entry(config, "server", "max_players", toml::value(20));
    validate_entry(config, "server", "icon_path", toml::value("server-icon.png"));
    validate_entry(config, "server", "compression_enabled", toml::value(true));
    validate_entry(config, "server", "compression_threshold", toml::value(256));

    validate_entry(config, "world", "is_hardcore", toml::value(false));
    validate_entry(config, "world", "gamemode", toml::value(0));
    validate_entry(config, "world", "previous_gamemode", toml::value(255));
    validate_entry(config, "world", "world_count", toml::value(1));
    validate_entry(config, "world", "world_name", toml::value("minecraft:overworld"));
    validate_entry(config, "world", "hashed_seed", toml::value(0LL));
    validate_entry(config, "world", "view_distance", toml::value(10));
    validate_entry(config, "world", "reduced_debug_info", toml::value(false));
    validate_entry(config, "world", "enable_respawn_screen", toml::value(true));
    validate_entry(config, "world", "is_debug", toml::value(false));
    validate_entry(config, "world", "is_flat", toml::value(true));

    if (before != table_to_string(config)) {
        std::ofstream file(path);
        file << config;
        LOG_INFO("Configuration repaired and saved.");
    }

    return config;
}

void ConfigValidator::create_default(const std::string& path) {
    auto config = toml::table{
        { "server", toml::table{
            { "ip", "0.0.0.0" },
            { "port", 25565 },
            { "motd", "§aAetherius §7— §bC++ Core\n§eWelcome!" },
            { "max_players", 20 },
            { "online_mode", true },
            { "icon_path", "server-icon.png" },
            { "compression_enabled", true },
            { "compression_threshold", 256 }
        }},
        { "world", toml::table{
            { "is_hardcore", false },
            { "gamemode", 0 },
            { "previous_gamemode", 255 },
            { "world_count", 1 },
            { "world_name", "minecraft:overworld" },
            { "hashed_seed", 0LL },
            { "view_distance", 10 },
            { "reduced_debug_info", false },
            { "enable_respawn_screen", true },
            { "is_debug", false },
            { "is_flat", true }
        }}
    };

    std::ofstream file(path);
    file << config;
    file.close();
}
