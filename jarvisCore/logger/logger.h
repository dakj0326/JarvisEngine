//
// Created by david on 2026-02-26.
//

#ifndef JARVISENGINE_LOGGER_H
#define JARVISENGINE_LOGGER_H

#include <string>
#include <fstream>
#include <mutex>

enum class LogType {
    TYPELESS,
    INFO,
    NOTE,
    WARNING,
    ERROR,
    DEBUG,
    MISC,
};

enum class LogSender {
    NO_SENDER,
    MAIN,
};

class Logger {
public:
    static Logger& instance();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // Call in main to start the logger, without it the logger will be disabled.
    void init();

    // Logs a message in both console and log file if enabled.
    void log(LogType level, LogSender sender, const std::string& message);

private:
    Logger() = default;
    ~Logger() = default;

    std::string typeToString(LogType type);
    std::string senderToString(LogSender sender);
    std::string getTimestamp();

    std::ofstream m_file;
    std::mutex m_mutex;

    bool m_initialized = false;
    bool m_fileEnabled = false;

};

#endif //JARVISENGINE_LOGGER_H