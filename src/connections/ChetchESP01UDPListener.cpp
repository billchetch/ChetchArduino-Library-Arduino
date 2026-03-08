#include "ChetchESP01UDPListener.h"


namespace Chetch{
    ESP01UDPListener::ESP01UDPListener(const char* ssid, const char* pass, int port, byte maxPacketSize) : ESP01Listener(ssid, pass, port)
    {
        this->maxPacketSize = maxPacketSize;
        buffer = new byte[maxPacketSize];
    }

    ESP01UDPListener::~ESP01UDPListener(){
        delete[] buffer;
    }

    int ESP01UDPListener::begin(Stream* stream){
        int s = ESP01Listener::begin(stream);    

        if(isConnectedToNetwork()){
            //Serial.print("IP: ");
            //Serial.println(WiFi.localIP());
            //Serial.println("Begin UDP...");
            if(udp.begin(port)){
                //Serial.println("UDP begun!");
            } else {
                Serial.println("UDP failed to begin");
                return 0;
            }
        } else {
            //Serial.print("Network status: ");
            //Serial.print(WiFi.status());
        }
        
        return s;
    }

    int ESP01UDPListener::available(){
        if(bytesToRead > 0){
            //There are bytes on the buffer remaining to be read
            return bytesToRead - bufferIdx;
        } else if(bytesToRead == 0 && bufferIdx > 0){
            //the are bytes on the buffer that need to be written
            if(remotePort > 0){
                //If we have somewhere to send them then try and make a packet
                if(udp.beginPacket(remoteIP, remotePort)){
                    //Write to packet
                    udp.write(buffer, bufferIdx);

                    //test if packet ends (means passed to transmit buffer)
                    if(udp.endPacket()){
                        msent++;
                        Serial.print("-> Msent: ");
                        Serial.println(msent);
                        delay(1);
                    } else {
                        Serial.print("EPkt Fail");
                    }
                } else {
                    Serial.println("BPkt Fail");
                } //end udp packet construction
            } //end check remote port
            bufferIdx = 0;
            return 0;
        } else {
            //Since there are no bytes on the buffer we see if there are bytes on the network
            int n = udp.parsePacket();
            if(n > 0){
                //record the remote port and ip for sending data without making expensive AT calls
                if(remotePort == 0){
                    remoteIP = udp.remoteIP();
                    remotePort = udp.remotePort();
                }

                //read bytes in to buffer and set bufferIdx to 0 ready for reading
                bytesToRead = min(n, maxPacketSize);
                udp.read(buffer, bytesToRead);
                bufferIdx = 0;
            }
            return n;
        }
    }

    int ESP01UDPListener::peek(){
        return -1;
    }

    int ESP01UDPListener::read() {
        if(bufferIdx >= bytesToRead){ //got to the end of the buffer
            return -1;
        } else {
            byte b = buffer[bufferIdx];
            bufferIdx++;
            if(bufferIdx >= bytesToRead){
                Serial.println("Message read!");
                bytesToRead = 0; //so we can read in the next packet
                bufferIdx = 0;
                delay(1);
            }
            return (int)b; //-1;*/
        }
    }

    size_t ESP01UDPListener::write(byte b){
        if(bytesToRead == 0 && bufferIdx < maxPacketSize){
            buffer[bufferIdx++] = b;
            return 1;
        }
        else
            return 0; //
    }
}