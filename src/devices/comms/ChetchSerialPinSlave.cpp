#include "ChetchUtils.h"
#include "ChetchSerialPinSlave.h"


namespace Chetch{

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
            
            if(bitCount >= 9){
                //Serial.print("<- EOR: ");
                //Serial.println(millis());
                bitCount = 0;
                ready4comms = false;
                
                frame[frameIdx] = data;
                frameIdx++;
                data = 0;

                if(frameIdx == frameSize){
                    frameIdx = 0;
                    if(dataListener != NULL){
                        dataListener(this, frame, frameSize);
                    }
                }
            }
        }
        
    }

} //end namespace
