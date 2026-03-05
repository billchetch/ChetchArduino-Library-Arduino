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
            Serial.print("Attempting to connect to WPA SSID: ");
            Serial.println(ssid);
            // Connect to WPA/WPA2 network
            status = WiFi.begin(ssid, pass);
            delay(2000);
        }

        if(isConnectedToNetwork()){
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());
            Serial.println("Starting server...");
            server.begin();
        } else {
            Serial.print("Network status: ");
            Serial.print(WiFi.status());
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
            if(client)Serial.println("Client with data available!");
            return 0;
        }

        //We know client object evaluates to true here otherwise the return above would be triggered
        if(client.connected()){
            return client.available();
        } else {
            Serial.println("client disconnected");
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