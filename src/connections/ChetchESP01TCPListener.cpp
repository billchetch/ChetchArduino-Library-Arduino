#include "ChetchESP01TCPListener.h"


namespace Chetch{
    ESP01TCPListener::ESP01TCPListener() : server(SERVER_PORT)
    {
        //empty
    }

    int ESP01TCPListener::connectToNetwork(){
        char ssid[] = "Bulan Baru Internet";            // your network SSID (name)
        char pass[] = "bulanbaru";   

        // attempt to connect to WiFi network
        int status = WL_IDLE_STATUS;
        while ( status != WL_CONNECTED) {
            //Serial.print("Attempting to connect to WPA SSID: ");
            Serial.println(ssid);
            // Connect to WPA/WPA2 network
            status = WiFi.begin(ssid, pass);
        }

        if(isConnectedToNetwork()){
            //Serial.println("Starting server...");
            server.begin();
        }
        return status;
    }

    bool ESP01TCPListener::isConnectedToNetwork(){
        return WiFi.status() == WL_CONNECTED;
    }

    void ESP01TCPListener::begin(Stream* stream){
        WiFi.init(stream);

        connectToNetwork();
    }

    int ESP01TCPListener::available(){
        if(!client){
            client = server.available();
            return 0;
        }

        if(client.connected()){
            return client.available();
        } else {
            return 0;
        }
    }

    int ESP01TCPListener::peek(){
        return client ? client.peek() : -1;
    }

    int ESP01TCPListener::read() {
        return client ? client.read() :  -1;
    }

    size_t ESP01TCPListener::write(byte b){
        if(client){
            return client.write(b);
        } else {
            return 0;
        }
    }
}