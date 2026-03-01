//
// Created by david on 2026-02-28.
//

#ifndef JARVISWORKSPACE_AUDIO_H
#define JARVISWORKSPACE_AUDIO_H

#include <string>
#include <asio.hpp>
#include <vector>
#include <deque>

class UDPHandler {
private:
    struct ESP {
        std::string name;
        asio::ip::udp::endpoint endpoint;
        std::string status;
        int keepAlivePort;
        int keepAliveLast;
        int keepAliveInterval;
    };

    ESP esp;
    std::deque<int16_t> buffer;

    asio::io_context io_context;

    std::string broadCastAddr;
    std::string broadCastPort;

    static std::vector<std::string> charsToWords(const std::string &chars);

    std::string getBroadcast();
    void KeepAlive(ESP &esp);

public:
    UDPHandler();
    ~UDPHandler();
    void listenToESP(unsigned short port, std::deque<int16_t> &buffer);
};

#endif //JARVISWORKSPACE_AUDIO_H