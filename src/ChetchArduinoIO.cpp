#include "ChetchArduinoIO.h"
#include "ChetchArduinoBoard.h"

namespace Chetch{

    //Constructor
    ArduinoIO::ArduinoIO(Stream* stream) : MessageIO(),
                                        inFrame(MessageFrame::FrameSchema::SMALL_SIMPLE_CHECKSUM, MessageFrame::MessageEncoding::SYSTEM_DEFINED, MAX_FRAME_PAYLOAD_SIZE), 
                                        outFrame(MessageFrame::FrameSchema::SMALL_SIMPLE_CHECKSUM, MessageFrame::MessageEncoding::SYSTEM_DEFINED, MAX_FRAME_PAYLOAD_SIZE), 
                                                                    inboundMessage(MAX_FRAME_PAYLOAD_SIZE), 
                                                                    outboundMessage(MAX_FRAME_PAYLOAD_SIZE)
    {
        //empty
    }

    void ArduinoIO::begin(Stream* stream, byte framePadding){
        this->stream = stream;
        this->framePadding = framePadding;
        
        inboundMessage.clear();
        outboundMessage.clear();
    }

    void ArduinoIO::loop(){
        //1. If after receiving a message the outbound was pouplated for sending then go ahead and send it
        if(!outboundMessage.isEmpty()){
            sendMessage();
            return;
        }


        //2. If we are here then we know there is nothing in the outbound message so check to see if there is something in the queu and send if so
        if(!isMessageQueueEmpty()){
            MessageQueueItem qi = dequeueMessageToSend();
            outboundMessage.target = qi.handler->getID();
            outboundMessage.sender = outboundMessage.target;
            outboundMessage.tag = qi.messageTag;
            qi.handler->populateOutboundMessage(&outboundMessage, qi.messageID);
            if(sendMessage()){
                qi.handler->onOutboundMessageSent(&outboundMessage, qi.messageID);
            }
        }

        //3. Check if message has been received, process it and respond which will populate outbound to be sent next loop
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
                Board->handleInboundMessage(message, response);
                setResponseInfo(response, message, boardID);
            } else { //intending a device
                ArduinoDevice* device = Board->getDeviceByID(message->target);
                if(device != NULL){
                    device->handleInboundMessage(message, response);
                    setResponseInfo(response, message, device->getID());
                } else {
                    setErrorInfo(response, IOErrorCode::TARGET_NOT_FOUND, message->target);
                    setResponseInfo(response, message, boardID);
                }
            }
        } //end received message
    }

    bool ArduinoIO::receiveMessage(){
        if(stream == NULL)return false;

        while(stream->available()){
            byte b = stream->read();
            
            if(inFrame.add(b)){
                if(inFrame.validate()){
                    //Frame is good so let's get the payload and try turn it into an adm message
                    //note: this clears the inbound message
                    if(inboundMessage.deserialize(inFrame.getPayload(), inFrame.getPayloadSize())){
                        inFrame.reset();
                        return true; //BINGO! received a valid message
                    } else {
                        //an ArduinoMessage deserialization error
                        setErrorInfo(&outboundMessage, IOErrorCode::MESSAGE_ERROR, (byte)inboundMessage.error);
                        outboundMessage.target = ((ArduinoBoard*)owner)->getID();
                        outboundMessage.sender = outboundMessage.target;
                        outboundMessage.tag = (byte)inboundMessage.error;
                        inFrame.reset();
                        return false;
                    }
                } else {
                    //a MessageFrame error
                    if(inFrame.error == MessageFrame::FrameError::NON_VALID_SCHEMA || inFrame.error == MessageFrame::FrameError::NON_VALID_ENCODING){
                        //We reset the frame and continue reading...
                        inFrame.reset();
                    } else {
                        //We send an error message back to client
                        setErrorInfo(&outboundMessage, IOErrorCode::MESSAGE_FRAME_ERROR, (byte)inFrame.error);
                        outboundMessage.target = ((ArduinoBoard*)owner)->getID();
                        outboundMessage.sender = outboundMessage.target;
                        outboundMessage.tag = (byte)inFrame.error;
                        inFrame.reset();
                        return false;
                    }
                } //end frame validate
            }
        } //end stream read

        //Not received a message OR stream was empty
        return false;
    }

    bool ArduinoIO::sendMessage(){
        bool retVal = false;
        if(stream != NULL){
            //TODO:??? maybe handle case of if oubound message is in an error state
            if(outFrame.setPayload(outboundMessage.getBytes(), outboundMessage.getByteCount())){
                outFrame.write(stream, framePadding, framePadding); //write bytes to stream
                outFrame.reset();
                retVal = true;
            }
        }
        //ready for reuse
        outboundMessage.clear();
        return retVal;
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

    bool ArduinoIO::enqueueMessageToSend(void* handler, byte messageID, byte messageTag, bool requireUniqueID){
        if(isMessageQueueFull()){
            return false;
        } else {
            int idx = (queueStart + queueCount) % MAX_QUEUE_SIZE;
            messageQueue[idx].handler = (ArduinoMessageHandler*)handler;
            messageQueue[idx].messageID = messageID;
            messageQueue[idx].messageTag = messageTag;
            queueCount++;
            return true;
        }
    }

    ArduinoIO::MessageQueueItem ArduinoIO::dequeueMessageToSend(){
        MessageQueueItem qi;
        if(isMessageQueueEmpty()){
            qi.handler = NULL;
            qi.messageID = 0;
            qi.messageTag = 0;
        } else {
            qi.handler = messageQueue[queueStart].handler;
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