#include "ChetchArduinoBoard.h"
#include <MemoryFree.h>

namespace Chetch{

    
    //Constructors
    ArduinoBoard::ArduinoBoard() : frame(MessageFrame::FrameSchema::SMALL_SIMPLE_CHECKSUM, MAX_FRAME_PAYLOAD_SIZE), 
                                inboundMessage(MAX_FRAME_PAYLOAD_SIZE), 
                                outboundMessage(MAX_FRAME_PAYLOAD_SIZE)
    {
        for(byte i = 0; i < MAX_DEVICES; i++){
            devices[i] = NULL;
        }
    }

    bool ArduinoBoard::begin(Stream* stream){
        this->stream = stream;
        inboundMessage.clear();
        outboundMessage.clear();

        //Note: could set up an outbound message here which will be sent on first loop
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

    ArduinoDevice* ArduinoBoard::getDevice(byte id){
        if(id >= START_DEVICE_IDS_AT && id < deviceCount + START_DEVICE_IDS_AT){
            return devices[id - START_DEVICE_IDS_AT];
        } else {
            return NULL;
        }
    }
    
    //returns true if received a valid message, false otherwise
    bool ArduinoBoard::receiveMessage(){
        while(stream->available()){
            byte b = stream->read();
            if(frame.add(b)){
                if(frame.validate()){
                    //Frame is good so let's get the payload and try turn it into an adm message
                    //note: this clears the inbound message
                    if(inboundMessage.deserialize(frame.getPayload(), frame.getPayloadSize())){
                        return true;
                    } else {
                        //an ArduinoMessage deserialization error
                        setErrorInfo(&outboundMessage, ErrorCode::MESSAGE_ERROR, (byte)inboundMessage.error);
                    }
                } else {
                    //a MessageFrame error
                    setErrorInfo(&outboundMessage, ErrorCode::MESSAGE_FRAME_ERROR, (byte)frame.error);
                } //end frame validate

                //if we are here it must be an error so let the sender know
                setResponseInfo(&outboundMessage, &inboundMessage, BOARD_ID);
            }
        } //end stream read
        return false;
    }

    void ArduinoBoard::sendMessage(){
        //TODO:??? maybe handle case of if oubound message is in an error state
        frame.setPayload(outboundMessage.getBytes(), outboundMessage.getByteCount());
        frame.write(stream); //write bytes to stream
        frame.reset();
        
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
                setResponseInfo(response, message, BOARD_ID);
                break;

            case ArduinoMessage::TYPE_STATUS_REQUEST:
                response->type = ArduinoMessage::TYPE_STATUS_RESPONSE;
                //response->add(BOARD_NAME);
                response->add(millis());
                response->add(deviceCount);
                response->add(freeMemory());
                setResponseInfo(response, message, BOARD_ID);
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
            qi.messageID = 100;
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

    void ArduinoBoard::loop(){
        //1. Receieve any message and possilbly reply (will get sent next loop)
        if(receiveMessage()){
            //we have received a valid message ... so direct it then to the appropriate place for handling
            outboundMessage.clear();
            switch(inboundMessage.target){
                case ArduinoMessage::NO_TARGET:
                    setErrorInfo(&outboundMessage, ErrorCode::TARGET_NOT_SUPPLIED, inboundMessage.target);
                    setResponseInfo(&outboundMessage, &inboundMessage, BOARD_ID);
                    break;

                case ArduinoBoard::BOARD_ID:
                    handleInboundMessage(&inboundMessage, &outboundMessage);
                    break;

                default: //assume this is for device
                    ArduinoDevice* device = getDevice(inboundMessage.target);
                    if(device != NULL){
                        device->handleInboundMessage(&inboundMessage, &outboundMessage);
                        setResponseInfo(&outboundMessage, &inboundMessage, device->id);
                    } else {
                        setErrorInfo(&outboundMessage, ErrorCode::TARGET_NOT_FOUND, inboundMessage.target);
                        setResponseInfo(&outboundMessage, &inboundMessage, BOARD_ID);
                    }
                    break;
            }

            //if the processing of the received message resulted in an outbound message then send it!
            if(!outboundMessage.isEmpty()){
                sendMessage();
            }
        } //end receive massage

        //2. Loop next device
        static byte currentdevice = 0;
        if(devices[currentdevice] != NULL){
            devices[currentdevice]->loop(); //will update device state, possible set flags etc. to then pouplate outbound message
            currentdevice = (currentdevice + 1) % deviceCount;
        }

        //3. Process next message to send in queue
        if(!isMessageQueueEmpty() && outboundMessage.isEmpty()){
            MessageQueueItem qi = dequeueMessageToSend();
            qi.device->populateOutboundMessage(&outboundMessage, qi.messageID);
            outboundMessage.target = qi.device->id;
            outboundMessage.sender = qi.device->id;
            outboundMessage.tag = qi.messageTag;
            sendMessage();
        }
    }
}