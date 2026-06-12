#include "ChetchUtils.h"
#include "ChetchMCP2515Device.h"

namespace Chetch{

    MCP2515Device::MCP2515Device(byte nodeID, int csPin, unsigned int presenceInterval) : 
                                    mcp2515(csPin, 4000000), 
                                    imsg(ARDUINO_MESSAGE_SIZE),
                                    omsg(ARDUINO_MESSAGE_SIZE)
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

    void MCP2515Device::resetErrors(){
#if defined(COUNT_ERROR_CODES)
        for(byte i = 0;i < COUNT_ERROR_CODES; i++){
            errorCounts[i] = 0;
        }
#endif
        lastError = NO_ERROR;
        lastErrorOn = 0;
        lastErrorData = 0;
        errorCodeFlags = 0;
    }

    int MCP2515Device::clearReceive(){
        int n = 0;
        while(mcp2515.checkReceive() && n < 3){
            readMessage();
            n++;
        }

        return n;
    }

    void MCP2515Device::init(bool forceInit){
        if(!initialised || forceInit){
            mcp2515.reset();
            resetErrors();
            initialised = true;
        }
    }

    bool MCP2515Device::begin(){
        if(nodeID < MIN_NODE_ID || nodeID > MAX_NODE_ID){
            return false;
        }

        init();

        mcp2515.setBitrate(CAN_125KBPS);

        //apply targeted filter
        uint32_t mask;
        uint32_t filter;
        MCP2515::ERROR err;

        if(filterPolicy == FilterPolicy::RESTRICT_TO_TARGETED){
            //add filter for 'targeted' message types (bit 5 = 0, bit 4 = 1, bit 3,2,1 = x)
            mask = 0x18000000; //bits 5 and 4 of first byte //MESSAGE_TYPES_MASK; 
            err = mcp2515.setFilterMask(MCP2515::MASK::MASK0, true, mask);
            if(err != MCP2515::ERROR_OK){
                return false;
            }
            //Set first filter to targeted messages
            filter = 0x08000000; //targeted messages BIT 5 = 0, BIT 4 = 1
            err = mcp2515.setFilter(MCP2515::RXF::RXF0, true, filter);
            if(err != MCP2515::ERROR_OK){
                return false;
            }
            //Set second filter zero to avoid "random" data passing a match test
            err = mcp2515.setFilter(MCP2515::RXF::RXF1, true, filter);
            if(err != MCP2515::ERROR_OK){
                return false;
            }
        }

        //apply node dependency filters
        if(firstDependency != NULL && filterPolicy != FilterPolicy::DO_NOT_USE_FILTERS){
            mask = 0x18F00000; //we allow braodcast message types AND node ID (bits 8-5 of second byte) filter combinations
            err = mcp2515.setFilterMask(MCP2515::MASK::MASK1, true, mask);
            if(err != MCP2515::ERROR_OK){
                return false;
            }

            filter = 0x00000000; //targeted messages BIT 5 = 0, BIT 4 = 0
            NodeDependency* nd = firstDependency;
            int i = 2; 
            do{
                uint32_t ndFilter = filter | ((uint32_t)nd->getNodeID() << 20);
                err = mcp2515.setFilter((MCP2515::RXF)i, true, ndFilter);
                if(err != MCP2515::ERROR_OK){
                    return false;
                }
                i++;
                nd = nd->next;
            }while (nd != NULL);

            //check we haven't exceeded filters
            if(i > 5)return false;
            
            while(i <= 5){
                err = mcp2515.setFilter((MCP2515::RXF)i, true, 0xFFFFFFFF);
                if(err != MCP2515::ERROR_OK){
                    return false;
                }
                i++;
            }
        }

#if CAN_AS_LOOPBACK 
        Serial.println("Using loopback");
        mcp2515.setLoopbackMode();
#else
        mcp2515.setNormalMode();
#endif
        if(indicatorPin > 0){
            pinMode(indicatorPin, OUTPUT);
            indicate(false);
        }

        presenceSentCount = 0;
        enqueueMessageToSend(MESSAGE_ID_PRESENCE);

        return ArduinoDevice::begin();;
	}

     void MCP2515Device::indicate(bool on, bool force){
        if(on == indicated || indicatorPin == 0)return; 

        if(on){
            indicatedOn = millis();
            indicated = true;
            digitalWrite(indicatorPin, HIGH);
        } else if(force || millis() - indicatedOn > INDICATOR_INTERVAL){
            indicated = false;
            digitalWrite(indicatorPin, LOW);
        }
    }

