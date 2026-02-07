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

    validate_entry(config, "server", "motd", toml::value("§aAetherius Server"));
    validate_entry(config, "server", "online_mode", toml::value(false));
    validate_entry(config, "server", "compression_threshold", toml::value(256));

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
            { "max_players", 100 },
            { "online_mode", true },
            { "icon_path", "server-icon.png" },
            { "compression_enabled", true },
            { "compression_threshold", 256 }
        }}
    };

    std::ofstream file(path);
    file << config;
    file.close();
}
