
#ifndef CHETCH_ARDUINO_ESP01UDPLISTENER_H
#define CHETCH_ARDUINO_ESP01UDPLISTENER_H

#include <Arduino.h>

//#include <WiFiEsp.h> //WiFiEsp_h
#include <WiFiEspAT.h> //_WIFI_ESP_AT_H_

#define UDP_PORT 18880

namespace Chetch{
    class ESP01UDPListener : public Stream{
        protected:
#ifdef WiFiEsp_h        
            ??
            ??
#endif
#ifdef _WIFI_ESP_AT_H_
            WiFiUDP udp;
#endif
            int udpPort = 0;
            byte* buffer;
            int maxPacketSize = 0;
            int bytesRead = 0;
            byte bufferIdx = 0;

            //temp
            unsigned int mrecv = 0;
            unsigned int msent = 0;
            IPAddress remoteIP;
            uint16_t remotePort;

        public:
            ESP01UDPListener(int udpPort, byte maxPacketSize = 255);
            ~ESP01UDPListener();
            
            int connectToNetwork();
            bool isConnectedToNetwork();
            IPAddress getLocalIP(){ return WiFi.localIP(); }

            void begin(Stream* stream);

            //Stream overrides
            int available() override;
            int peek() override;
            int read() override;
            size_t write(byte b) override;
    };
}
#endif