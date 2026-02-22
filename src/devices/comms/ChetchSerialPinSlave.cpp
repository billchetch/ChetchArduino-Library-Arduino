#include "ChetchUtils.h"
#include "ChetchSerialPinSlave.h"


namespace Chetch{
    SerialPinSlave::SerialPinSlave(byte pin, int interval, byte bufferSize) : SerialPin(pin, interval, bufferSize){
        data2send = new byte[this->bufferSize];
    }

    SerialPinSlave::~SerialPinSlave(){
        delete[] data2send;
    }

    bool SerialPinSlave::begin(){
        pinMode(pin, INPUT_PULLUP);
        
        //Serial.print("Slippage is: ");
        //Serial.println(slippage);

        begun = true;
        return begun;
	}


    void SerialPinSlave::loop(){
        SerialPin::loop();

        if(!ready4comms)return;

        //receive mode
        if(bitCount == 0 && intervalElapsed() && pinRead(false) == 0){
            lastPinIO = millis();
            bitCount = 1;
            //Serial.print("<- SOR: ");
            //Serial.println(millis());
        } else if(bitCount > 0 && intervalElapsed(bitCount == 1 ? interval/4 : 0)){
            unsigned int ival = millis() - lastPinIO;
            byte bit = pinRead(true);
            /*Serial.print("<< B");
            Serial.print(bitCount);
            Serial.print("=");
            Serial.print(bit);
            Serial.print(" ival=");
            Serial.println(ival);*/
            data = data | (bit << (bitCount - 1));
            bitCount++;
            
            if(bitCount >= 9){ //byte received
                //Serial.print("<- EOR: ");
                //Serial.println(millis());
                bitCount = 0;
                ready4comms = false;
                
                buffer[bufferIdx] = data;
                data2send[bufferIdx] = data; //keep a copy for sending

                bufferIdx++;
                data = 0;

                if(bufferIdx == bufferSize){
                    bufferIdx = 0;
                    
                    enqueueMessageToSend(MESSAGE_ID_SEND_DATA);
                    if(dataListener != NULL){
                        dataListener(this, buffer, bufferSize);
                    }
                }
            }
        } //end counting bits  
    }

    void SerialPinSlave::populateOutboundMessage(ArduinoMessage* message, byte messageID){
        ArduinoDevice::populateOutboundMessage(message, messageID);

        if(messageID == MESSAGE_ID_SEND_DATA){

            message->type = ArduinoMessage::TYPE_DATA;
            message->addBytes(data2send, bufferSize);
        }
    }
} //end namespace
