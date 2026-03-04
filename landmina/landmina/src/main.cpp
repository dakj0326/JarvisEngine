
#include <Arduino.h>
#include <driver/i2s.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <mutex>
#include <type_traits>

#define I2S_WS 25                                   // Word Select pin
#define I2S_SD 33                                   // Serial Data pin
#define I2S_SCK 32                                  // Serial Clock pin

// I2S audio settings
#define SAMPLE_RATE 22050                           // Sample rate for I2S
#define BITS_PER_SAMPLE 16                          // 8, 16, 24, 32 bits per sample Annars blir I2S arg
#define BUFFER_SIZE 256                             // I2S buffer, max 256 Denna måste vara delbar med BITS_PER_SAMPLE
#define I2S_PORT I2S_NUM_0                          // Use I2S Processor 0
#define BYTES_PER_SAMPLE (BITS_PER_SAMPLE / 8)      // Bytes per sample

// Ge korrekt typ till sBuffer
using SampleType = typename std::conditional<BITS_PER_SAMPLE == 8, int8_t,
                    typename std::conditional<BITS_PER_SAMPLE == 16, int16_t,
                    typename std::conditional<BITS_PER_SAMPLE == 24 || BITS_PER_SAMPLE == 32, int32_t,
                    void>::type>::type>::type;

// Skapa buffer av dynamisk typ & små skit
SampleType sBuffer[(BUFFER_SIZE / (BITS_PER_SAMPLE / 8))];
int sBufferSize = (BUFFER_SIZE / (BITS_PER_SAMPLE / 8)) * BYTES_PER_SAMPLE; // Byte storlek av sBuffer
uint8_t audioData[BUFFER_SIZE];                     // I2S buffer
uint8_t* audioDataPtr = audioData;                  // Pointer till audiData

// Network settings
//const char *ssid = "NG24";                        // WiFi SSID
//const char *password = "123asdqwe";               // WiFi Password
const char* ssid = "TN_wifi_D737B5_EXT";
const char* password = "LDMAEJJWDU";

// UDP buffer settings
const int UDP_BUFFER_SIZE = 2048;                   // UDP Buffer. buffer >= packet + audio buffer and multiple of buffer
const int UDP_PACKET_SIZE = 1024;                   // Måste vara mindre än 1400 annars blir udp.write arg
static uint8_t data[UDP_PACKET_SIZE];               // Data som plockas ut och skickas i udpSend

// UDP settings
WiFiUDP udp;
int keepaliveInterval = 0;                          // Hur ofta vi förväntar oss keepalive
int keepaliveCounter = 0;                           // Packets sedan senaste keepalive togs emot
const int keepalivePort = 9000;                     // Keppalive Port
int remotePort = 10000;                             // Data port
const int broadcastPort = 9999;                     // Broadcast port
const String keepaliveMsg = "keepalive ";
const String name = "ESP_0";
const String greeting = "Hej Hej, jag heter " + name;
const String ackMsg = "Hola " + name;
IPAddress remoteIP;                                 // Pi's IP address


/************************************************************************************************
        Functions & Classes
*************************************************************************************************/

// Buffer där ljud bor
class buffer {
private:
    uint8_t buffer[UDP_BUFFER_SIZE];            // Storlek
    size_t writeIndex = 0;                      // Index för skriv
    size_t readIndex = 0;                       // Index för läs
    size_t count = 0;                           // element i buffern
    std::mutex mutex;                           // Ajabaja stygg försvar

public:
    // pushar en array, returns false om det inte finns plats, annars true
    bool push(const uint8_t* data, size_t len) {
        std::lock_guard<std::mutex> lock(mutex);
    
        if (len > (UDP_BUFFER_SIZE - count)) {
            return false;
        }
    
        size_t spaceLeft = UDP_BUFFER_SIZE - writeIndex;
        size_t bytesToWrite = std::min(len, spaceLeft);
    
        std::copy(data, data + bytesToWrite, buffer + writeIndex);
        writeIndex = (writeIndex + bytesToWrite) % UDP_BUFFER_SIZE; // Loopa tbks
    
        if (len > bytesToWrite) {
            size_t remainingBytes = len - bytesToWrite;
            std::copy(data + bytesToWrite, data + len, buffer);
            writeIndex = remainingBytes;
        }
    
        count += len;
        return true;
    }
    
    size_t size() {
        std::lock_guard<std::mutex> lock(mutex);
        return count;
    }
    
    // Returnerar lista med storlek udpBufferSize
    void retrieveAndShift(uint8_t* outputBuffer) {
        std::lock_guard<std::mutex> lock(mutex);

        if (count < UDP_PACKET_SIZE) {
            memset(outputBuffer, 0, UDP_PACKET_SIZE); // Skicka tbks 0
            return;
        }
        for (size_t i = 0; i < UDP_PACKET_SIZE; i++) {
            outputBuffer[i] = buffer[readIndex];
            readIndex = (readIndex + 1) % UDP_BUFFER_SIZE; // Loopa tbks
        }
        count -= UDP_PACKET_SIZE;
    }
};

