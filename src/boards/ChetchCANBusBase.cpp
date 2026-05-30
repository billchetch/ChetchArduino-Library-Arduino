#include "ChetchCANBusBase.h"

namespace Chetch{
    
    bool CANBusBase::sendBusMessage(ArduinoDevice* device, byte messageID){
        CANBusBase* canbus = (CANBusBase*)device->Board;
        return canbus->getMCP()->sendMessageForDevice(device, messageID);
    }

    CANBusBase::CANBusBase(MCP2515Device* pmcp, SerialPin* pspin) : ArduinoBoard(){
        this->pmcp = pmcp;
        this->pspin = pspin;

        pmcp->addMessageReceivedListener([](MCP2515Device* mcpdev, byte nodeID, ArduinoMessage* msg, byte* canData){
            //Capture this
            CANBusBase* busNode = (CANBusBase*)mcpdev->Board;

            busNode->handleReceivedBusMessage(nodeID, msg, canData);
        }); 
    }

    void CANBusBase::handleReceivedBusMessage(byte sourceNodeID, ArduinoMessage* messsge, byte* canData){
        //empty
    }

    void CANBusBase::loop(){
        ArduinoBoard::loop();

        if(begun){
            pmcp->readMessage();
        }
    }
}