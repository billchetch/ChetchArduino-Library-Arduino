#include "ChetchUtils.h"
#include "ChetchMCP2515Device.h"


namespace Chetch{
    MCP2515Device::MCP2515Device(byte nodeID, int csPin) : 
                                    mcp2515(csPin), 
                                    amsg(ARDUINO_MESSAGE_SIZE)
#if CAN_FORWARD_MESSAGES 
                                    , fmsg(ARDUINO_MESSAGE_SIZE + 6) //Add 6 bytes for can ID (=4), can DLC (=1) and orig message type (=1)
#endif
    {
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
            randomSeed(getNodeID() + analogRead(A0));
            initialised = true;
        }
    }

    bool MCP2515Device::begin(){
        if(nodeID < MIN_NODE_ID || nodeID > MAX_NODE_ID){
            begun = false;
            return begun;
        }

        init();
        mcp2515.setBitrate(CAN_125KBPS);
#if CAN_AS_LOOPBACK 
        mcp2515.setLoopbackMode();
#else
        mcp2515.setNormalMode();
#endif
        if(indicatorPin > 0){
            pinMode(indicatorPin, OUTPUT);
            indicate(false);
        }
        begun = true;
        return begun;
	}

    void MCP2515Device::indicate(bool on){
        if(indicatorPin > 0)digitalWrite(indicatorPin, on ? HIGH : LOW);
    }

    void MCP2515Device::raiseError(MCP2515ErrorCode errorCode){
        lastError = errorCode;
        if(errorListener != NULL){
            errorListener(this, errorCode);
        }
   #if CAN_REPORT_ERRORS      
        enqueueMessageToSend(MESSAGE_ID_REPORT_ERROR);
    #endif
    }

    void MCP2515Device::setStatusInfo(ArduinoMessage* message){
        ArduinoDevice::setStatusInfo(message);
        message->add(CAN_FORWARD_MESSAGES);
        message->add(CAN_REPORT_ERRORS);
        message->add(getNodeID());
        message->add((byte)mcp2515.getStatus());
        message->add((byte)mcp2515.getErrorFlags());
        message->add((byte)mcp2515.errorCountTX());
        message->add((byte)mcp2515.errorCountRX());
    }

    void MCP2515Device::setReportInfo(ArduinoMessage* message){
        ArduinoDevice::setReportInfo(message);

        message->add((byte)mcp2515.getStatus());
        message->add((byte)mcp2515.getErrorFlags());
        message->add((byte)mcp2515.errorCountTX());
        message->add((byte)mcp2515.errorCountRX());
    }

    void MCP2515Device::populateOutboundMessage(ArduinoMessage* message, byte messageID){
        ArduinoDevice::populateOutboundMessage(message, messageID);


    #if CAN_FORWARD_MESSAGES
        if(messageID == MESSAGE_ID_FORWARD_RECEIVED){
            message->copy(&fmsg);
            //IMPORTANT: we identify forwarded messages as having the INFO type (original type is recorded as last parameter)
            message->type = ArduinoMessage::MessageType::TYPE_INFO;
        }
    #endif
    #if CAN_REPORT_ERRORS
        if(messageID == MESSAGE_ID_REPORT_ERROR){
            setErrorInfo(message, lastError);
            message->add((byte)mcp2515.getErrorFlags());
            message->add((byte)mcp2515.errorCountTX());
            message->add((byte)mcp2515.errorCountRX());
        }
    #endif
    }

    void MCP2515Device::loop(){
        indicate(false);
        ArduinoDevice::loop();

        readMessage();    
    
        if(!canTrySending && millis() > 2000){
            canTrySending = true;
        }
    }


    ArduinoMessage* MCP2515Device::getMessageForDevice(ArduinoDevice* device, ArduinoMessage::MessageType messageType){
        amsg.clear();
        amsg.type = messageType;
        amsg.sender = device->id - ArduinoBoard::START_DEVICE_IDS_AT;

        return &amsg;
    }

    void MCP2515Device::readMessage(){
        MCP2515::ERROR err = mcp2515.readMessage(&canInFrame);
        switch(err){
            case MCP2515::ERROR_OK:
                //Clear message and split out the ID
                amsg.clear();
                CANMessagePriority priority = (CANMessagePriority)(canInFrame.can_id >> 24) & 0b00001111;
                amsg.type = (canInFrame.can_id >> 16) & 0xFF;
                byte nodeIDAndSender = (canInFrame.can_id >> 8) & 0xFF;
                amsg.sender = nodeIDAndSender & 0b00001111; //last 4 bits make the sender
                byte remoteNodeID = nodeIDAndSender >> 4; //first 4 bits are used for the remote node ID

                if((byte)priority < (byte)CANMessagePriority::CAN_PRIORITY_CRITICAL || (byte)priority > (byte)CANMessagePriority::CAN_PRIORITY_LOW){
                    raiseError(INVALID_MESSAGE);
                    break; //ERROR.... 
                }

                if(remoteNodeID < MIN_NODE_ID || remoteNodeID > MAX_NODE_ID){
                    raiseError(INVALID_MESSAGE);
                    break; //ERROR....
                }

                if(amsg.type < 1 || amsg.type > 31){
                    raiseError(INVALID_MESSAGE);
                    break; //ERROR....
                }

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

                /*
                Serial.println("Received ID: ");
                for (int i = (sizeof(canInFrame.can_id) * 8) - 1; i >= 0; i--) { // Loop from bit 7 (MSB) down to 0 (LSB)
                    Serial.print(bitRead(canInFrame.can_id, i)); // Print the value of the i-th bit
                    if(i % 8 == 0)Serial.println("----");
                }*/
                
                canTrySending = true;
                indicate(true);

                //Call a listener if we have a message
                if(messageReceivedListener != NULL){
                    messageReceivedListener(this, priority, remoteNodeID, &amsg);
                }

#if CAN_FORWARD_MESSAGES
                fmsg.clear();
                fmsg.copy(&amsg);
                fmsg.add(canInFrame.can_id);
                fmsg.add((byte)canInFrame.can_dlc);
                fmsg.add(fmsg.type);
                fmsg.tag = MESSAGE_ID_FORWARD_RECEIVED;
                enqueueMessageToSend(MESSAGE_ID_FORWARD_RECEIVED);
#endif
                break;

            case MCP2515::ERROR_FAIL:
                raiseError(READ_FAIL);
                break;

            case MCP2515::ERROR_NOMSG: //ignore
                break;

            default:
                //Serial.println("Received something weird");
                raiseError(UNKNOWN_RECEIVE_ERROR);
                break;
        }
    }

    bool MCP2515Device::sendMessage(ArduinoMessage* message, CANMessagePriority messagePriority){
        if(!canTrySending)return false;

        if(message == NULL){
            raiseError(NO_MESSAGE);
            return false; //ERROR!
        }
        if(message->getArgumentCount() > 4){
            raiseError(INVALID_MESSAGE);
            return false; //ERROR ... can data of 8 bytes sets this limit
        }
        if(message->type > 31 || message->type < 1){
            raiseError(INVALID_MESSAGE);
            return false; //ERROR .... not a valid message type
        }
        if(message->sender > 15){
            raiseError(INVALID_MESSAGE);
            return false; //ERROR .... sender only has 4 bits available
        }
        if((byte)messagePriority > (byte)CANMessagePriority::CAN_PRIORITY_LOW){
            raiseError(INVALID_MESSAGE);
            return false; //ERROR .... priority not valid
        }

        canOutFrame.can_dlc = 0;
        byte messageStructure = 0;
        if(message->getArgumentCount() <= 1){
            canOutFrame.can_dlc = message->getArgumentCount() == 0 ? 0 : message->getArgumentSize(0);
            if(canOutFrame.can_dlc > CAN_MAX_DLC){
                return false; //ERROR ... can data of 8 bytes sets this limit
            }
        } else {
            byte shiftLeft = 6;
            messageStructure = ((message->getArgumentCount() - 1) << shiftLeft);
            for(int i = 0; i < message->getArgumentCount(); i++){
                if(message->getArgumentSize(i) > 4){
                    return false; //ERROR ... to keep structures representable by one byte we don't allow multi argument messages to have a single argument over 4 bytes
                }
                canOutFrame.can_dlc += message->getArgumentSize(i);
                if(canOutFrame.can_dlc > CAN_MAX_DLC){
                    return false; //ERROR ... can data of 8 bytes sets this limit
                }
                if(i < 3){
                    shiftLeft -= 2;
                    messageStructure = messageStructure | (message->getArgumentSize(i) - 1 << shiftLeft);
                }
            }
        }
        
        byte priority = 0; 
        if(messagePriority == CANMessagePriority::CAN_PRIORITY_RANDOM){
            priority = (byte)random(CANMessagePriority::CAN_PRIORITY_HIGH, CANMessagePriority::CAN_PRIORITY_LOW + 1);
        } else {
            priority = (byte)messagePriority;
        }
        priority = 0b00001111 & priority;
        byte nodeIDAndSender = (nodeID << 4 ) | (message->sender & 0b000001111);
        canOutFrame.can_id = (unsigned long)priority << 24 | (unsigned long)message->type << 16 | (unsigned long)nodeIDAndSender << 8 | (unsigned long)messageStructure;
        /*Serial.println("Sending ID: ");
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

        MCP2515::ERROR err = mcp2515.sendMessage(&canOutFrame);
        switch(err){
            case MCP2515::ERROR_OK:
                indicate(true);
                return true;
            case MCP2515::ERROR_FAILTX:
                raiseError(MCP2515ErrorCode::FAIL_TX);
                return false;
            case MCP2515::ERROR_ALLTXBUSY:
                raiseError(MCP2515ErrorCode::ALL_TX_BUSY);
                return false;
            default:
                raiseError(MCP2515ErrorCode::UNKNOWN_SEND_ERROR);
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
