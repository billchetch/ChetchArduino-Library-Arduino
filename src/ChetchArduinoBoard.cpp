#include "ChetchArduinoBoard.h"

namespace Chetch{
    void ArduinoBoard::begin(Stream* stream){
        this->stream = stream;
        inboundMessage.clear();
        outboundMessage.clear();
    }
    
    bool ArduinoBoard::receiveMessage(){
        while(stream->available()){
            byte b = stream->read();
            if(frame.add(b)){
            if(frame.validate()){
                //Frame is good so let's get the payload and try turn it into an adm message
                //note: this clears the inbound message
                if(inboundMessage.deserialize(frame.getPayload(), frame.getPayloadSize())){
                    //stream->println("Yahhh");
                    return true;
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
        return false;
    }


    void ArduinoBoard::loop(){
        if(receiveMessage()){
            outboundMessage.clear();
            outboundMessage.add(millis());
            
            //OR??? outboundMessage.serialize(frame.getPayload());
            frame.reset();
            frame.setPayload(outboundMessage.getBytes(), outboundMessage.getByteCount());
            frame.write(stream);
            frame.reset();
        }
    }
}