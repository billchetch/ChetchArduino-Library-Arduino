#include "ChetchESP01UDPListener.h"


namespace Chetch{
    ESP01UDPListener::ESP01UDPListener(int udpPort, byte maxPacketSize)
    {
        this->udpPort = udpPort;
        this->maxPacketSize = maxPacketSize;
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
            // Connect to WPA/WPA2 network
            status = WiFi.begin(ssid, pass);
            delay(2000);
        }

        if(isConnectedToNetwork()){
            //Serial.print("IP: ");
            //Serial.println(WiFi.localIP());
            //Serial.println("Begin UDP...");
            if(udp.begin(udpPort)){
                //Serial.println("UDP begun!");
            } else {
                Serial.println("UDP failed to begin");
                return 0;
            }
        } else {
            //Serial.print("Network status: ");
            //Serial.print(WiFi.status());
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
        if(bytesRead == 0 && bufferIdx > 0){
            Serial.print("Write bytes: ");
            Serial.println(bufferIdx);
            
            if(udp.beginPacket(remoteIP, remotePort)){
                udp.write(buffer, bufferIdx);
                if(udp.endPacket()){
                    msent++;
                    Serial.print("Msg sent: ");
                    Serial.println(msent);
                    delay(5);
                } else {
                    Serial.print("End packet failure");
                }
            } else {
                Serial.println("Cound not begin packet");
            }
            bufferIdx = 0;
        }

        int n = udp.parsePacket(); 
        if(n > 0 && n <= maxPacketSize && bytesRead == 0){
            if(mrecv == 0){
                remoteIP = udp.remoteIP();
                remotePort = udp.remotePort();
                Serial.println("Client connected!");
            }

            mrecv++;
            Serial.print("Available bytes: ");
            Serial.println(n);
            Serial.print("Msg recv: ");
            Serial.println(mrecv);
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
            return -1;
        } else {
            byte b = buffer[bufferIdx];
            //Serial.print("Read byte: ");
            //Serial.println(b);
            
            bufferIdx++;
            if(bufferIdx >= bytesRead){
                delay(5);
                bytesRead = 0; //so we can read in the next packet
                bufferIdx = 0;
            }
            return (int)b; //-1;*/
        }
    }

    size_t ESP01UDPListener::write(byte b){
        if(bytesRead == 0 && bufferIdx < maxPacketSize){
            buffer[bufferIdx++] = b;
            return 1;
        }
        else
            return 0; //
        return 0;
    }
}