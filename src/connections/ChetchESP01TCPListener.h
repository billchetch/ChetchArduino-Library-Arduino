
#ifndef CHETCH_ARDUINO_ESP01TCPLISTENER_H
#define CHETCH_ARDUINO_ESP01TCPLISTENER_H

#include <Arduino.h>
#include <WiFiEsp.h>

#define SERVER_PORT 80

namespace Chetch{
    class ESP01TCPListener : public Stream{
        protected:
            WiFiEspServer server;
            WiFiEspClient client;

        public:
            ESP01TCPListener();
            
            int connectToNetwork();
            bool isConnectedToNetwork();

            void begin(Stream* stream);

            //Stream overrides
            int available() override;
            int peek() override;
            int read() override;
            size_t write(byte b) override;
    };
}
#endif