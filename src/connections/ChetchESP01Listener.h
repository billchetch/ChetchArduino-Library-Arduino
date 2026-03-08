
#ifndef CHETCH_ARDUINO_ESP01LISTENER_H
#define CHETCH_ARDUINO_ESP01LISTENER_H

#include <Arduino.h>

//#include <WiFiEsp.h> //WiFiEsp_h
#include <WiFiEspAT.h> //_WIFI_ESP_AT_H_

#define SERVER_PORT 80

namespace Chetch{
    class ESP01Listener : public Stream{
        private:
            const char* ssid;
            const char* pass;

        protected:
            int port;

        public:
            ESP01Listener(const char* ssid, const char* pass, int port);
            
            int connectToNetwork();
            bool isConnectedToNetwork();

            virtual int begin(Stream* stream);

            IPAddress getLocalIP(){ return WiFi.localIP(); };
    };
}
#endif