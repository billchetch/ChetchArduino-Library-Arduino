#include "ChetchFloatSwitch.h"

namespace Chetch{
    FloatSwitch::FloatSwitch(byte lowPin, int tolerance, bool onState) : SwitchArray(SwitchDevice::SwitchMode::PASSIVE, lowPin, 2, tolerance, onState) 
    {
        
    }

    /*FloatSwitch::trigger(){
        SwitchArray::trigger();

    }*/

}
