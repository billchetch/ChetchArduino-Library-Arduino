
#ifndef CHETCH_ARDUINO_ESP01LISTENER_H
#define CHETCH_ARDUINO_ESP01LISTENER_H

#include <Arduino.h>

//#include <WiFiEsp.h> //WiFiEsp_h
#include <WiFiEspAT.h> //_WIFI_ESP_AT_H_

#define SERVER_PORT 80

namespace Chetch{
    class ESP01Listener : public Stream{
        protected:
            int port;

        public:
            ESP01Listener(int port);
            
            virtual int connectToNetwork();
            bool isConnectedToNetwork();

            void begin(Stream* stream);

            
    };
}
#endif