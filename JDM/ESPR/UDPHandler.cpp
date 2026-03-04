//
// Created by david on 2026-02-28.
//

#include "audio.h"
#include <array>
#include <sstream>
#include <asio/io_context.hpp>
#include <asio/socket_base.hpp>
#include <iostream>
#include <vector>
#include <fstream>
#include <filesystem>
#include <regex>
#include <cmath>
#include <chrono>
#include <thread>

UDPHandler::UDPHandler() {
    int broadcastPort = 9999;
    std::string msgIdentifer = "Hej Hej, jag heter ESP_0";
    std::string ackTemplate = "Hola ESP_010000";

    asio::ip::udp::resolver resolver(io_context);
    // Recieving endpoint
    asio::ip::udp::endpoint recvEndpoint;

    // Setup socket for recv. Listen to all ports on all interfaces
    asio::ip::udp::socket socket(io_context);
    socket.open(asio::ip::udp::v4());
    socket.set_option(asio::socket_base::broadcast(true)); // allow broadcast
    socket.set_option(asio::socket_base::reuse_address(true));
    socket.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), broadcastPort)); // Bind to all ports on all interfaces

    std::array<char, 256> recvBuffer; // 1024 is enough for ack

    std::cout << "Listening for ESPs..." << std::endl;


    // Listen for esp
    size_t len = socket.receive_from(asio::buffer(recvBuffer), recvEndpoint);
    std::cout << "Packet recieved, parsing frame..." << std::endl;
    std::string rcvdMsg = std::string(recvBuffer.data(), len);
    std::vector<std::string> rcvdWords = charsToWords(rcvdMsg);

    // Parse msg
    std::cout << "Received: " << rcvdMsg << std::endl;
    if (rcvdMsg == msgIdentifer) {
        esp.name = "ESP_0";
        esp.endpoint = asio::ip::udp::endpoint(asio::ip::make_address(recvEndpoint.address().to_string()), 10000);
        esp.keepAlivePort = 9000;
        std::cout << "Found ESP: " << esp.name << " at: " << esp.endpoint.address() << std::endl;

        // Handshake & stoppa in i vector om ESP svarar & vi inte redan pratar med espn

    } else {
      std::cout << "Packet missing identifier." << std::endl;
    }

    socket.send_to(asio::buffer(ackTemplate), recvEndpoint);

    std::cout << "Handshake completed with: " << std::endl;
}

UDPHandler::~UDPHandler() {return;}

std::vector<std::string> UDPHandler::charsToWords(const std::string &chars) {
    std::vector<std::string> words;
    std::string word;
    std::istringstream iss(chars);
    while (iss >> word) {
        words.push_back(word);
    }
    return words;
}

std::string makeFilename(const std::string& folder, const std::string& prefix, int index)
{
    std::ostringstream oss;
    oss << folder << "/"
        << prefix << "_"
        << std::setw(4)
        << std::setfill('0')
        << index
        << ".wav";

    return oss.str();
}

double computeRMS(const std::vector<int16_t>& data) {
    double sum = 0.0;

    for (auto s : data)
        sum += static_cast<double>(s) * s;

    return std::sqrt(sum / data.size());
}

void UDPHandler::recordToWav(unsigned short port, dataIndex& indexes, bool jarvis, bool musicAndTV, bool similar)
{
    if (jarvis + musicAndTV + similar > 1) {
        throw std::runtime_error("Misconfiguration: Multiple destinations are True.");
    }

    namespace fs = std::filesystem;

    asio::io_context ioContext;
    asio::ip::udp::socket socket(ioContext,
        asio::ip::udp::endpoint(asio::ip::udp::v4(), port));

    int secondsToRecord;
    const int sampleRate = 22050;
    if (jarvis) {
        secondsToRecord = 3;
    } else {
        secondsToRecord = 3;
    }
    const int totalSamples = sampleRate * secondsToRecord;

    const int gain = 16;
    const double silenceThreshold = 220.0;

    std::array<char, 1024> recvBuffer;

    while (true) {

        std::vector<int16_t> recorded;
        recorded.reserve(totalSamples);

        std::cout << "Recording..." << std::endl;

        while (recorded.size() < totalSamples) {

            asio::ip::udp::endpoint sender;
            size_t bytesReceived =
                socket.receive_from(asio::buffer(recvBuffer), sender);

            if (bytesReceived == 1024) {

                const int16_t* samples =
                    reinterpret_cast<const int16_t*>(recvBuffer.data());

                size_t sampleCount = bytesReceived / sizeof(int16_t);

                for (size_t i = 0; i < sampleCount; i++) {

                    int32_t amplified = samples[i] * gain;

                    if (amplified > 32767) amplified = 32767;
                    if (amplified < -32768) amplified = -32768;

                    recorded.push_back(static_cast<int16_t>(amplified));

                    if (recorded.size() >= totalSamples)
                        break;
                }
            }
        }

        double rms = computeRMS(recorded);

        std::string folder;
        std::string prefix;
        int* indexPtr = nullptr;

        if (rms < silenceThreshold) {
            folder = "../JDM/dataset/silence";
            prefix = "silence";
            indexPtr = &indexes.silence;
        }
        else if (jarvis) {
            folder = "../JDM/dataset/jarvis";
            prefix = "jarvis";
            indexPtr = &indexes.jarvis;
        }
        else if (musicAndTV) {
            folder = "../JDM/dataset/musicAndTV";
            prefix = "musicAndTV";
            indexPtr = &indexes.musicAndTV;
        }
        else if (similar) {
            folder = "../JDM/dataset/similar";
            prefix = "similar";
            indexPtr = &indexes.similar;
        }
        else {
            folder = "../JDM/dataset/other";
            prefix = "other";
            indexPtr = &indexes.other;
        }

        fs::create_directories(folder);

        std::string filename =
            makeFilename(folder, prefix, *indexPtr);

        writeWav(filename, recorded, sampleRate);

        std::cout << "Saved: " << filename
                  << " | RMS=" << rms << std::endl;

        (*indexPtr)++;
    }
}



