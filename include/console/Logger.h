#pragma once
#include <string>
#include <functional>
#include <mutex>

#define ANSI_RESET              "\x1b[0m"
#define ANSI_BOLD               "\x1b[1m"
#define ANSI_INVERSE            "\x1b[7m"

#define ANSI_COLOR_CYAN         "\x1b[36m"
#define ANSI_COLOR_GREEN        "\x1b[32m"
#define ANSI_COLOR_YELLOW       "\x1b[33m"
#define ANSI_COLOR_RED          "\x1b[31m"
#define ANSI_COLOR_WHITE        "\x1b[37m"
#define ANSI_COLOR_GRAY         "\x1b[90m"

#define ANSI_COLOR_256_CYAN           "\x1b[38;5;51m"
#define ANSI_COLOR_256_DARK_CYAN      "\x1b[38;5;30m"
#define ANSI_COLOR_256_GREEN          "\x1b[38;5;46m"
#define ANSI_COLOR_256_DARK_GREEN     "\x1b[38;5;34m"
#define ANSI_COLOR_256_YELLOW         "\x1b[38;5;226m"
#define ANSI_COLOR_256_DARK_YELLOW    "\x1b[38;5;172m"
#define ANSI_COLOR_256_RED            "\x1b[38;5;196m"
#define ANSI_COLOR_256_DARK_RED       "\x1b[38;5;124m"
#define ANSI_COLOR_256_WHITE          "\x1b[38;5;255m"
#define ANSI_COLOR_256_GRAY           "\x1b[38;5;244m"
#define ANSI_COLOR_256_DARK_GRAY      "\x1b[38;5;236m"

enum class LogLevel {
    INFO,
    DEBUG,
    WARNING,
    ERROR,
    FATAL
};

struct LogEntry {
    LogLevel Level;
    std::string Timestamp;
    std::string Message;
    std::string File;
    int Line;
};

class Logger {
public:
    Logger() = default;

    using LogCallback = std::function<void(LogLevel, const std::string&)>;

    static void Log(LogLevel level, const std::string& message,
                    const char* function, const char* file, int line);

    static std::vector<LogEntry>& GetHistory();
    static void ClearHistory();

    static void SetCallback(const LogCallback &callback) {
        GetCallback() = callback;
    }

private:
    static LogCallback& GetCallback();

    static std::vector<LogEntry> s_History;
    static std::mutex s_Mutex;
};

#define LOG_INFO(msg)     Logger::Log(LogLevel::INFO, msg, __FUNCTION__, __FILE__, __LINE__)
#define LOG_DEBUG(msg)    Logger::Log(LogLevel::DEBUG, msg, __FUNCTION__, __FILE__, __LINE__)
#define LOG_WARN(msg)     Logger::Log(LogLevel::WARNING, msg, __FUNCTION__, __FILE__, __LINE__)
#define LOG_ERROR(msg)    Logger::Log(LogLevel::ERROR, msg, __FUNCTION__, __FILE__, __LINE__)
#define LOG_FATAL(msg)    Logger::Log(LogLevel::FATAL, msg, __FUNCTION__, __FILE__, __LINE__)