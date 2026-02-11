#include "ChetchUtils.h"
#include "ChetchSerialPinSlave.h"


namespace Chetch{

    SerialPinSlave::SerialPinSlave(byte pin, int interval) : SerialPin(pin, interval){
        slippage = interval / 4;
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
        if(bitCount == 0 && intervalElapsed() && pinRead() == 0){
            bitCount = 1;
            //Serial.println("Receiving...");
        } else if(bitCount > 0 && intervalElapsed(bitCount == 1 ? slippage : 0)){
            byte bit = pinRead();
            //Serial.print("Received=");
            //Serial.println(bit);
            data = data | (bit << (bitCount - 1));
            bitCount++;
            
            if(bitCount >= 9){
                bitCount = 0;
                ready4comms = false;

                raiseEvent(EVENT_DATA_RECEIVED, data);

                data = 0;
            }
        }
        
    }

} //end namespace