void UDPHandler::writeWav(const std::string& filename, const std::vector<int16_t>& samples, int sampleRate)
{
    std::ofstream file(filename, std::ios::binary);

    int32_t subchunk2Size = samples.size() * sizeof(int16_t);
    int32_t chunkSize = 36 + subchunk2Size;
    int16_t audioFormat = 1;        // PCM
    int16_t numChannels = 1;        // Mono
    int16_t bitsPerSample = 16;
    int32_t byteRate = sampleRate * numChannels * bitsPerSample / 8;
    int16_t blockAlign = numChannels * bitsPerSample / 8;

    file.write("RIFF", 4);
    file.write(reinterpret_cast<char*>(&chunkSize), 4);
    file.write("WAVE", 4);

    file.write("fmt ", 4);
    int32_t subchunk1Size = 16;
    file.write(reinterpret_cast<char*>(&subchunk1Size), 4);
    file.write(reinterpret_cast<char*>(&audioFormat), 2);
    file.write(reinterpret_cast<char*>(&numChannels), 2);
    file.write(reinterpret_cast<char*>(&sampleRate), 4);
    file.write(reinterpret_cast<char*>(&byteRate), 4);
    file.write(reinterpret_cast<char*>(&blockAlign), 2);
    file.write(reinterpret_cast<char*>(&bitsPerSample), 2);

    file.write("data", 4);
    file.write(reinterpret_cast<char*>(&subchunk2Size), 4);

    file.write(reinterpret_cast<const char*>(samples.data()),
               subchunk2Size);

    file.close();
}

dataIndex UDPHandler::initIndex() {
    dataIndex indexes = {0, 0, 0 , 0};

    namespace fs = std::filesystem;

    if (!fs::exists("../JDM/dataset")) {
        fs::create_directory("../JDM/dataset");
    }

    std::vector<std::string> filepaths = {"../JDM/dataset/jarvis", "../JDM/dataset/other", "../JDM/dataset/musicAndTV", "../JDM/dataset/silence", "../JDM/dataset/similar"};

    for (int i = 0; i < filepaths.size(); i++) {
        if (!fs::exists(filepaths[i])) {
            fs::create_directory(filepaths[i]);
        }

        size_t pos = filepaths[i].find_last_of("/\\");
        std::string folderName = (pos == std::string::npos) ? filepaths[i] : filepaths[i].substr(pos + 1);
        std::regex pattern(folderName + "_(\\d+)\\.wav", std::regex_constants::icase);

        int maxIndex = 0;

        for (const auto& entry : fs::directory_iterator(filepaths[i])) {
            if (!entry.is_regular_file()) continue;

            std::string filename = entry.path().filename().string();
            std::smatch match;

            if (std::regex_match(filename, match, pattern)) {
                int index = std::stoi(match[1].str());

                if (index > maxIndex)
                    maxIndex = index;
            }
        }
        switch (i) {
            case 0: indexes.jarvis = maxIndex + 1; break;
            case 1: indexes.other = maxIndex + 1; break;
            case 2: indexes.musicAndTV = maxIndex + 1; break;
            case 3: indexes.silence = maxIndex + 1; break;
            case 4: indexes.similar = maxIndex + 1; break;

        }
    }
    return indexes;
}

// bool jarvis, bool musicAndTV, bool Similar.
// Set one to true and the other to false and start recording.
// If all are set to false, recording will be put in the "other" category
void UDPHandler::setUpStream(bool jarvis, bool musicAndTV, bool similar) {
    int port = esp.endpoint.port();
    dataIndex indexes = initIndex();
    recordToWav(port, indexes, jarvis, musicAndTV, similar);
}



