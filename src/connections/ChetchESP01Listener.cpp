#include "ChetchESP01Listener.h"


namespace Chetch{
    ESP01Listener::ESP01Listener(int port)
    {
        this->port = port;
    }

    int ESP01Listener::connectToNetwork(){
        char ssid[] = "Bulan Baru Internet";            // your network SSID (name)
        char pass[] = "bulanbaru";   

        // attempt to connect to WiFi network
        int status = WL_IDLE_STATUS;
        while ( status != WL_CONNECTED) {
            Serial.print("Attempting to connect to WPA SSID: ");
            Serial.println(ssid);
            // Connect to WPA/WPA2 network
            status = WiFi.begin(ssid, pass);
            delay(2000);
        }

        return status;
    }

    bool ESP01Listener::isConnectedToNetwork(){
        return WiFi.status() == WL_CONNECTED;
    }

    void ESP01Listener::begin(Stream* stream){
        WiFi.init(stream);

        connectToNetwork();
    }
}