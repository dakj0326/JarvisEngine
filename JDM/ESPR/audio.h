//
// Created by david on 2026-02-28.
//

#ifndef JARVISWORKSPACE_AUDIO_H
#define JARVISWORKSPACE_AUDIO_H

#include <string>
#include <asio.hpp>
#include <vector>
#include <deque>

struct dataIndex {
    int jarvis;
    int other;
    int musicAndTV;
    int silence;
    int similar;
};

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

    void recordToWav(unsigned short port, dataIndex& indexes, bool jarvis, bool musicAndTV, bool similar);
    static void writeWav(const std::string& filename, const std::vector<int16_t>& samples, int sampleRate);

    static dataIndex initIndex();


public:
    UDPHandler();
    ~UDPHandler();
    void setUpStream(bool jarvis, bool musicAndTV, bool similar);
};

#endif //JARVISWORKSPACE_AUDIO_H