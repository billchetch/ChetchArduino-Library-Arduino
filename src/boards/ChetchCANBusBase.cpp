#include "ChetchCANBusBase.h"

namespace Chetch{
    
    bool CANBusBase::sendBusMessage(ArduinoDevice* device, byte messageID){
        CANBusBase* canbus = (CANBusBase*)device->Board;

        return true; //canbus->enqueueBusMessage(device, messageID);
    }

    CANBusBase::CANBusBase(MCP2515Device* pmcp, SerialPin* pspin) : ArduinoBoard(){
        this->pmcp = pmcp;
        this->pspin = pspin;        
    }

    void CANBusBase::handleReceivedBusMessage(byte sourceNodeID, ArduinoMessage* message, byte* canData){
        //empty
    }

    
}