buffer udpBuffer;                    // Skapa buffer for UDP data

// I2S Svammel
void i2s_install() {

    const i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = (i2s_bits_per_sample_t)BITS_PER_SAMPLE,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = 0,
        .dma_buf_count = 8,
        .dma_buf_len = (BUFFER_SIZE / (BITS_PER_SAMPLE / 8)), // number of saples in the buffer
        .use_apll = false
    };

    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
}

void i2s_setpin() {
    // Set I2S pin configuration
    const i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = -1,
        .data_in_num = I2S_SD};

    i2s_set_pin(I2S_PORT, &pin_config);
}

void broadcast(WiFiUDP &udp) {
    char buffer[255];
    String rcvdMsg;

    Serial.println("Broadcasting to IP: " + WiFi.broadcastIP().toString());
    udp.begin(broadcastPort);
    delay(50);
    while (true) {
        Serial.println("Sending UDP broadcast...");
        udp.beginPacket(WiFi.broadcastIP(), broadcastPort);
        udp.write((uint8_t *)greeting.c_str(), greeting.length());
        delay(1);
        udp.endPacket();
        delay(500); // Give time for response

        int packetSize = udp.parsePacket(); // Greppa packet
        Serial.printf("Packet Size Received: %d\n", packetSize);

        if (packetSize) {
            int readBytes = udp.read(buffer, sizeof(buffer) + 1);
            if (readBytes > 0) {
                buffer[readBytes] = '\0';
                rcvdMsg = String(buffer);
                rcvdMsg.trim(); // Bort med skumma grejer
            }

            Serial.println("Received message: " + rcvdMsg);
            if (ackMsg == rcvdMsg.substring(0, ackMsg.length())) {
                remotePort = rcvdMsg.substring(ackMsg.length()).toInt();
                remoteIP = udp.remoteIP();
                Serial.println("Acknowledgment received!");
                delay(500);
                udp.stop();
                return;
            }
        } else {
            Serial.println("Retrying..");
        }
    }
}

// Skicka från buffer över UDP
void udpSend(void* param) {
    uint time = 0; // Debug
    //int supposedTime = (int)((float)UDP_PACKET_SIZE / SAMPLE_RATE * 1000 / 2); // Debug
    while (true) {
        if (udpBuffer.size() >= UDP_PACKET_SIZE) {
            udpBuffer.retrieveAndShift(data); // Write directly into the static buffer
            udp.beginPacket(remoteIP, remotePort);
            udp.write(data, UDP_PACKET_SIZE);
            udp.endPacket();
            // Debug
            //Serial.printf("Sent packet of size: %d bytes, Time: %d ms, Target: %d ms\n", UDP_PACKET_SIZE, (millis() - time), supposedTime);
            time = millis();
        }
        vTaskDelay(10 / portTICK_PERIOD_MS); // Yield
    }
    return;
}


void setup() {
    Serial.begin(250000);
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting...");
    }
    Serial.println("Connected to WiFi");

    // Set up I2S for audio capture
    i2s_install();
    i2s_setpin();
    i2s_start(I2S_PORT);

    // Init UDP & broadcast to get Pi IP address
    broadcast(udp);         // Get remote IP & port
    udp.begin(remotePort);  // Data port
    delay(100);             // Wait for UDP
    Serial.println("Sending data to: " + remoteIP.toString() + ":" + remotePort);

    // Start udpSend tråd
    xTaskCreatePinnedToCore(
        udpSend,            // Function
        "udpSend",          // Task name
        4096,               // Stack size in words
        nullptr,            // Task parameter
        1,                  // Task priority
        nullptr,            // Task handle
        1                   // Core to run the task on (1 for APP core)
    );
}

void loop() {
    audioDataPtr = audioData;
    size_t bytesRead = 0;

    i2s_read(I2S_PORT, sBuffer, sBufferSize, &bytesRead, portMAX_DELAY);

    static int16_t prevInput = 0;
    static float prevOutput = 0;
    static const float alpha = 0.995f;

    size_t sampleCount = bytesRead / sizeof(int16_t);

    for (size_t i = 0; i < sampleCount; i++) {
        int16_t sample = sBuffer[i];

        float filtered = alpha * (prevOutput + sample - prevInput);

        prevInput = sample;
        prevOutput = filtered;

        int16_t finalSample =
            (int16_t)constrain(filtered, -32768, 32767);

        *audioDataPtr++ = finalSample & 0xFF;
        *audioDataPtr++ = (finalSample >> 8) & 0xFF;
    }

    if (!udpBuffer.push(audioData, sampleCount * 2)) {
        // avoid spamming serial in final build
    }
}