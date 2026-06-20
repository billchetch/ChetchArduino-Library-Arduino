#include "ChetchFloatSwitch.h"

namespace Chetch{
    FloatSwitch::FloatSwitch(byte lowPin, bool useOverflow, bool requireReset, int tolerance, bool onState) : SwitchArray(SwitchDevice::SwitchMode::PASSIVE, lowPin, useOverflow ? 3 : 2, tolerance, onState) 
    {
        this->useOverflow = useOverflow;
        this->requireReset = requireReset;
    }

    void FloatSwitch::reset(){
        waitForReset = false;
        setOnFlags(0x00);
    }

    void FloatSwitch::loop(){
        if(waitForReset)return;

        SwitchArray::loop();
    }

    

    void FloatSwitch::trigger(){
        SwitchArray::trigger();

        if(isOverflow() && requireReset){
            waitForReset = true;
        }
    }

}
