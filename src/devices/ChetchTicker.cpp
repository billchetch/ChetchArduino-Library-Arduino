#include "ChetchTicker.h"

namespace Chetch{
    
    Ticker::Ticker(){
        //empty
    }

    Ticker::Ticker(unsigned int highDuration, unsigned int lowDuration){
        setHighLowDuration(highDuration, lowDuration);
    }

    void Ticker::setHighLowDuration(unsigned int highDuration, unsigned int lowDuration){
        pinHighDuration = highDuration;
        pinLowDuration = lowDuration;
    }

    void Ticker::setReportInfo(ArduinoMessage* message){
        message->add(tickCount);
    }

	void Ticker::loop(){
        ArduinoDevice::loop();
        

        if(pinState == LOW && millis() - pinLowStartedOn > pinLowDuration){
            pinState = HIGH;
            pinHighStartedOn = millis();
            tickCount++;
            digitalWrite(pin, pinState);
        } else if(pinState == HIGH && millis() - pinHighStartedOn > pinHighDuration){
            pinState = LOW;
            pinLowStartedOn = millis();
            digitalWrite(pin, pinState);
        }
    }
} //end namespace
