#include "ChetchESP01TCPListener.h"


namespace Chetch{
    ESP01TCPListener::ESP01TCPListener(const char* ssid, const char* pass, int port) : ESP01Listener(ssid, pass, port),
                server(port)
    {
        
    }

    int ESP01TCPListener::begin(Stream* stream){
        int s = ESP01Listener::begin(stream);

        if(isConnectedToNetwork()){
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());
            Serial.println("Starting server...");
            server.begin();
        } else {
            Serial.print("Network status: ");
            Serial.print(WiFi.status());
        }
        return s;
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