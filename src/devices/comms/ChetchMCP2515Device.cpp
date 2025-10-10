#include "ChetchUtils.h"
#include "ChetchMCP2515Device.h"


namespace Chetch{
    MCP2515Device::MCP2515Device(byte nodeID, int csPin) : 
                                    mcp2515(csPin, 4000000), 
                                    amsg(ARDUINO_MESSAGE_SIZE)
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

    void MCP2515Device::raiseError(MCP2515ErrorCode errorCode, unsigned int errorData){
        lastError = errorCode;
        lastErrorData = errorData;

        if(errorListener != NULL){
            errorListener(this, errorCode);
        }
    }

    void MCP2515Device::loop(){
        indicate(false);
        ArduinoDevice::loop();

        readMessage();    
    
        if(!canSend && millis() > 2000){
            allowSending();
        }

        if(lastError != MCP2515ErrorCode::NO_ERROR){
            //send error message
            ArduinoMessage* msg = getMessageForDevice(this, ArduinoMessage::MessageType::TYPE_ERROR, 1);
            msg->add(lastError);
            msg->add(lastErrorData);
            msg->add(mcp2515.getErrorFlags());
            msg->add(mcp2515.getStatus());

            sendMessage(msg);

            //reset code
            lastError = MCP2515ErrorCode::NO_ERROR;
            lastErrorData = 0;
        }
    }

    bool MCP2515Device::allowSending(){
        if(!canSend){
            canSend = true;
            raiseEvent(EVENT_READTY_TO_SEND);
            return true;
        } else {
            return false;
        }
    }

    byte MCP2515Device::crc5(byte* data, byte len){
        if(len == 0)return 0;

        static byte generator = (0b00110101 & 0x1F) << 3; //x^5 + x^4 + x^2 + 1 ...
        //static byte generator = 0b00100101; //x^5 + x^2 + 1 ... 
        byte crc = 0;
        for (byte i = 0; i < len; i++) {
            crc ^= data[i];
            for (byte k = 0; k < 8; k++) {
                crc = crc & 0x80 ? (crc << 1) ^ generator : crc << 1;
            }
        }
        crc >>= 3;
        return crc & 0x1F;
    }

    bool MCP2515Device::vcrc5(byte crc, byte* data, byte len){
        return crc == crc5(data, len);
    }

    ArduinoMessage* MCP2515Device::getMessageForDevice(ArduinoDevice* device, ArduinoMessage::MessageType messageType, byte tag){
        amsg.clear();
        amsg.type = messageType;
        amsg.tag = tag;
        amsg.sender = device->id - ArduinoBoard::START_DEVICE_IDS_AT;

        return &amsg;
    }

