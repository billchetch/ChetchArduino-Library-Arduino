
#ifndef CHETCH_ARDUINO_ESP01UDPLISTENER_H
#define CHETCH_ARDUINO_ESP01UDPLISTENER_H

#include <Arduino.h>
#include "connections/ChetchESP01Listener.h"

#define ESP_UDP_DEFAULT_PORT 18800

namespace Chetch{
    class ESP01UDPListener : public ESP01Listener{
        protected:
#ifdef WiFiEsp_h        
            ??
            ??
#endif
#ifdef _WIFI_ESP_AT_H_
            WiFiUDP udp;
#endif
            byte* buffer;
            int maxPacketSize = 0;
            int bytesToRead = 0;
            byte bufferIdx = 0;
            IPAddress remoteIP;
            uint16_t remotePort;

            //temp
            unsigned int mrecv = 0;
            unsigned int msent = 0;
            

        public:
            ESP01UDPListener(const char* ssid, const char* pass, int port = ESP_UDP_DEFAULT_PORT, byte maxPacketSize = 64);
            ~ESP01UDPListener();
            
            //Listener overrides
            int begin(Stream* stream) override;

            //Stream overrides
            int available() override;
            int peek() override;
            int read() override;
            size_t write(byte b) override;
    };
}
#endif