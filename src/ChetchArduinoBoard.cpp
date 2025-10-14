#include "ChetchArduinoBoard.h"
#include <MemoryFree.h>

namespace Chetch{
    
    //Constructors
    ArduinoBoard::ArduinoBoard() 
#if ARDUINO_BOARD_USE_STREAM            
    : frame(MessageFrame::FrameSchema::SMALL_SIMPLE_CHECKSUM, MessageFrame::MessageEncoding::SYSTEM_DEFINED, MAX_FRAME_PAYLOAD_SIZE), 
                                inboundMessage(MAX_FRAME_PAYLOAD_SIZE), 
                                outboundMessage(MAX_FRAME_PAYLOAD_SIZE)
#endif
    {
        for(byte i = 0; i < MAX_DEVICES; i++){
            devices[i] = NULL;
        }
    }


    bool ArduinoBoard::begin(Stream* stream){
#if ARDUINO_BOARD_USE_STREAM            
        this->stream = stream;
        inboundMessage.clear();
        outboundMessage.clear();
#endif
        for(int i = 0; i < deviceCount; i++){
            if(!devices[i]->begin() || !devices[i]->hasBegun()){ //in case we forget to set the begun flag
                return false;
            }
        }

        begun = true;
        return begun;
    }

    void ArduinoBoard::addDevice(ArduinoDevice* device){
        byte i = deviceCount;
        if(i < MAX_DEVICES){
            devices[i] = device;
            device->id = i + START_DEVICE_IDS_AT;
            device->Board = this;
            deviceCount++;
        }
    }

    ArduinoDevice* ArduinoBoard::getDeviceByID(byte id){
        if(id >= START_DEVICE_IDS_AT && id < deviceCount + START_DEVICE_IDS_AT){
            return devices[id - START_DEVICE_IDS_AT];
        } else {
            return NULL;
        }
    }

    ArduinoDevice* ArduinoBoard::getDeviceAt(byte idx){
        if(idx >= 0 && idx < deviceCount){
            return devices[idx];
        } else {
            return NULL;
        }
    }

    int ArduinoBoard::getFreeMemory(){
        return freeMemory();
    }
    
#if ARDUINO_BOARD_USE_STREAM            
    //returns true if received a valid message, false otherwise
    bool ArduinoBoard::receiveMessage(){
        if(stream == NULL)return false;

        while(stream->available()){
            byte b = stream->read();
            if(frame.add(b)){
                if(frame.validate()){
                    //Frame is good so let's get the payload and try turn it into an adm message
                    //note: this clears the inbound message
                    if(inboundMessage.deserialize(frame.getPayload(), frame.getPayloadSize())){
                        return true; //BINGO! received a valid message
                    } else {
                        //an ArduinoMessage deserialization error
                        setErrorInfo(&outboundMessage, ErrorCode::MESSAGE_ERROR, (byte)inboundMessage.error);
                        return false;
                    }
                } else {
                    //a MessageFrame error
                    setErrorInfo(&outboundMessage, ErrorCode::MESSAGE_FRAME_ERROR, (byte)frame.error);
                    return false;
                } //end frame validate

                //if we are here it must be an error so let the sender know
                setResponseInfo(&outboundMessage, &inboundMessage, getID());
                return false;
            }
        } //end stream read

        //Not received a message OR stream was empty
        return false;
    }

    void ArduinoBoard::sendMessage(){
        if(stream != NULL){
            //TODO:??? maybe handle case of if oubound message is in an error state
            frame.setPayload(outboundMessage.getBytes(), outboundMessage.getByteCount());
            frame.write(stream); //write bytes to stream
            frame.reset();
        }
        //ready for reuse
        outboundMessage.clear();
        
    }

    void ArduinoBoard::setErrorInfo(ArduinoMessage* message, ErrorCode errorCode, byte errorSubCode){
        message->type = ArduinoMessage::TYPE_ERROR;
        message->add((byte)errorCode);
        message->add(errorSubCode);
    }

    void ArduinoBoard::setResponseInfo(ArduinoMessage* response, ArduinoMessage* message, byte sender){
        response->tag = message->tag;
        response->target = message->sender;
        response->sender = sender;
    }

