
#include "logger/logger.h"

int main() {
    Logger::instance().init();

    Logger::instance().log(LogType::INFO, LogSender::MAIN, "This is an info message");
    Logger::instance().log(LogType::ERROR, LogSender::MAIN, "This is an error message");
    Logger::instance().log(LogType::DEBUG, LogSender::NO_SENDER, "This is a debug message");
}
