#include "ChetchArduinoMessage.h"

namespace Chetch{

    ArduinoMessage::ErrorCode ArduinoMessage::error = ArduinoMessage::NO_ERROR;
    bool ArduinoMessage::hasError(){
        return ArduinoMessage::error != ArduinoMessage::NO_ERROR;
    }

    
    /*
    * Constructor
    */

    ArduinoMessage::ArduinoMessage(byte maxBytes){
        this->maxBytes = maxBytes;
        
        bytes = new byte[maxBytes];
        argumentIndices = new byte[(maxBytes - HEADER_SIZE) / 2];
        newID();
    };

    ArduinoMessage::~ArduinoMessage() {
        delete[] bytes;
        delete[] argumentIndices;
    }

    void ArduinoMessage::newID(){
        id = millis();
    }

    void ArduinoMessage::clear(){
        type = (byte)MessageType::TYPE_NONE;
        tag = 0;
        target = 0;
        sender = 0;

        for(int i = 0; i < byteCount; i++)bytes[i] = 0;
        for(int i = 0; i < argumentCount; i++)argumentIndices[i] = 0;

        byteCount = ArduinoMessage::HEADER_SIZE;
        argumentCount = 0;
        newID();
    }

    bool ArduinoMessage::isEmpty(){
        return type == (byte)MessageType::TYPE_NONE && argumentCount == 0;
    }

    void ArduinoMessage::copy(ArduinoMessage *message){
        clear();
        byte *mbytes = message->getBytes();
        for(int i = 0; i < message->getByteCount(); i++){
            bytes[i] = mbytes[i];
        }
        argumentCount = message->getArgumentCount();
        byteCount = message->getByteCount();
    }


    byte ArduinoMessage::getArgumentCount(){
        return argumentCount;
    }

    bool ArduinoMessage::hasArgument(byte argIdx){
        return argIdx < argumentCount;
    }

    byte *ArduinoMessage::getArgument(byte argIdx){
        if(hasArgument(argIdx)){
            return &bytes[argumentIndices[argIdx] + 1]; //add 1 cos the first byte designates the argument size
        } else {
            return NULL;
        }       
    }

    byte ArduinoMessage::getArgumentSize(byte argIdx){
        if(hasArgument(argIdx)){
            return bytes[argumentIndices[argIdx]];
        } else {
            return 0;
        }
    }

    //Adding values
    void ArduinoMessage::addBytes(byte *bytev, byte bytec){
        if(byteCount + bytec + 1 > maxBytes)return;
        if(byteCount < ArduinoMessage::HEADER_SIZE)byteCount = ArduinoMessage::HEADER_SIZE;

        argumentIndices[argumentCount++] = byteCount; //record the index
        bytes[byteCount++] = bytec; //add the number of bytes this argument requires
        for(int i = 0; i < bytec; i++){
            bytes[byteCount++] = bytev[i]; //add the bytes
        }
    }

    void ArduinoMessage::add(byte argv){
        addBytes((byte*)&argv, sizeof(argv));
    }

    void ArduinoMessage::add(bool argv){
        addBytes((byte*)&argv, sizeof(argv));
    }
  
    void ArduinoMessage::add(int argv){
        addBytes((byte*)&argv, sizeof(argv));
    }

    void ArduinoMessage::add(unsigned int argv){
        addBytes((byte*)&argv, sizeof(argv));
    }

    void ArduinoMessage::add(long argv){
        addBytes((byte*)&argv, sizeof(argv));
    }

    void ArduinoMessage::add(unsigned long argv){
        addBytes((byte*)&argv, sizeof(argv));
    }

    void ArduinoMessage::add(const char *argv){
        addBytes((byte*)argv, strlen(argv));
    }
  
    void ArduinoMessage::add(float argv){
        addBytes((byte*)&argv, sizeof(argv));
    }

    void ArduinoMessage::add(double argv){
        add((float)argv);
    }
  
    //Serialization
    byte* ArduinoMessage::getBytes(){
        //Add all the message data to the byte array
        bytes[0] = type;
        bytes[1] = tag;
        bytes[2] = target;
        bytes[3] = sender;

        return bytes;
    }

    byte ArduinoMessage::getByteCount(){
        return byteCount;
    }

    byte ArduinoMessage::serialize(byte* destination){
        getBytes(); // will add everything to byte array
        
        if(destination != NULL){
            for(int i = 0; i < byteCount; i++){
                destination[i] = bytes[i];
            }
        }

        return byteCount;
    }

    bool ArduinoMessage::deserialize(byte* source, byte bCount){
        clear();
        ArduinoMessage::error = ArduinoMessage::NO_ERROR;

        int argByteCountIdx = ArduinoMessage::HEADER_SIZE;
        if(bCount < argByteCountIdx){
            ArduinoMessage::error = ArduinoMessage::ERROR_INSUFFICIENT_BYTES;
            return false;
        }

        //move everything to the bytes array
        byteCount = bCount;
        for(int i = 0; i < byteCount; i++){
            bytes[i] = source[i];
        }

        //Member vars (= HEADER)
        type = bytes[0];
        tag = bytes[1];
        target = bytes[2];
        sender = bytes[3];

        //Arguments
        int idx = argByteCountIdx;
        while(idx < byteCount){
            argumentIndices[argumentCount++] = idx;
            idx += bytes[idx] + 1;
        }
        if(idx != byteCount){
            ArduinoMessage::error = ArduinoMessage::ERROR_BADLY_FORMED;
            return false;
        }
        return true;
    }
} //end of namespace