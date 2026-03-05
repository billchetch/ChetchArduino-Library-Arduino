#include "ChetchESP01UDPListener.h"


namespace Chetch{
    ESP01UDPListener::ESP01UDPListener(int udpPort, byte maxPacketSize)
    {
        this->udpPort = udpPort;
        buffer = new byte[maxPacketSize];
    }

    ESP01UDPListener::~ESP01UDPListener(){
        delete[] buffer;
    }

    int ESP01UDPListener::connectToNetwork(){
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
            udp.begin(udpPort);
        } else {
            Serial.print("Network status: ");
            Serial.print(WiFi.status());
        }
        return status;
    }

    bool ESP01UDPListener::isConnectedToNetwork(){
        return WiFi.status() == WL_CONNECTED;
    }

    void ESP01UDPListener::begin(Stream* stream){
        WiFi.init(stream);

        connectToNetwork();
    }

    int ESP01UDPListener::available(){
        int n = udp.parsePacket(); 
        if(n > 0 && bytesRead == 0){
            Serial.print("Available bytes: ");
            Serial.println(n);
            udp.read(buffer, n);
            bytesRead = n;
            bufferIdx = 0;
        }
        return bytesRead - bufferIdx;
    }

    int ESP01UDPListener::peek(){
        return -1;
    }

    int ESP01UDPListener::read() {
        if(bufferIdx >= bytesRead){
            bytesRead = 0; //so we can read in the next packet
            return -1;
        } else {
            byte b = buffer[bufferIdx];
            bufferIdx++;
            Serial.print("Read byte: ");
            Serial.println(b);
            return (int)b; //-1;*/
        }
    }

    size_t ESP01UDPListener::write(byte b){
        Serial.print("Write byte: ");
        Serial.println(b);
        return udp.write(b);
        //return 0;
    }
}