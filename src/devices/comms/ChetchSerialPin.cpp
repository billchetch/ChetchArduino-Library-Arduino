#include "ChetchUtils.h"
#include "ChetchSerialPin.h"


namespace Chetch{
    SerialPin::SerialPin(Role role, byte pin)
    {
        this->role = role;
        this->pin = pin;
    }
    
    
    bool SerialPin::begin(){

        if(role == Role::MASTER){
            Serial.print("Setting to master!");
            pinMode(pin, OUTPUT);
            pinWrite(HIGH);
        } else {
            pinMode(pin, INPUT_PULLUP);
        }

        begun = true;
        return begun;
	}


    void SerialPin::loop(){
        ArduinoDevice::loop();

        if(!ready4comms){
            if(pinRead() == 1){
                ready4comms = true;
            } else {
                return;
            }
        }

        //receive mode
        /*if(bitCount == 0 && pinRead() == 0 && millis() - lastPinIO > interval){
            interval = 100 + 25;
            bitCount = 1;
        } else if(millis() - lastPinIO > interval){
            byte bit = pinRead();
            data = data << 1 | bit;
            bitCount++;
            interval = 100;

            if(bitCount >= 9){
                bitCount = 0;
                data = 0;
                ready4comms = false;
            }
        }*/

        //transmit mode
        if(sending && (millis() - lastPinIO > interval)){
            if(bitCount == 0){
                pinWrite(0);
                interval = 100; 
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

    void SerialPin::pinWrite(byte bit){
        Serial.print("Bit ");
        Serial.print(bitCount);
        Serial.print("=");
        Serial.println(bit);

        digitalWrite(pin, bit);
        lastPinIO = millis();
    }

    byte SerialPin::pinRead(){
        lastPinIO = millis();
        return digitalRead(pin) & 0x01;
    }

    bool SerialPin::send(byte b){
        if(sending){
            return false;
        } else {
            data = b;
            sending = true;
            interval = 100;
            bitCount = 0;
            return true;
        }
    }


} //end namespace
