#include "ChetchPinSelector.h"

namespace Chetch{
    
    PinSelector::PinSelector(SwitchDevice::SwitchMode::PASSIVE, byte firstPin, byte maxPins, int tolerance) : SwitchDevice(mode, firstPin, tolerance, LOW){
        this->firstPin = firstPin;
        watchPinFor = tolerance + 1;
    }
   

    bool PinSelector::begin(){
        SwitchDevice::begin();
        
        for(byte i = firstPin + 1; i < firstPin + maxPins; i++){
            initPin(i);
        }
        return begun;
    }

    void PinSelector::loop(){

        
        if(!isOn() && selectedPin == getPin()){

        }

        SwitchDevice::loop();
    }
    
    void PinSelector::trigger(){
        SwitchDevice::trigger();

        selectedPin = getPin();
    }
}