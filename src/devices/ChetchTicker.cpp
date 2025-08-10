#include "ChetchTicker.h"

namespace Chetch{
    
    Ticker::Ticker(){
        //empty
    }

    Ticker::Ticker(byte pin, unsigned int duration){
        this->pin = pin;
        int hd = (unsigned int)(duration / 2);
        setHighLowDuration(hd, duration - hd);
    }

    Ticker::Ticker(byte pin, unsigned int highDuration, unsigned int lowDuration){
        this->pin = pin;
        setHighLowDuration(highDuration, lowDuration);
    }

    void Ticker::setHighLowDuration(unsigned int highDuration, unsigned int lowDuration){
        pinHighDuration = highDuration;
        pinLowDuration = lowDuration;
        setReportInterval(1000); //default interval off 1 second
    }

    bool Ticker::begin(){
        pinMode(pin, OUTPUT);
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
            raiseEvent(EVENT_TICKED);
        } else if(pinState == HIGH && millis() - pinHighStartedOn > pinHighDuration){
            pinState = LOW;
            pinLowStartedOn = millis();
            digitalWrite(pin, pinState);
        }
    }
} //end namespace
