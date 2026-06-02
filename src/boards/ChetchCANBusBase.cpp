#include "ChetchCANBusBase.h"

namespace Chetch{
    
    bool CANBusBase::sendBusMessage(ArduinoDevice* device, byte messageID){
        CANBusBase* canbus = (CANBusBase*)device->Board;
        return canbus->getMCP()->sendMessageForDevice(device, messageID);
    }

    CANBusBase::CANBusBase(MCP2515Device* pmcp, SerialPin* pspin) : ArduinoBoard(){
        this->pmcp = pmcp;
        this->pspin = pspin;

        
    }

    void CANBusBase::handleReceivedBusMessage(byte sourceNodeID, ArduinoMessage* message, byte* canData){
        //empty
        
    }

    bool CANBusBase::begin(MessageIO* io){
        pmcp->addMessageReceivedListener([](MCP2515Device* mcpdev, byte sourceNodeID, ArduinoMessage* message, byte* canData){
            
            
            //Capture this
            CANBusBase* busNode = (CANBusBase*)mcpdev->Board;

            busNode->handleReceivedBusMessage(sourceNodeID, message, canData);
        }); 
        return ArduinoBoard::begin(io);
    }

    void CANBusBase::loop(){
        ArduinoBoard::loop();

        if(begun){
            pmcp->readMessage();
        }
    }
}