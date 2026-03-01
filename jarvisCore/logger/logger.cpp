//
// Created by david on 2026-02-26.
//

#include "logger.h"

#include <iostream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

void Logger::init() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_initialized = true;

    try {
        std::filesystem::create_directories("logs");

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);

        std::tm localTime{};

#ifdef _WIN32
        localtime_s(&localTime, &time);
#else
        localtime_r(&time, &localTime);
#endif

        std::stringstream dateStream;
        dateStream << std::put_time(&localTime, "%Y-%m-%d");
        std::string date = dateStream.str();

        // Find next file number
        int i = 0;
        for (const auto& entry : std::filesystem::directory_iterator("logs")) {
            std::string filename = entry.path().filename().string();

            if (filename.find("log_" + date) == 0) {
                size_t lastUnderscore = filename.find_last_of("_");
                size_t dot = filename.find(".txt");

                if (lastUnderscore != std::string::npos && dot != std::string::npos) {
                    std::string numberStr = filename.substr(lastUnderscore + 1, dot - lastUnderscore - 1);

                    int index = std::stoi(numberStr);
                    if (index > i)
                        i = index;
                }
            }
        }

        int newIndex = i + 1;

        std::stringstream filename;
        filename << "logs/log_" << date << "_"
                 << std::setw(4) << std::setfill('0') << newIndex
                 << ".txt";

        m_file.open(filename.str());

        if (!m_file.is_open()) {
            std::cerr << "ERROR LOGGER: Logger init failed to open new log file. Logs will still be printed in console" << std::endl;
            m_fileEnabled = false;
        } else {
            m_fileEnabled = true;
        }

    } catch (...) {
        std::cerr << "Logger: Failed to initialize logging directory.\n";
        m_fileEnabled = false;
    }

    m_initialized = true;
}

void Logger::log(const LogType level, const LogSender sender, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::stringstream output;
    output << "[" << getTimestamp() << "] "
           << "[" << typeToString(level) << "] "
           << "[" << senderToString(sender) << "] "
           << message;

    std::cout << output.str() << std::endl;

    if (m_fileEnabled) {
        m_file << output.str() << std::endl;
    }
}

std::string Logger::senderToString(LogSender sender) {
    switch (sender) {
        case LogSender::NO_SENDER: return "NO_SENDER";
        case LogSender::MAIN: return "MAIN";
    }
    return "UNKNOWN";
}

std::string Logger::typeToString(LogType type) {
    switch (type) {
        case LogType::TYPELESS: return "TYPELESS";
        case LogType::INFO: return "INFO";
        case LogType::WARNING: return "WARNING";
        case LogType::ERROR: return "ERROR";
        case LogType::DEBUG: return "DEBUG";
        case LogType::MISC: return "MISC";
        case LogType::NOTE: return "NOTE";
    }
    return "UNKNOWN";
}

std::string Logger::getTimestamp() {
    const auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &time);
#else
    localtime_r(&time, &localTime);
#endif

    std::stringstream ss;
    ss << std::put_time(&localTime, "%H:%M:%S");
    return ss.str();
}
