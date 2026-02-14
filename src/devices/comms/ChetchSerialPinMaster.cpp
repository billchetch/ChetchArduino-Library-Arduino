#include "ChetchUtils.h"
#include "ChetchSerialPinMaster.h"


namespace Chetch{

    
    bool SerialPinMaster::begin(){
        pinMode(pin, OUTPUT);
        pinWrite(HIGH);
        
        begun = true;
        return begun;
	}

    void SerialPinMaster::loop(){
        SerialPin::loop();

        if(!ready4comms)return;

        //transmit mode
        if(sending && intervalElapsed()){
            //get byte to send
            if(bitCount == 0){
                //Set the line to LOW to indicate transmission to occur
                pinWrite(0);
                bitCount = 1;
            } else if(bitCount > 0 && bitCount < 9) {
                //Transmit the data
                byte bit = (data >> (bitCount - 1)) & 0x01;
                /*Serial.print(">> B");
                Serial.print(bitCount);
                Serial.print("=");
                Serial.print(bit);
                Serial.print(" ival=");
                Serial.println(millis() - lastPinIO);*/

                pinWrite(bit);
                bitCount++;
            } else if(bitCount == 9) {
                //Byte has been transmitted so reset some stuff and return the line to high
                bitCount = 0;
                pinWrite(1);
                
                //Increment frame index to get next byte to send
                frameIdx++;
                if(frameIdx == frameSize){
                    //Serial.print("-> EOT: ");
                    //Serial.println(millis());
                
                    sending = false;
                    frameIdx = 0;
                } else {
                    data = frame[frameIdx]; //get next byte to send
                }
            }
        }
        
    }

    void SerialPinMaster::pinWrite(byte bit){
        /*Serial.print("Bit ");
        Serial.print(bitCount);
        Serial.print("=");
        Serial.println(bit);*/

        digitalWrite(pin, bit);
        lastPinIO = millis();
    }

    bool SerialPinMaster::send(byte *bytes, byte byteCount){
        for(byte i = 0; i < byteCount; i++){
            if(!send(bytes[i]))return false;
        }

        return true;
    }

    bool SerialPinMaster::send(byte b){
        if(sending)return false;

        frame[frameIdx] = b;
        frameIdx++; 

        if(frameIdx == frameSize){
            //send the frame
            frameIdx = 0;
            bitCount = 0;
            data = frame[0]; //get first byte to send
            sending = true;
        }
        return true;
    }

    bool SerialPinMaster::send(int argv){
        return send((byte*)&argv, sizeof(argv));
    }

    bool SerialPinMaster::send(long argv){
        return send((byte*)&argv, sizeof(argv));
    }

    bool SerialPinMaster::executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response){
        bool handled = ArduinoDevice::executeCommand(command, message, response);
        
        if(!handled){
            switch(command){
                case DeviceCommand::SEND:
                case DeviceCommand::TRANSMIT:
                    for(byte i = 1; i < message->getArgumentCount(); i++){
                        byte bytec = message->getArgumentSize(i);
                        if(!send(message->getArgument(i), bytec))break;
                    }
                    break;
            }
        }
        
        return handled;
    }

} //end namespace