    void MCP2515Device::raiseError(MCP2515ErrorCode errorCode, unsigned long errorData, bool canBroadcast){
        bool repeatError = lastError == errorCode;
        bool broadcastError = false;
        if(canBroadcast){
            if(lastError == MCP2515ErrorCode::NO_ERROR){
                broadcastError = true;
            } else if(repeatError){
                broadcastError = millis() - lastErrorOn > 1000;
            } else {
                broadcastError = millis() - lastErrorOn > 250;
            }
        }

        lastError = errorCode;
        lastErrorData = errorData;
        lastErrorOn = millis();
        byte idx = (byte)(errorCode) - 1;
        
#if defined(COUNT_ERROR_CODES)
        if(errorCounts[idx] < 255)errorCounts[idx]++;
#endif
        unsigned int emask = (1 << idx);
        errorCodeFlags = errorCodeFlags | emask;

        if(errorListener != NULL){
            errorListener(this, errorCode, errorData);
        }

        if(broadcastError){
            enqueueMessageToSend(MESSAGE_ID_ERROR);
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

    MCP2515Device::NodeDependency* MCP2515Device::addNodeDependency(byte nodeID, byte tolerance){
        NodeDependency* nd = new NodeDependency(nodeID, tolerance);

        if(firstDependency == NULL){
            firstDependency = nd;
        } else {
            NodeDependency* d = firstDependency;
            while(d != NULL){
                if(d->getNodeID() == nodeID){
                    delete nd;
                    return d;
                }

                if(d->next == NULL){
                    d->next = nd;
                    break;
                }
                d = d->next;
            }
        }
        return nd;
    }

    void MCP2515Device::loop(){
        indicate(false);
        ArduinoDevice::loop();

        unsigned long ms = millis();
        ArduinoMessage* msg;
        
        //We ensure that the first message sent is a presence message and this should be as soon as the device has begun
        if(presenceInterval > 0 && (ms - lastPresenceOn) > presenceInterval){
            enqueueMessageToSend(MESSAGE_ID_PRESENCE);
            lastPresenceOn = millis();
        }
    }

    void MCP2515Device::handleInboundMessage(ArduinoMessage* message, ArduinoMessage* response){
        switch(message->type){
            case ArduinoMessage::TYPE_INITIALISE:
                response->type = ArduinoMessage::TYPE_INITIALISE_RESPONSE;
                response->add(millis());
                response->add((byte)TIMESTAMP_RESOLUTION);
                response->add(presenceInterval);
                break;

            case ArduinoMessage::TYPE_RESET:
                {
                    ResetRegime regime = message->get<ResetRegime>(0);
                    switch(regime){
                        case RESET_ERRORS:
                            resetErrors();
                            break;

                        case RESET_DEVICE:
                            break;

                        default:
                            break;
                    }
                    response->type = ArduinoMessage::TYPE_RESET_RESPONSE;
                    setStatusInfo(response);
                }
                break;

            case ArduinoMessage::TYPE_ERROR_TEST:
                {
                    MCP2515ErrorCode ecode = message->get<MCP2515ErrorCode>(0);
                    unsigned long edata = message->hasArgument(1) ? message->get<unsigned long>(1) : 0;
                    raiseError(ecode, edata);
                }
                break;

            default:
                ArduinoDevice::handleInboundMessage(message, response);
                break;
        }
    }

    void MCP2515Device::populateOutboundMessage(ArduinoMessage* message, byte messageID){
        switch(messageID){
            case MESSAGE_ID_PRESENCE:
                message->type = ArduinoMessage::TYPE_PRESENCE;
                message->add(millis());
                message->add(presenceSentCount); //if 0 then resets presence in remote node (i.e. first presence message)
                break;

            case MESSAGE_ID_ERROR:
                message->type = ArduinoMessage::TYPE_ERROR;
                message->add((byte)lastError);
                message->add(lastErrorData);
                message->add(errorCodeFlags);
                message->add(mcp2515.getErrorFlags());
                break;

            default:
                ArduinoDevice::populateOutboundMessage(message, messageID);
                break;
        }
    }

    void MCP2515Device::onOutboundMessageSent(ArduinoMessage* message, byte messageID){
        switch(messageID){
            case MESSAGE_ID_PRESENCE:
                presenceSentCount++;
                if(presenceSentCount == 0)presenceSentCount = 1; //to avoid firing like first joined
                break;

            case MESSAGE_ID_ERROR:
                //Serial.print("Sent error");
                break;

            default:
                ArduinoDevice::onOutboundMessageSent(message, messageID);
                break;
        }
    }

    void MCP2515Device::setStatusInfo(ArduinoMessage* message){
        message->add(mcp2515.getStatus());
        message->add(mcp2515.getErrorFlags());
        message->add(mcp2515.errorCountTX());
        message->add(mcp2515.errorCountRX());
        message->add(errorCodeFlags);
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

    ArduinoMessage* MCP2515Device::getMessageForHandler(byte handlerID, ArduinoMessage::MessageType messageType, byte tag){
        omsg.clear();
        omsg.type = messageType;
        omsg.tag = tag;
        if(handlerID < ArduinoBoard::START_DEVICE_IDS_AT){
            omsg.sender = 0;
        } else {
            omsg.sender = 1 + (handlerID - ArduinoBoard::START_DEVICE_IDS_AT);
        }

        return &omsg;
    }

    ArduinoMessage* MCP2515Device::getMessageForDevice(ArduinoDevice* device, ArduinoMessage::MessageType messageType, byte tag){
        return getMessageForHandler(device->getID(), messageType, tag);
    }

    ArduinoMessage* MCP2515Device::getMessageForBoard(ArduinoMessage::MessageType messageType, byte tag){
        return getMessageForHandler(Board->getID(), messageType, tag);
    }

    MCP2515Device::MCP2515ErrorCode MCP2515Device::sendMessageForDevice(ArduinoDevice* device, byte messageID){
        ArduinoMessage* msg = getMessageForDevice(device);
        if(device != NULL){
            device->populateOutboundMessage(msg, messageID);
            return sendMessage(msg);
        } else {
            return MCP2515ErrorCode::NO_ERROR;
        }
    }

    MCP2515Device::MCP2515ErrorCode MCP2515Device::sendMessageForBoard(byte messageID){
        ArduinoMessage* msg = getMessageForBoard();
        if(Board != NULL){
            Board->populateOutboundMessage(msg, messageID);
            return sendMessage(msg);
        } else {
            return MCP2515ErrorCode::NO_ERROR;
        }
    }

    bool MCP2515Device::checkReceive(){
        return mcp2515.checkReceive();
    }

    ArduinoMessage* MCP2515Device::readMessage(){
        MCP2515::ERROR err = mcp2515.readMessage(&canInFrame);
        switch(err){
            case MCP2515::ERROR_OK:
            {
                //Clear message and split out the ID
                imsg.clear();
                byte messageType = (canInFrame.can_id >> 24) & 0x1F; //first 5 bits
                byte nodeIDAndSender = (canInFrame.can_id >> 16) & 0xFF; //whole byte split 4 | 4
                byte tagAndCRC = (canInFrame.can_id >> 8) & 0xFF; //whole byte split 3 | 5
                byte sourceNodeID = nodeIDAndSender >> 4 & 0x0F; //first 4 bits are used for the remote node ID
                byte timestamp = canInFrame.can_id & 0xFF;
                //Serial.print("Received timestamp: ");
                //Serial.println(timestamp);

                /*Serial.print("Received CANID: ");
                for (int i = 32; i >= 0; i--) {
                    Serial.print(bitRead(canInFrame.can_id, i));
                    if(i % 8 == 0 && i != 32 && i > 0)Serial.print("-");
                }
                Serial.println();*/
                

                unsigned long edata = (unsigned long)sourceNodeID << 24 | (unsigned long)messageType << 16;
                if(!vcrc5(tagAndCRC & 0x1F, canInFrame.data, canInFrame.can_dlc)){
                    //data error
                    raiseError(CRC_ERROR, edata | (tagAndCRC & 0x1F));
                    return NULL;
                }

                imsg.data = sourceNodeID; //possibly useful as this is data in CAN ID that is not included in Arduino message
                imsg.type = messageType;
                imsg.tag = (tagAndCRC >> 5) & 0x07;
                imsg.sender = (nodeIDAndSender & 0x0F);
                if(imsg.sender > 0){
                    imsg.sender = (imsg.sender - 1)+ ArduinoBoard::START_DEVICE_IDS_AT; //last 4 bits make the sender
                } else {
                    imsg.sender = Board->getID();
                }
                
                //Do some basic validationg
                if(sourceNodeID < MIN_NODE_ID || sourceNodeID > MAX_NODE_ID){
                    raiseError(INVALID_MESSAGE, edata | 1);
                    return NULL; //ERROR....
                }

                if(imsg.type < 1 || imsg.type > 31){
                    raiseError(INVALID_MESSAGE, edata | 2);
                    return NULL; //ERROR....
                }

                if(imsg.tag > 7){
                    raiseError(INVALID_MESSAGE, edata | 3);
                    return NULL; //ERROR....
                }

                //handle node dependency
                NodeDependency* nd = getDependency(sourceNodeID);
                if(nd != NULL){
                    if(imsg.type == ArduinoMessage::MessageType::TYPE_PRESENCE){
                        imsg.populate<unsigned long, unsigned int>(canInFrame.data);
                        if(imsg.get<unsigned int>(1) == 0){ //reset node dependency (incase remote node restarted NOTE)
                            nd->reset();
                        }
                        nd->setNodeTime(imsg.get<unsigned long>(0)); //, imsg.get<unsigned int>(1));
                    } else { 
                        //If the node dependency is yet to be updated then we don't go any further
                        if(!nd->isUpdated()){
                            return NULL;
                        }
                        
                        //check for stame messages
                        if(nd->isStale(timestamp)){
                            raiseError(STALE_MESSAGE, edata | nd->getDiff(timestamp));
                            return NULL;
                        }
                    }
                }
                
                //By here we have received and successfully parsed an ArduinoMessage
                if(canIndicate(INDICATE_ON_RECEIVE)){
                    indicate(true); 
                }

                bool parsed = parseReceivedMessage(sourceNodeID, &imsg);
                onMessageReceived(sourceNodeID, &imsg);
                return parsed ? &imsg : NULL;
            }

            case MCP2515::ERROR_FAIL:
                raiseError(READ_FAIL);
                return NULL;

            case MCP2515::ERROR_NOMSG: //ignore
                return NULL;

            default:
                //Serial.println("Received something weird");
                raiseError(UNKNOWN_RECEIVE_ERROR);
                return NULL;
        }
    }


    bool MCP2515Device::parseReceivedMessage(byte sourceNodeID, ArduinoMessage* message){
        bool parsed = false;

        switch(message->type){
            case ArduinoMessage::TYPE_STATUS_REQUEST:
            case ArduinoMessage::TYPE_PING:
            case ArduinoMessage::TYPE_INITIALISE:
                if(canInFrame.can_dlc == 0){ //error code + node
                    raiseError(INVALID_MESSAGE, 10);
                    parsed = false;
                } else {
                    message->populate<byte>(canInFrame.data);
                    parsed = true;
                }
                break;

            case ArduinoMessage::TYPE_ERROR_TEST:
                if(canInFrame.can_dlc == 2){ //error code + node
                    message->populate<byte, byte>(canInFrame.data);
                    parsed = true;
                } else if(canInFrame.can_dlc == 6){ //erro code + error data + node
                    message->populate<byte, unsigned long, byte>(canInFrame.data);
                    parsed = true;
                } else {
                    raiseError(INVALID_MESSAGE, 11);
                    parsed = false;
                }
                break;

            case ArduinoMessage::TYPE_RESET:
                if(canInFrame.can_dlc != 2){
                    raiseError(INVALID_MESSAGE, 12);
                    parsed = false;
                } else {
                    message->populate<byte, byte>(canInFrame.data); //reset regime + node id
                    parsed = true;
                }
                break;

            case ArduinoMessage::TYPE_COMMAND:
                if(canInFrame.can_dlc == 2){ //command + node
                    message->populate<byte, byte>(canInFrame.data);
                    parsed = true;
                } else if(canInFrame.can_dlc == 3){ //command + byte arg + node
                    message->populate<byte, byte, byte>(canInFrame.data);
                    parsed = true;
                } else if(canInFrame.can_dlc == 4){ //command + int arg + node
                    message->populate<byte, int, byte>(canInFrame.data);
                    parsed = true;
                } else {
                    raiseError(INVALID_MESSAGE, 13);
                    parsed = false;
                }
                break;

            case ArduinoMessage::TYPE_PRESENCE:
                message->populate<unsigned long, unsigned int>(canInFrame.data);
                parsed = true;
                break;

            default: 
                parsed = false;
                break;
        }

        return parsed;
    }
    

    void MCP2515Device::onMessageReceived(byte sourceNodeID, ArduinoMessage *message){
        if(messageReceivedListener != NULL){
            messageReceivedListener(this, sourceNodeID, message, canInFrame.can_id, canInFrame.data, canInFrame.can_dlc);
        }
    }

    MCP2515Device::MCP2515ErrorCode MCP2515Device::sendMessage(ArduinoMessage* message, bool broadcastSendFailedError){
        if(message == NULL){
            raiseError(MCP2515ErrorCode::NO_MESSAGE);
            return MCP2515ErrorCode::NO_MESSAGE; //ERROR!
        }
        if(message->type > 31 || message->type < 1){
            raiseError(MCP2515ErrorCode::INVALID_MESSAGE, 4);
            return MCP2515ErrorCode::INVALID_MESSAGE; //ERROR .... not a valid message type
        }
        
        if(message->tag > 7){
            raiseError(MCP2515ErrorCode::INVALID_MESSAGE, 5);
            return  MCP2515ErrorCode::INVALID_MESSAGE; //ERROR .... tag values can only be 0 - 7
        }

        if(message->sender > 15){
            raiseError(MCP2515ErrorCode::INVALID_MESSAGE, 6);
            return  MCP2515ErrorCode::INVALID_MESSAGE; //ERROR .... sender only has 4 bits available
        }
        
        
        canOutFrame.can_dlc = message->getByteCount() - ArduinoMessage::HEADER_SIZE - message->getArgumentCount();
        if(canOutFrame.can_dlc > CAN_MAX_DLC){
            raiseError(MCP2515ErrorCode::INVALID_MESSAGE, 7);
            return MCP2515ErrorCode::INVALID_MESSAGE; //ERROR ... can data of 8 bytes sets this limit
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
            raiseError(INVALID_MESSAGE, 8);
            return  MCP2515ErrorCode::INVALID_MESSAGE;
        }
        
        //CAN ID
        byte messageType = message->type & 0x1F;
        byte nodeIDAndSender = (nodeID << 4 ) | (message->sender & 0x0F);
        byte tagAndCRC = (message->tag << 5) | crc5(canOutFrame.data, canOutFrame.can_dlc);
        unsigned long ms = millis();
        byte timestamp = (byte)((ms >> TIMESTAMP_RESOLUTION) & 0xFF);
        canOutFrame.can_id = (unsigned long)messageType << 24 | (unsigned long)nodeIDAndSender << 16 | (unsigned long)tagAndCRC << 8 | (unsigned long)timestamp;
        canOutFrame.can_id |= CAN_EFF_FLAG;
        
        if(sendValidator != NULL && !sendValidator(this, &omsg, canOutFrame.can_id, canOutFrame.data, canOutFrame.can_dlc)){
            return MCP2515ErrorCode::CUSTOM_ERROR; //maybe change to send cancelled error code
        }

        //Send the message and handle the response
        MCP2515::ERROR err = mcp2515.sendMessage(&canOutFrame);
        unsigned long edata = (unsigned long)nodeIDAndSender << 24 | (unsigned long)messageType << 16;
        switch(err){
            case MCP2515::ERROR_OK:
                if(canIndicate(INDICATE_ON_SEND)){
                    indicate(true);
                }
                onMessageSent(message);
                return MCP2515ErrorCode::NO_ERROR;

            case MCP2515::ERROR_FAILTX:
                raiseError(MCP2515ErrorCode::FAIL_TX, edata, broadcastSendFailedError);
                return MCP2515ErrorCode::FAIL_TX;

            case MCP2515::ERROR_ALLTXBUSY:
                raiseError(MCP2515ErrorCode::ALL_TX_BUSY, edata, broadcastSendFailedError);
                return MCP2515ErrorCode::ALL_TX_BUSY;

            default:
                raiseError(MCP2515ErrorCode::UNKNOWN_SEND_ERROR, edata);
                return MCP2515ErrorCode::UNKNOWN_SEND_ERROR;
        }
    }

    void MCP2515Device::onMessageSent(ArduinoMessage *message){
        if(messageSentListener != NULL){
            messageSentListener(this, getNodeID(), message, canOutFrame.can_id, canOutFrame.data, canOutFrame.can_dlc);
        }
    }
} //end namespace
