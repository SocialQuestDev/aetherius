#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>

#ifdef _WIN32
#include <windows.h>
#endif

#include "../include/Logger.h"

std::vector<LogEntry> Logger::s_History;
std::mutex Logger::s_Mutex;

Logger::LogCallback& Logger::GetCallback() {
    static LogCallback callback = nullptr;
    return callback;
}

std::vector<LogEntry>& Logger::GetHistory() {
    std::lock_guard<std::mutex> lock(s_Mutex);
    return s_History;
}

void Logger::ClearHistory() {
    std::lock_guard<std::mutex> lock(s_Mutex);
    s_History.clear();
}

void Logger::Log(const LogLevel level, const std::string& message,const char* function, const char* file, const int line) {
    #ifdef _WIN32
    static bool consoleInitialized = []() {
        auto enableVT = [](DWORD handle_id) {
            HANDLE hOut = GetStdHandle(handle_id);
            if (hOut != INVALID_HANDLE_VALUE) {
                DWORD dwMode = 0;
                if (GetConsoleMode(hOut, &dwMode)) {
                    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
                }
            }
        };

        enableVT(STD_OUTPUT_HANDLE);
        enableVT(STD_ERROR_HANDLE);
        return true;
    }();
    #endif

    const auto now = std::chrono::system_clock::now();
    const auto time_t = std::chrono::system_clock::to_time_t(now);
    const std::tm* local_tm = std::localtime(&time_t);

    std::string levelStr;
    std::string msgColor = ANSI_RESET;
    std::string labelColor;

    std::ostream& outStream = (level == LogLevel::ERROR || level == LogLevel::CRITICAL)
                              ? std::cerr
                              : std::cout;

    switch (level) {
        case LogLevel::INFO:     levelStr = " INFO   "; labelColor = ANSI_INVERSE ANSI_COLOR_WHITE; msgColor = ANSI_COLOR_WHITE; break;
        case LogLevel::DEBUG:    levelStr = " DEBUG  "; labelColor = ANSI_INVERSE ANSI_COLOR_GREEN; msgColor = ANSI_COLOR_GREEN; break;
        case LogLevel::WARNING:  levelStr = "WARNING "; labelColor = ANSI_INVERSE ANSI_COLOR_YELLOW; msgColor = ANSI_COLOR_YELLOW; break;
        case LogLevel::ERROR:    levelStr = " ERROR  "; labelColor = ANSI_INVERSE ANSI_COLOR_RED; msgColor = ANSI_COLOR_RED; break;
        case LogLevel::CRITICAL: levelStr = "CRITICAL"; labelColor = ANSI_INVERSE ANSI_COLOR_256_DARK_RED; msgColor = ANSI_COLOR_256_DARK_RED; break;
    }

    const std::string filePath(file);
    const size_t lastSlash = filePath.find_last_of("\\/");
    const std::string fileName = (lastSlash == std::string::npos) ? filePath : filePath.substr(lastSlash + 1);

    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        std::stringstream timeStream;
        if (local_tm) timeStream << std::put_time(local_tm, "%H:%M:%S");
        s_History.push_back(LogEntry{level, timeStream.str(), message, fileName, line});
    }

    if (local_tm) {
        outStream
            << ANSI_COLOR_GRAY << "[" << std::put_time(local_tm, "%H:%M:%S") << "]" << ANSI_RESET
            << " " << labelColor << " " << levelStr << " " << ANSI_RESET
            << " " << ANSI_COLOR_CYAN << "[" << function << " in " << fileName << ":" << line << "]" << ANSI_RESET
            << " " << msgColor << " " << message << ANSI_RESET
            << std::endl;
    }

    if (auto& callback = GetCallback()) {
        std::stringstream ss;
        if (local_tm) ss << "[" << std::put_time(local_tm, "%H:%M:%S") << "] ";
        ss << "[" << levelStr << "] ";
        ss << "[" << fileName << ":" << line << "] " << message;

        callback(level, ss.str());
    }
}