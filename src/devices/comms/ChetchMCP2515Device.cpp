#include "ChetchUtils.h"
#include "ChetchMCP2515Device.h"


namespace Chetch{
    MCP2515Device::MCP2515Device(byte nodeID, int csPin) : mcp2515(csPin), amsg(ARDUINO_MESSAGE_SIZE) {
        this->nodeID = nodeID;

    }

    void MCP2515Device::reset(){
        maskNum = 0;
        filterNum = 0;
        mcp2515.reset();
    }

    void MCP2515Device::init(){
        if(!initialised){
            reset();
            initialised = true;
        }
    }

    bool MCP2515Device::begin(){
        init();
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
        switch(err){
            case MCP2515::ERROR_OK:
                /*Serial.println("CAN ID: ");
                for (int i = (sizeof(canInFrame.can_id) * 8) - 1; i >= 0; i--) { // Loop from bit 7 (MSB) down to 0 (LSB)
                    Serial.print(bitRead(canInFrame.can_id, i)); // Print the value of the i-th bit
                    if(i % 8 == 0)Serial.println("----");
                }*/

                //Clear message and split out the ID
                amsg.clear();
                byte nodeID = (canInFrame.can_id >> 16) & 0xFF;
                byte messageTypeAndSender = (canInFrame.can_id >> 8) & 0xFF;
                amsg.type = (messageTypeAndSender & 0b11111000) >> 3;
                amsg.sender = messageTypeAndSender & 0b00000111;

                //Add message arguments
                if(canInFrame.can_dlc > 0){
                    byte messageStructure = canInFrame.can_id & 0x000000FF;
                    byte shiftRight = 6;
                    byte argCount = 1 + ((messageStructure >> shiftRight) & 0b00000011);
                    byte idx = 0;
                    byte argSize = 0;
                    for(int i = 0; i < argCount; i++){
                        if(i < 3 && argCount > 1){
                            shiftRight -= 2;
                            argSize = 1 + ((messageStructure >> shiftRight) & 0b00000011);
                        } else {
                            argSize = canInFrame.can_dlc - idx;
                        }
                        amsg.addBytes(&canInFrame.data[idx], argSize);
                        idx += argSize;
                    }
                }

                //Call a listener if we have a message
                if(messageReceivedListener != NULL){
                    messageReceivedListener(this, nodeID, &amsg);
                }
                break;

            default:
                //Serial.println("Received something weird");
                break;
        }
        
    }

    ArduinoMessage* MCP2515Device::getMessageForDevice(ArduinoDevice* device, ArduinoMessage::MessageType messageType){
        amsg.clear();
        amsg.type = messageType;
        amsg.sender= device->id - ArduinoBoard::START_DEVICE_IDS_AT;

        return &amsg;
    }


    bool MCP2515Device::isMessageFromDevice(byte nodeID, byte deviceIdx, ArduinoMessage* message){
        if(nodeID != this->nodeID)return false; //coming from a different bus
        if(deviceIdx != message->sender)return false;
        return true;
    }

    bool MCP2515Device::sendMessage(ArduinoMessage* message){
        if(message == NULL){
            return false; //ERROR!
        }
        if(message->getArgumentCount() > 4){
            return false; //ERROR ... can data of 8 bytes sets this limit
        }
        if(message->type > 31){
            return false; //ERROR .... type only has 5 bits available
        }
        if(message->sender > 7){
            return false; //ERROR .... sender only has 3 bits available
        }

        canOutFrame.can_dlc = 0;
        byte messageStructure = 0;
        if(message->getArgumentCount() <= 1){
            canOutFrame.can_dlc = message->getArgumentCount() == 0 ? 0 : message->getArgumentSize(0);
        } else {
            byte shiftLeft = 6;
            messageStructure = ((message->getArgumentCount() - 1) << shiftLeft);
            for(int i = 0; i < message->getArgumentCount(); i++){
                if(message->getArgumentSize(i) > 4){
                    return false; //ERROR ... to keep structures representable by one byte we don't allow multi argument messages to have a single argument over 4 bytes
                }
                canOutFrame.can_dlc += message->getArgumentSize(i);
                if(canOutFrame.can_dlc > 8){
                    return false; //ERROR ... can data of 8 bytes sets this limit
                }
                if(i < 3){
                    shiftLeft -= 2;
                    messageStructure = messageStructure | (message->getArgumentSize(i) - 1 << shiftLeft);
                }
            }
        }
        
        byte messageTypeAndSenderID = (message->type << 3 ) | (message->sender & 0b00000111);
        canOutFrame.can_id = (unsigned long)nodeID << 16 | (unsigned long)messageTypeAndSenderID << 8 | (unsigned long)messageStructure;
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

    uint32_t MCP2515Device::createFilterMask(bool checkNodeID, bool checkMessageType, bool checkSender){
        uint32_t mask = 0;
        
        //Node mask
        if(checkNodeID){
            mask = mask | 0x00FF0000; //second byte of can ID is considered
        }

        //Type mask (first 5 bites of 3rd byte)
        if(checkMessageType){
            mask = mask | 0x0000F800;
        }

        //Sender mask (last 3 bites of 3rd byte)
        if(checkSender){
            mask = mask | 0x00000700;
        }

        /*Serial.println("Mask: ");
        for (int i = (sizeof(mask) * 8) - 1; i >= 0; i--) { // Loop from bit 7 (MSB) down to 0 (LSB)
            Serial.print(bitRead(mask, i)); // Print the value of the i-th bit
            if(i % 8 == 0)Serial.println("----");
        }*/
        
        return mask;
    }

    uint32_t MCP2515Device::createFilter(int nodeID, int messageType, int sender){

        uint32_t filter = 0;

        if(nodeID != NO_FILTER){
            filter = filter | (uint32_t)(nodeID & 0x00FF) << 16;
        }

        //Type mask (first 5 bites of 3rd byte)
        if(messageType != NO_FILTER){

            filter = filter | (uint32_t)(messageType & 0x001F) << 11;
        }

        //Sender mask (last 3 bites of 3rd byte)
        if(sender != NO_FILTER){
            filter = filter | (uint32_t)(sender & 0x0007) << 8;
        }

        /*Serial.println("Filter: ");
        for (int i = (sizeof(filter) * 8) - 1; i >= 0; i--) { // Loop from bit 7 (MSB) down to 0 (LSB)
            Serial.print(bitRead(filter, i)); // Print the value of the i-th bit
            if(i % 8 == 0)Serial.println("----");
        }*/

        return filter;
    }

    bool MCP2515Device::addFilter(uint32_t mask, uint32_t filter){
        init(); //make sure we are initialised otherwise begin will cause a reset erasing the filter and mask values

        if(filterNum >= 5)return false;

        MCP2515::ERROR err;
        err = mcp2515.setFilterMask(maskNum, true, mask);
        if(err != MCP2515::ERROR_OK){
            return false;
        }

        err = mcp2515.setFilter(filterNum, true, filter);
        if(err != MCP2515::ERROR_OK){
            return false;
        }

        filterNum++;
        if(filterNum == 2)maskNum = 1;

        return true;
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
