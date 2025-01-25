#include "ChetchArduinoBoard.h"

namespace Chetch{
    void ArduinoBoard::begin(Stream* stream){
        this->stream = stream;
    }
    
    void ArduinoBoard::receiveMessage(){
        while(stream->available()){
            byte b = stream->read();
            if(frame.add(b)){
            if(frame.validate()){
                //Frame is good so let's get the payload and try turn it into an adm message
                if(inboundMessage.deserialize(frame.getPayload(), frame.getPayloadSize())){
                    //stream->println("Yahhh");

                } else {
                //an ArduinoMessage deserialization error
                //TODO some error stuff here
                }

            } else {
                //a MessageFrame error
                switch(frame.error){
                case MessageFrame::FrameError::NON_VALID_SCHEMA:
                    break;

                default:
                    break;
                }
            } //end frame validate
            }
        } //end stream read
    }


    void ArduinoBoard::loop(){
        receiveMessage();
    }
}