    void MCP2515Device::readMessage(){
        MCP2515::ERROR err = mcp2515.readMessage(&canInFrame);
        switch(err){
            case MCP2515::ERROR_OK:
                //Clear message and split out the ID
                amsg.clear();
                byte messageType = (canInFrame.can_id >> 24) & 0x1F; //first 5 bits
                byte nodeIDAndSender = (canInFrame.can_id >> 16) & 0xFF; //whole byte split 4 | 4
                byte tagAndCRC = (canInFrame.can_id >> 8) & 0xFF; //whole byte split 3 | 5
                if(!vcrc5(tagAndCRC & 0x1F, canInFrame.data, canInFrame.can_dlc)){
                    //data error
                    raiseError(CRC_ERROR, tagAndCRC & 0x1F);
                    return;
                }

                amsg.type = messageType;
                amsg.tag = (tagAndCRC >> 5) & 0x07;
                amsg.sender = nodeIDAndSender & 0x0F; //last 4 bits make the sender
                byte sourceNodeID = nodeIDAndSender >> 4 & 0x0F; //first 4 bits are used for the remote node ID

                //Do some basic validationg
                if(sourceNodeID < MIN_NODE_ID || sourceNodeID > MAX_NODE_ID){
                    raiseError(INVALID_MESSAGE);
                    return; //ERROR....
                }

                if(amsg.type < 1 || amsg.type > 31){
                    raiseError(INVALID_MESSAGE);
                    return; //ERROR....
                }

                if(amsg.tag > 7){
                    raiseError(INVALID_MESSAGE);
                    return; //ERROR....
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
                
                //By here we have received and successfully parsed an ArduinoMessage
                allowSending();
                indicate(true); 

                handleReceivedMessage(sourceNodeID, &amsg);
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

    void MCP2515Device::handleReceivedMessage(byte sourceNodeID, ArduinoMessage* message){
            
        //The concern here is that a messageReceivedListener will often send out a message
        //This would then potentially cause problems with correct forwarding of messages in the case of a master nod
        //For now we use the flag 'handled' to determine whether a message listener is called or not.
        bool handled = false;

        //check the message type in case we need to handle messages directed to this device specifically
        ArduinoDevice::DeviceCommand command;
        ArduinoMessage* msg;

        switch(message->type){
            case ArduinoMessage::TYPE_STATUS_REQUEST:
                msg = getMessageForDevice(this, ArduinoMessage::TYPE_STATUS_RESPONSE, message->tag);
                msg->add(mcp2515.getStatus());
                msg->add(mcp2515.getErrorFlags());
                msg->add(mcp2515.errorCountTX());
                msg->add(mcp2515.errorCountRX());
                
                //NOTE: We are sending out a message during a possible readMessage execution
                sendMessage(msg);
                handled = true;
                break;

            case ArduinoMessage::TYPE_COMMAND:
                if(commandListener != NULL && message->getArgumentCount() > 0){
                    command = message->get<ArduinoDevice::DeviceCommand>(0);
                    commandListener(this, sourceNodeID, command, message);
                    handled = true;
                } else {
                    handled = false;
                }
                break;

            default:
                handled = false;
                break;
        }

        if(!handled && messageReceivedListener != NULL){
            messageReceivedListener(this, sourceNodeID, message);
        }
    }

    bool MCP2515Device::sendMessage(ArduinoMessage* message){
        if(!canSend)return false;

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
        if(message->tag > 7){
            raiseError(INVALID_MESSAGE);
            return false; //ERROR .... tag values can only be 0 - 7
        }
        if(message->sender > 15){
            raiseError(INVALID_MESSAGE);
            return false; //ERROR .... sender only has 4 bits available
        }
        
        //Determine data length and message structure from original message
        canOutFrame.can_dlc = 0;
        byte messageStructure = 0;
        if(message->getArgumentCount() <= 1){
            canOutFrame.can_dlc = message->getArgumentCount() == 0 ? 0 : message->getArgumentSize(0);
            if(canOutFrame.can_dlc > CAN_MAX_DLC){
                raiseError(INVALID_MESSAGE);
                return false; //ERROR ... can data of 8 bytes sets this limit
            }
        } else {
            byte shiftLeft = 6;
            messageStructure = ((message->getArgumentCount() - 1) << shiftLeft);
            for(int i = 0; i < message->getArgumentCount(); i++){
                if(message->getArgumentSize(i) > 4){
                    raiseError(INVALID_MESSAGE);
                    return false; //ERROR ... to keep structures representable by one byte we don't allow multi argument messages to have a single argument over 4 bytes
                }
                canOutFrame.can_dlc += message->getArgumentSize(i);
                if(canOutFrame.can_dlc > CAN_MAX_DLC){
                    raiseError(INVALID_MESSAGE);
                    return false; //ERROR ... can data of 8 bytes sets this limit
                }
                if(i < 3){
                    shiftLeft -= 2;
                    messageStructure = messageStructure | (message->getArgumentSize(i) - 1 << shiftLeft);
                }
            }
        }
        
        byte messageType = message->type & 0x1F;
        byte nodeIDAndSender = (nodeID << 4 ) | (message->sender & 0x0F);
        byte tagAndCRC = (message->tag << 5) | crc5(canOutFrame.data, canOutFrame.can_dlc);
        canOutFrame.can_id = (unsigned long)messageType << 24 | (unsigned long)nodeIDAndSender << 16 | (unsigned long)tagAndCRC << 8 | (unsigned long)messageStructure;
        canOutFrame.can_id |= CAN_EFF_FLAG;

        //copy message bytes to data array
        int n = 0;
        for(int i = 0; i < message->getArgumentCount(); i++){
            byte* b = message->getArgument(i);
            for(int j = 0; j < message->getArgumentSize(i); j++){
                canOutFrame.data[n++] = b[j];
            }
        }

        //Send the message and handle the response
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
}
