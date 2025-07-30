#include "ChetchUtils.h"
#include "ChetchMCP2515Device.h"


namespace Chetch{
    MCP2515Device::MCP2515Device(byte busID, byte nodeID, int csPin) : mcp2515(csPin) {
       busAndNodeID = busID << 5 | (0b0011111 & nodeID);
    }

    bool MCP2515Device::begin(){
        mcp2515.reset();
        mcp2515.setBitrate(CAN_125KBPS);
#if CAN_AS_LOOPBAck 
        mcp2515.setLoopbackMode();
#else
        mcp2515.setNormalMode();
#endif
	}
    
    void MCP2515Device::loop(){
        ArduinoDevice::loop();

        MCP2515::ERROR err = mcp2515.readMessage(&canInFrame);
        ArduinoMessage msg(16);
        switch(err){
            case MCP2515::ERROR_OK:
                //split out the ID and check

                //Make the message

                //Call a listener if we have a message
                if(messageReceivedListener != NULL){
                    messageReceivedListener(this, &msg);
                }
                break;

            default:
                //Serial.println("Received something weird");
                break;
        }

    }

    bool MCP2515Device::sendMessage(ArduinoMessage* message){
        if(message->getArgumentCount() > 4){
            return false; //ERROR
        }
        canOutFrame.can_dlc = 0;
        byte shiftLeft = 6;
        byte messageStructure = message->getArgumentCount() - 1 << shiftLeft;
        for(int i = 0; i < message->getArgumentCount(); i++){
            if(message->getArgumentSize(i) > 4){
                return false; //ERROR
            }
            canOutFrame.can_dlc += message->getArgumentSize(i);
            if(canOutFrame.can_dlc > 8){
                return false; //ERROR
            }
            if(i < 3){
                shiftLeft -= 2;
                messageStructure = messageStructure | (message->getArgumentSize(i) - 1 << shiftLeft);
            }
        }
        /*Serial.print("CAN dlc ");
        Serial.println(canOutFrame.can_dlc);
        Serial.print("Bits of the byte: ");
        for (int i = 7; i >= 0; i--) { // Loop from bit 7 (MSB) down to 0 (LSB)
            Serial.print(bitRead(messageStructure, i)); // Print the value of the i-th bit
        }        
        return true;*/

        byte messageTypeAndSenderID;
        switch(message->type){
            case ArduinoMessage::TYPE_ERROR:
                messageTypeAndSenderID = 0b00100000 | message->sender;
                break;
            case ArduinoMessage::TYPE_DATA:
                messageTypeAndSenderID = 0b01000000 | message->sender;;
                break;
            default:
                messageTypeAndSenderID = 0b00000000 | message->sender;;
                break;
        }
        canOutFrame.can_id = (unsigned long)busAndNodeID << 16 | messageTypeAndSenderID << 8 | messageStructure;
        /*Serial.println("Bits of the byte: ");
        for (int i = (sizeof(canOutFrame.can_id) * 8) - 1; i >= 0; i--) { // Loop from bit 7 (MSB) down to 0 (LSB)
            Serial.print(bitRead(canOutFrame.can_id, i)); // Print the value of the i-th bit
            if(i % 8 == 0)Serial.println("----");
        }  
        Serial.println("----");*/
        
        
        canOutFrame.can_id |= CAN_EFF_FLAG;
        int n = 0;
        for(int i = 0; i < message->getArgumentCount(); i++){
            byte* b = message->getArgument(i);
            for(int j = 0; j < message->getArgumentSize(i); j++){
                canOutFrame.data[n++] = b[j];
            }
        }

        if(mcp2515.sendMessage(&canOutFrame) == MCP2515::ERROR_OK){
            return true;
        } else {
            return false;
        }
    }

    bool MCP2515Device::executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response){
        bool handled = ArduinoDevice::executeCommand(command, message, response);
        
        if(!handled)
        {
            //switch(command){
            //}
        }
        return handled;
    }
}
