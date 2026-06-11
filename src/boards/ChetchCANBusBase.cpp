#include "ChetchCANBusBase.h"

namespace Chetch{
    
    CANBusBase::CANBusBase(MCP2515Device* pmcp, SerialPin* pspin) : ArduinoBoard(){
        this->pmcp = pmcp;
        this->pspin = pspin;        
    }
}