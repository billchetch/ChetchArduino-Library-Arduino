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
                pinWrite(0);
                bitCount = 1;
            } else if(bitCount > 0 && bitCount < 9) {
                byte bit = (data >> (bitCount - 1)) & 0x01;
                pinWrite(bit);
                bitCount++;
            } else if(bitCount == 9) {
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


} //end namespace
