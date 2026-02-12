#pragma once

#include <string>
#include <unordered_map>

enum class ChatColor {
    BLACK,
    DARK_BLUE,
    DARK_GREEN,
    DARK_AQUA,
    DARK_RED,
    DARK_PURPLE,
    GOLD,
    GRAY,
    DARK_GRAY,
    BLUE,
    GREEN,
    AQUA,
    RED,
    LIGHT_PURPLE,
    YELLOW,
    WHITE,
    OBFUSCATED,
    BOLD,
    STRIKETHROUGH,
    UNDERLINED,
    ITALIC,
    RESET
};

const std::unordered_map<ChatColor, std::string> color_names = {
    {ChatColor::BLACK, "black"},
    {ChatColor::DARK_BLUE, "dark_blue"},
    {ChatColor::DARK_GREEN, "dark_green"},
    {ChatColor::DARK_AQUA, "dark_aqua"},
    {ChatColor::DARK_RED, "dark_red"},
    {ChatColor::DARK_PURPLE, "dark_purple"},
    {ChatColor::GOLD, "gold"},
    {ChatColor::GRAY, "gray"},
    {ChatColor::DARK_GRAY, "dark_gray"},
    {ChatColor::BLUE, "blue"},
    {ChatColor::GREEN, "green"},
    {ChatColor::AQUA, "aqua"},
    {ChatColor::RED, "red"},
    {ChatColor::LIGHT_PURPLE, "light_purple"},
    {ChatColor::YELLOW, "yellow"},
    {ChatColor::WHITE, "white"},
    {ChatColor::OBFUSCATED, "obfuscated"},
    {ChatColor::BOLD, "bold"},
    {ChatColor::STRIKETHROUGH, "strikethrough"},
    {ChatColor::UNDERLINED, "underlined"},
    {ChatColor::ITALIC, "italic"},
    {ChatColor::RESET, "reset"}
};

inline std::string to_string(ChatColor color) {
    auto it = color_names.find(color);
    if (it != color_names.end()) {
        return it->second;
    }
    return "white";
}
