#include "ChetchESP01Listener.h"


namespace Chetch{
    ESP01Listener::ESP01Listener(const char* ssid, const char* pass, int port)
    {
        this->ssid = ssid;
        this->pass = pass;
        this->port = port;
    }

    int ESP01Listener::connectToNetwork(){
        // attempt to connect to WiFi network
        int status = WL_IDLE_STATUS;
        while ( status != WL_CONNECTED) {
            // Connect to WPA/WPA2 network
            status = WiFi.begin(ssid, pass);
            delay(100);
        }

        return status;
    }

    bool ESP01Listener::isConnectedToNetwork(){
        return WiFi.status() == WL_CONNECTED;
    }

    int ESP01Listener::begin(Stream* stream){
        WiFi.init(stream);

        return connectToNetwork();
    }
}