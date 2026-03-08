
#ifndef CHETCH_ARDUINO_ESP01TCPLISTENER_H
#define CHETCH_ARDUINO_ESP01TCPLISTENER_H

#include <Arduino.h>

#include "connections/ChetchESP01Listener.h"

namespace Chetch{
    class ESP01TCPListener : public ESP01Listener{
        protected:
#ifdef WiFiEsp_h        
            WiFiEspServer server;
            WiFiEspClient client;
#endif
#ifdef _WIFI_ESP_AT_H_
            WiFiServer server;
            WiFiClient client;
#endif

        public:
            ESP01TCPListener(const char* ssid, const char* pass, int port);
            
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