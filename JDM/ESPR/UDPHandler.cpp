//
// Created by david on 2026-02-28.
//

#include "UDPHandler.h"
#include <array>
#include <sstream>
#include <thread>

std::vector<std::string> UDPHandler::charsToWords(const std::string &chars) {
    std::vector<std::string> words;
    std::string word;
    std::istringstream iss(chars);
    while (iss >> word) {
        words.push_back(word);
    }
    return words;
}

void UDPHandler::listenToESP(unsigned short port, std::deque<int16_t> &buffer) {
    asio::io_context ioContext;
    asio::ip::udp::socket socket(ioContext, asio::ip::udp::endpoint(asio::ip::udp::v4(), port));
    std::array<char, 1024> recvBuffer;
    size_t sampleCount = 1024 / sizeof(int16_t);


    while (true) {
        asio::ip::udp::endpoint senderEndPoint;
        size_t bytesReceived = socket.receive_from(asio::buffer(recvBuffer), senderEndPoint);

        if (bytesReceived == 1024) {
            const int16_t* samples = reinterpret_cast<const int16_t*>(recvBuffer.data());

            buffer.insert(buffer.end(), samples, samples + sampleCount);
        }
    }
}


#include <asio/io_context.hpp>
#include <asio/steady_timer.hpp>
#include <chrono>

// Sends a keepalive msg to the ESP and updates last_keepalive
// Returns True if ESP acknowledges request.
// Return False if ESP does not acknowledge.
void UDPHandler::keepAlive(esp &esp) {
  std::string keepaliveMsg = "keepalive ";
  std::string ackMsg = globalCH.getValue("esp_general", "ack_msg");
  std::string keepaliveInterval = "1000";
  int maxRetries = 5;
  int retries = 0;

  asio::io_context ioctx;
  asio::steady_timer timer(ioctx);
  asio::ip::udp::socket socket(ioctx);
  socket.open(asio::ip::udp::v4());
  socket.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), esp.keepalivePort));
  asio::ip::udp::endpoint espEndpoint(asio::ip::make_address(esp.endpoint.address().to_string()), esp.keepalivePort);

  // keepalive ESP_0 10 meddelande
  string msg = keepaliveMsg + " " + esp.name + " " + keepaliveInterval;
  vector<string> msgWords = charsToWords(msg);

  array<char, 256> buffer;
  copy_n(msg.begin(), min(msg.size(), ackMsg.size() - 1), msg.begin());

  // Send keepalive to ESP
  cout << "Sending keepalive frame to: " << esp.name << " at: " << esp.endpoint.address() << ":" << esp.keepalivePort << endl;
  bool ack = false; // Om vi fått tbks en ack
  while (maxRetries >= retries  && !ack) {
    cout << "Sent keepalive frame to: " << esp.name << endl;
    socket.send_to(asio::buffer(buffer.data(), strlen(buffer.data())), espEndpoint);

    // Wait for response
    cout << "Awaiting keepalive ACK from: " << esp.name << endl;
    array<char, 1024> recvBuffer;

    // Start recive
    socket.async_receive_from(asio::buffer(recvBuffer), espEndpoint, [&](error_code ec, size_t bytesRecvd) {
      if (!ec) {
        // Kolla om svar korrekt
        string recvMsg = string(recvBuffer.data(), bytesRecvd);
        vector<string> rcvdWords = charsToWords(recvMsg);

        // Jämför
        if (rcvdWords == msgWords) {
          cout << "Recieved keepalive ACK from: " << esp.name << endl;
          esp.keepaliveInterval = stoi(keepaliveInterval);
          esp.keepaliveLast = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
          esp.status = "alive";
        }
      } else {
        cout << "keepalive recieve error: " << ec.message() << endl;
      }
    });

    //timeout timer - timeout efter keepalive interval
    timer.expires_after(chrono::seconds(stoi(keepaliveInterval)));
    timer.async_wait([&](error_code ec) {
      if (!ec) {
        cout << "keepalive recieve timeout, retrying..." << endl;
        socket.cancel();
      }
    });

    ioctx.run(); // Start sync funktioner. Blockar tills de är färdiga
    ioctx.reset(); // Preppa för att köra sync func igen
    retries++;
  }
  // Om vi inte fick svar
  if (!ack) {
    esp.status = "timeout";
  }
  return;
}