    void ArduinoBoard::handleInboundMessage(ArduinoMessage* message, ArduinoMessage* response){
        switch(message->type){
            case ArduinoMessage::TYPE_ECHO:
                response->copy(message);
                response->type = ArduinoMessage::TYPE_ECHO_RESPONSE;
                setResponseInfo(response, message, getID());
                break;

            case ArduinoMessage::TYPE_STATUS_REQUEST:
                response->type = ArduinoMessage::TYPE_STATUS_RESPONSE;
                response->add(BOARD_NAME);
                response->add(millis());
                response->add(deviceCount);
                response->add(getFreeMemory());
                setResponseInfo(response, message, getID());
                break;

            case ArduinoMessage::TYPE_PING:
                response->type = ArduinoMessage::TYPE_PING_RESPONSE;
                //response->add(BOARD_NAME);
                response->add(millis());
                setResponseInfo(response, message, getID());
                break;
        }
    }

    bool ArduinoBoard::isMessageQueueFull(){
        return queueCount >= MAX_QUEUE_SIZE;
    }

    bool ArduinoBoard::isMessageQueueEmpty(){
        return queueCount == 0;
    }

    bool ArduinoBoard::enqueueMessageToSend(ArduinoDevice* device, byte messageID, byte messageTag){
        if(isMessageQueueFull()){
            return false;
        } else {
            int idx = (queueStart + queueCount) % MAX_QUEUE_SIZE;
            messageQueue[idx].device = device;
            messageQueue[idx].messageID = messageID;
            messageQueue[idx].messageTag = messageTag;
            queueCount++;
            return true;
        }
    }

    ArduinoBoard::MessageQueueItem ArduinoBoard::dequeueMessageToSend(){
        MessageQueueItem qi;
        if(isMessageQueueEmpty()){
            qi.device = NULL;
            qi.messageID = 0;
            qi.messageTag = 0;
        } else {
            qi.device = messageQueue[queueStart].device;
            qi.messageID = messageQueue[queueStart].messageID;
            qi.messageTag = messageQueue[queueStart].messageTag;
            queueCount--;
            if(!isMessageQueueEmpty()){
                queueStart = (queueStart + 1) % MAX_QUEUE_SIZE;
            }
        }
        return qi;
    }
#endif

    void ArduinoBoard::loop(){
        //basic error checking here to make sure that we've begun
        if(!begun)return;

#if ARDUINO_BOARD_USE_STREAM
        //1. Receieve any message and possilbly reply (will get sent next loop)
        if(receiveMessage()){
            //we have received a VALID message ... so direct it then to the appropriate place for handling
            outboundMessage.clear();
            switch(inboundMessage.target){
                case ArduinoMessage::NO_TARGET:
                    setErrorInfo(&outboundMessage, ErrorCode::TARGET_NOT_SUPPLIED, inboundMessage.target);
                    setResponseInfo(&outboundMessage, &inboundMessage, getID());
                    break;

                default: //assume this is for device (but check it's within device range first)
                    if(inboundMessage.target == getID()){ //intending this board
                        handleInboundMessage(&inboundMessage, &outboundMessage);
                    } else if(inboundMessage.target < START_DEVICE_IDS_AT){ //possible wrong board targeted
                        setErrorInfo(&outboundMessage, ErrorCode::TARGET_NOT_VALID, inboundMessage.target);
                        setResponseInfo(&outboundMessage, &inboundMessage, getID());
                    } else { //intending a device
                        ArduinoDevice* device = getDeviceByID(inboundMessage.target);
                        if(device != NULL){
                            device->handleInboundMessage(&inboundMessage, &outboundMessage);
                            setResponseInfo(&outboundMessage, &inboundMessage, device->id);
                        } else {
                            setErrorInfo(&outboundMessage, ErrorCode::TARGET_NOT_FOUND, inboundMessage.target);
                            setResponseInfo(&outboundMessage, &inboundMessage, getID());
                        }
                    }
                    break;
            }

            //if the processing of the received message resulted in an outbound message then send it!
            if(!outboundMessage.isEmpty()){
                sendMessage();
            }
        } else if(!outboundMessage.isEmpty()){ //if the receiveMessage populated an outbound message (e.g. error)
            sendMessage();
        }
#endif
        //2. Loop next device
        if(deviceCount > 0 && devices[currentdevice] != NULL){
            devices[currentdevice]->loop(); //will update device state, possible set flags etc. to then pouplate outbound message
            currentdevice = (currentdevice + 1) % deviceCount;
        }

#if ARDUINO_BOARD_USE_STREAM
        //3. Process next message to send in queue
        if(!isMessageQueueEmpty() && outboundMessage.isEmpty()){
            MessageQueueItem qi = dequeueMessageToSend();
            qi.device->populateOutboundMessage(&outboundMessage, qi.messageID);
            outboundMessage.target = qi.device->id;
            outboundMessage.sender = qi.device->id;
            outboundMessage.tag = qi.messageTag;
            sendMessage();
        }
#endif
    }

    ArduinoBoard Board;
}