#include "ChetchUtils.h"
#include "ChetchMCP2515Device.h"


namespace Chetch{
    MCP2515Device::MCP2515Device(byte nodeID, unsigned int presenceInterval, int csPin) : 
                                    mcp2515(csPin, 4000000), 
                                    amsg(ARDUINO_MESSAGE_SIZE)
    {
        this->nodeID = nodeID;
        this->presenceInterval = presenceInterval;
    }

    MCP2515Device::~MCP2515Device(){
        NodeDependency* d = firstDependency;
        while(d != NULL){
            NodeDependency* prev = d;
            d = d->next;
            delete prev;
        }
    }

    void MCP2515Device::reset(){
        maskNum = 0;
        filterNum = 0;
        mcp2515.reset();
    }

    void MCP2515Device::init(){
        if(!initialised){
            reset();

            for(byte i = 0;i < 11; i++){
                errorCounts[i] = 0;
            }
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

        presenceSent = false;

        begun = true;
        return begun;
	}

    void MCP2515Device::indicate(bool on){
        if(indicatorPin > 0)digitalWrite(indicatorPin, on ? HIGH : LOW);
    }

    void MCP2515Device::raiseError(MCP2515ErrorCode errorCode, unsigned long errorData){
        lastError = errorCode;
        lastErrorData = errorData;

        lastErrorOn = millis();
        byte idx = (byte)(errorCode) - 1;
        if(errorCounts[idx] < 255)errorCounts[idx]++;

        if(errorListener != NULL){
            errorListener(this, errorCode);
        }
    }

    MCP2515Device::NodeDependency* MCP2515Device::getDependency(byte nodeID){
        NodeDependency* d = firstDependency;
        while(d != NULL){
            if(d->getNodeID() == nodeID){
                return d;
            }
            d = d->next;
        }
        return NULL;
    }

    bool MCP2515Device::addNodeDependency(byte nodeID){
        NodeDependency* nd = new NodeDependency(nodeID);

        if(firstDependency == NULL){
            firstDependency = nd;
        } else {
            NodeDependency* d = firstDependency;
            while(d != NULL){
                if(d->getNodeID() == nodeID){
                    delete nd;
                    return false;
                }

                if(d->next == NULL){
                    d->next = nd;
                    break;
                }
                d = d->next;
            }
        }
        return true;
    }

    void MCP2515Device::loop(){
        indicate(false);
        ArduinoDevice::loop();

        readMessage();    
    
        if(presenceInterval > 0 && millis() - lastPresenceOn > presenceInterval){
            ArduinoMessage* msg = getMessageForDevice(this, ArduinoMessage::MessageType::TYPE_PRESENCE, 1);
            msg->add(millis());
            msg->add(!presenceSent); //reset presence in remote node
            msg->add(mcp2515.getStatus());

            if(!canSend){
                canSend = true;
                sendMessage(msg);
                canSend = false;
                allowSending();
            } else {
                sendMessage(msg);
            }

            lastPresenceOn = millis();
            presenceSent = true;
        }

        if(lastError != MCP2515ErrorCode::NO_ERROR){
            //send error message
            ArduinoMessage* msg = getMessageForDevice(this, ArduinoMessage::MessageType::TYPE_ERROR, 1);
            msg->add((byte)lastError);
            msg->add(lastErrorData);
            msg->add(mcp2515.getErrorFlags());
            msg->add(mcp2515.getStatus());

            //TODO: restore this!!!
            //sendMessage(msg);
            
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
                byte timestamp = canInFrame.can_id & 0xFF;
                //Serial.print("Received timestamp: ");
                //Serial.println(timestamp);
                

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

                if(amsg.type != ArduinoMessage::MessageType::TYPE_PRESENCE){
                    NodeDependency* nd = getDependency(sourceNodeID);
                    if(nd != NULL && nd->isStale(timestamp)){
                        raiseError(STALE_MESSAGE, canInFrame.can_id);
                        return;
                    }
                }
                
                //By here we have received and successfully parsed an ArduinoMessage
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
                if(commandListener != NULL && canInFrame.can_dlc > 0){
                    if(canInFrame.can_dlc == 1){
                        message->populate<byte>(canInFrame.data);
                    } else {
                        message->populate<byte, byte>(canInFrame.data);
                    }
                    command = message->get<ArduinoDevice::DeviceCommand>(0);
                    commandListener(this, sourceNodeID, command, message);
                    handled = true;
                } else {
                    handled = false;
                }
                break;

            case ArduinoMessage::TYPE_PRESENCE:
                message->populate<unsigned long, bool, byte>(canInFrame.data);
                NodeDependency* nd = getDependency(sourceNodeID);
                if(nd != NULL){
                    if(message->get<bool>(1)){ //reset node dependency (incase remote node restarted)
                        nd->reset();
                    }

                    if(!nd->setNodeTime(message->get<unsigned long>(0))){
                        raiseError(SYNC_ERROR, message->get<unsigned long>(0));
                        handled = true;
                    } else {
                        //TODO: process other message values
                    }                    
                }
                break;

            default:
                handled = false;
                break;
        }

        if(!handled && messageReceivedListener != NULL){
            messageReceivedListener(this, sourceNodeID, message, canInFrame.data);
        }
    }

    bool MCP2515Device::sendMessage(ArduinoMessage* message){
        if(!canSend)return false;

        if(message == NULL){
            raiseError(MCP2515ErrorCode::NO_MESSAGE);
            return false; //ERROR!
        }
        if(message->type > 31 || message->type < 1){
            raiseError(MCP2515ErrorCode::INVALID_MESSAGE);
            return false; //ERROR .... not a valid message type
        }
        
        if(message->tag > 7){
            raiseError(MCP2515ErrorCode::INVALID_MESSAGE);
            return false; //ERROR .... tag values can only be 0 - 7
        }

        if(message->sender > 15){
            raiseError(MCP2515ErrorCode::INVALID_MESSAGE);
            return false; //ERROR .... sender only has 4 bits available
        }
        
        
        canOutFrame.can_dlc = message->getByteCount() - ArduinoMessage::HEADER_SIZE - message->getArgumentCount();
        if(canOutFrame.can_dlc > CAN_MAX_DLC){
            raiseError(MCP2515ErrorCode::INVALID_MESSAGE);
            return false; //ERROR ... can data of 8 bytes sets this limit
        }

        //CAN DATA: copy message bytes to data array
        int n = 0;
        for(int i = 0; i < message->getArgumentCount(); i++){
            byte* b = message->getArgument(i);
            for(int j = 0; j < message->getArgumentSize(i); j++){
                canOutFrame.data[n++] = b[j];
            }
        }
        if(canOutFrame.can_dlc != n){
            raiseError(INVALID_MESSAGE);
            return false;
        }
        
        //CAN ID
        byte messageType = message->type & 0x1F;
        byte nodeIDAndSender = (nodeID << 4 ) | (message->sender & 0x0F);
        byte tagAndCRC = (message->tag << 5) | crc5(canOutFrame.data, canOutFrame.can_dlc);
        unsigned long ms = millis();
        byte timestamp = (byte)((ms >> TIMESTAMP_RESOLUTION) & 0xFF);
        //Serial.print("Send millis: ");
        //Serial.println(ms);
        //Serial.print("Send timestamp: ");
        //Serial.println(timestamp);
        canOutFrame.can_id = (unsigned long)messageType << 24 | (unsigned long)nodeIDAndSender << 16 | (unsigned long)tagAndCRC << 8 | (unsigned long)timestamp;
        canOutFrame.can_id |= CAN_EFF_FLAG;

        if(sendValidator != NULL && !sendValidator(this, &amsg, canOutFrame.can_id, canOutFrame.data)){
            return false;
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
