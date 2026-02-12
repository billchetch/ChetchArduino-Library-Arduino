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
            if(bitCount == 0){
                //Set the line to LOW to indicate transmission to occur
                //Serial.print("-> SOT ");
                //Serial.println(millis());
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
                //Data has been transmitted so reset some stuff and return the line to high
                //Serial.print("-> EOT: ");
                //Serial.println(millis());
                bitCount = 0;
                sending = false;
                pinWrite(1);
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

   bool SerialPinMaster::send(byte b){
        if(sending){
            return false;
        } else {
            data = b;
            sending = true;
            bitCount = 0;
            return true;
        }
    }

    bool SerialPinMaster::executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response){
        bool handled = SerialPin::executeCommand(command, message, response);
        
        if(!handled){
            switch(command){
                case DeviceCommand::SEND:
                case DeviceCommand::TRANSMIT:
                    byte b2s = message->get<byte>(1);
                    send(b2s);
                    break;
            }
        }
        
        return handled;
    }

} //end namespace
