#include "ChetchArduinoBoard.h"

namespace Chetch{
    bool ArduinoBoard::begin(Stream* stream){
        this->stream = stream;
        inboundMessage.clear();
        outboundMessage.clear();

        //Note: could set up an outbound message here which will be sent on first loop
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

    void ArduinoBoard::handleMessage(ArduinoMessage* message, ArduinoMessage* response){
        switch(message->type){
            case ArduinoMessage::TYPE_ECHO:
                response->copy(message);
                response->type = ArduinoMessage::TYPE_ECHO_RESPONSE;
                setResponseInfo(response, message, BOARD_ID);
                break;

            case ArduinoMessage::TYPE_STATUS_REQUEST:
                response->type = ArduinoMessage::TYPE_STATUS_RESPONSE;
                response->add(millis());
                //response->add(BOARD_NAME);
                setResponseInfo(response, message, BOARD_ID);
                break;
        }
    }

    void ArduinoBoard::loop(){
        //1. Send any oustanding message
        if(!outboundMessage.isEmpty()){
            sendMessage();
        }

        //2. Receieve any message and possilbly reply (will get sent next loop)
        if(receiveMessage()){
            //we have received a valid message ... so direct it then to the appropriate place for handling
            outboundMessage.clear();
            switch(inboundMessage.target){
                case ArduinoBoard::BOARD_ID:
                    handleMessage(&inboundMessage, &outboundMessage);
                    break;

                default: //assume this is for device
                    
                    //unrecognised target so error
                    setErrorInfo(&outboundMessage, ErrorCode::TARGET_NOT_FOUND, inboundMessage.target);
                    setResponseInfo(&outboundMessage, &inboundMessage, BOARD_ID);
                    break;
            }
        } //end receive massage

        //3. Loop next device (it may want to send a message for instance)

    }
}