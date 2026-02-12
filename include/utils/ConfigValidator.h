#pragma once
#include <optional>
#include <string>
#include <filesystem>
#include <functional>
#include <format>

#define TOML_ENABLE_STD_OPTIONAL 1

#include <toml++/toml.hpp>

#include "console/Logger.h"

class ConfigValidator {
public:
    static toml::table load_and_validate(const std::string& path);

private:
    static std::string table_to_string(const toml::table& tbl);
    static void create_default(const std::string& path);

    template <typename T>
    static void validate_entry(toml::table& tbl, const std::string& section, const std::string& key, T&& default_val, const std::function<bool(const toml::node&)> validator = nullptr) {
        if (!tbl[section].is_table()) {
            tbl.insert_or_assign(section, toml::table{});
        }

        auto& sec_tbl = *tbl[section].as_table();

        if (!sec_tbl.contains(key)) {
            sec_tbl.insert_or_assign(key, std::forward<T>(default_val));
            LOG_WARN(std::format("Config: Missing [{0}].{1}. Using default.", section, key));
            return;
        }

        const toml::node* node = sec_tbl[key].node();
        if (!node) return;

        if (auto def_node = toml::value<typename std::decay_t<T>::value_type>(default_val); node->type() != def_node.type()) {
            sec_tbl.insert_or_assign(key, std::forward<T>(default_val));
            LOG_ERROR(std::format("Config: Type mismatch in [{0}].{1}. Resetting.", section, key));
            return;
        }

        if (validator && !validator(*node)) {
            sec_tbl.insert_or_assign(key, std::forward<T>(default_val));
            LOG_ERROR(std::format("Config: Invalid value in [{0}].{1}. Resetting.", section, key));
        }
    }
};