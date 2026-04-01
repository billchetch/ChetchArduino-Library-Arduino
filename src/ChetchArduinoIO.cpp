#include "ChetchArduinoIO.h"
#include "ChetchArduinoBoard.h"

namespace Chetch{

    //Constructor
    ArduinoIO::ArduinoIO(Stream* stream) : MessageIO(),
                                        frame(MessageFrame::FrameSchema::SMALL_SIMPLE_CHECKSUM, MessageFrame::MessageEncoding::SYSTEM_DEFINED, MAX_FRAME_PAYLOAD_SIZE), 
                                                                    inboundMessage(MAX_FRAME_PAYLOAD_SIZE), 
                                                                    outboundMessage(MAX_FRAME_PAYLOAD_SIZE)
    {
        //empty
    }

    void ArduinoIO::begin(Stream* stream){
        this->stream = stream;
        
        inboundMessage.clear();
        outboundMessage.clear();
    }

    void ArduinoIO::loop(){
        //1. Check queue for any messages, send if there is one
        if(!isMessageQueueEmpty() && outboundMessage.isEmpty()){
            MessageQueueItem qi = dequeueMessageToSend();
            qi.device->populateOutboundMessage(&outboundMessage, qi.messageID);
            outboundMessage.target = qi.device->id;
            outboundMessage.sender = qi.device->id;
            outboundMessage.tag = qi.messageTag;
            sendMessage();
        }

        //2. Check if message has been received, process/respond if there is one
        if(receiveMessage()){
            //for convenicence and clarity use vars for pointers
            ArduinoBoard* Board = (ArduinoBoard*)owner;
            ArduinoMessage *message = &inboundMessage;
            ArduinoMessage* response = &outboundMessage;
            response->clear(); //clear response as it may be used

            byte boardID = Board->getID();
            //route message to board and devices
            if(message->target == ArduinoMessage::NO_TARGET){ //Failed to provide a target
                setErrorInfo(response, IOErrorCode::TARGET_NOT_SUPPLIED, message->target);
                setResponseInfo(response, message, boardID);
            } else if(message->target != boardID && message->target < ArduinoBoard::START_DEVICE_IDS_AT){ //possible wrong board targeted
                setErrorInfo(response, IOErrorCode::TARGET_NOT_VALID, message->target);
                setResponseInfo(response, message, boardID);
            } else if(message->target == boardID){ //intending this board
                if(message->type == ArduinoMessage::TYPE_ECHO){
                    response->copy(message);
                    response->type = ArduinoMessage::TYPE_ECHO_RESPONSE;
                    setResponseInfo(response, message, boardID);
                } else if(message->type == ArduinoMessage::TYPE_STATUS_REQUEST){
                    //Anticipate message has server info
                    if(message->getArgumentCount() >= 2){
                        //Set unix time stamp and timezone
                        Board->setTime(message->get<unsigned long>(0), message->get<int>(1));
                    }
                    //Now formulate a response to the request
                    response->type = ArduinoMessage::TYPE_STATUS_RESPONSE;
                    response->add(BOARD_NAME);
                    response->add(millis());
                    response->add(Board->getDeviceCount());
                    response->add(Board->getFreeMemory());
                    setResponseInfo(response, message, boardID);
                } else if(message->type == ArduinoMessage::TYPE_PING){
                    response->type = ArduinoMessage::TYPE_PING_RESPONSE;
                    response->add(millis());
                    setResponseInfo(response, message, boardID);
                }
            } else { //intending a device
                ArduinoDevice* device = Board->getDeviceByID(message->target);
                if(device != NULL){
                    device->handleInboundMessage(message, response);
                    setResponseInfo(response, message, device->id);
                } else {
                    setErrorInfo(response, IOErrorCode::TARGET_NOT_FOUND, message->target);
                    setResponseInfo(response, message, boardID);
                }
            }

            //if there is some kind of response then send it
            if(!response->isEmpty()){
                sendMessage();
            }
        } //end received message
    }

    bool ArduinoIO::receiveMessage(){
        if(stream == NULL)return false;

        while(stream->available()){
            byte b = stream->read();
            Serial.print("Byte=");
            Serial.println(b);
            if(frame.add(b)){
                if(frame.validate()){
                    //Frame is good so let's get the payload and try turn it into an adm message
                    //note: this clears the inbound message
                    if(inboundMessage.deserialize(frame.getPayload(), frame.getPayloadSize())){
                        frame.reset();
                        return true; //BINGO! received a valid message
                    } else {
                        //an ArduinoMessage deserialization error
                        setErrorInfo(&outboundMessage, IOErrorCode::MESSAGE_ERROR, (byte)inboundMessage.error);
                        frame.reset();
                        return false;
                    }
                } else {
                    //a MessageFrame error
                    setErrorInfo(&outboundMessage, IOErrorCode::MESSAGE_FRAME_ERROR, (byte)frame.error);
                    frame.reset();
                    return false;
                } //end frame validate
            }
        } //end stream read

        //Not received a message OR stream was empty
        return false;
    }

    void ArduinoIO::sendMessage(){
        if(stream != NULL){
            //TODO:??? maybe handle case of if oubound message is in an error state
            frame.setPayload(outboundMessage.getBytes(), outboundMessage.getByteCount());
            frame.write(stream); //write bytes to stream
            frame.reset();
        }
        //ready for reuse
        outboundMessage.clear();
    }

    void ArduinoIO::setErrorInfo(ArduinoMessage* message, byte errorCode, byte errorSubCode){
        if(message != NULL){
            message->type = ArduinoMessage::TYPE_ERROR;
            message->add((byte)errorCode);
            message->add(errorSubCode);
        }
    }

    void ArduinoIO::setErrorInfo(ArduinoMessage* message, IOErrorCode errorCode, byte errorSubCode){
        setErrorInfo(message, (byte)errorCode, errorSubCode);
    }

    void ArduinoIO::setResponseInfo(ArduinoMessage* response, ArduinoMessage* message, byte sender){
        if(response != NULL){
            response->tag = message->tag;
            response->target = message->sender;
            response->sender = sender;
        }
    }

    bool ArduinoIO::isMessageQueueFull(){
        return queueCount >= MAX_QUEUE_SIZE;
    }

    bool ArduinoIO::isMessageQueueEmpty(){
        return queueCount == 0;
    }

    bool ArduinoIO::enqueueMessageToSend(void* device, byte messageID, byte messageTag){
        if(isMessageQueueFull()){
            return false;
        } else {
            int idx = (queueStart + queueCount) % MAX_QUEUE_SIZE;
            messageQueue[idx].device = (ArduinoDevice*)device;
            messageQueue[idx].messageID = messageID;
            messageQueue[idx].messageTag = messageTag;
            queueCount++;
            return true;
        }
    }

    ArduinoIO::MessageQueueItem ArduinoIO::dequeueMessageToSend(){
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

}//end namespace