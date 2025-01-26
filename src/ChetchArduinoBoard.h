
#ifndef CHETCH_ARDUINO_BOARD_H
#define CHETCH_ARDUINO_BOARD_H

#if defined(ARDUINO_AVR_UNO)
	//Uno specific code
	#define MAX_DEVICES 8	
    #define BOARD_NAME "UNO"	
    #define MAX_FRAME_PAYLOAD_SIZE 50
#elif defined(ARDUINO_AVR_MEGA2560)
	//Mega 2560 specific code
	#define MAX_DEVICES 32
    #define BOARD_NAME "MEGA"
    #define MAX_FRAME_PAYLOAD_SIZE 50
#elif defined(ARDUINO_SAM_DUE)
	#define MAX_DEVICES 16
    #define BOARD_NAME "SAM"
    #define MAX_FRAME_PAYLOAD_SIZE 50
#elif defined(ESP8266)
	#define MAX_DEVICES 8
    #define BOARD_NAME "ESP8266"
    #define MAX_FRAME_PAYLOAD_SIZE 50
#elif defined(ARDUINO_AVR_NANO)
    #define MAX_DEVICES 8
    #define BOARD_NAME "NANO"
    #define MAX_FRAME_PAYLOAD_SIZE 50
#else
    #define MAX_DEVICES 8
    #define BOARD_NAME "OTHER"
    #define MAX_FRAME_PAYLOAD_SIZE 50
//#error Unsupported hardware
#endif

#include <Arduino.h>
#include "ChetchMessageFrame.h"
#include "ChetchArduinoMessage.h"


namespace Chetch{
            
    class ArduinoBoard{
        public:
            static const byte BOARD_ID = 1; //the target ID for messaging

            enum class ErrorCode{
                NO_ERROR = 0,
                MESSAGE_FRAME_ERROR = 10, //To indicate this is a Frame error
                MESSAGE_ERROR = 20,
                TARGET_NOT_FOUND = 30,
                MESSAGE_TYPE_PROHIBITED = 31, //if a particular target rejects a message type
                NO_DEVICE_ID = 40,
                DEVICE_LIMIT_REACHED = 41,
                DEVICE_ID_ALREADY_USED = 42,
                DEVICE_NOT_FOUND = 43,
                DEVICE_ERROR = 100, //To indicate this is an error from the device (not ADM)
            };

        private:
            MessageFrame frame;
            ArduinoMessage inboundMessage;
            ArduinoMessage outboundMessage;
            Stream* stream;

        public:
            //Constructor/Destructor
            ArduinoBoard() : frame(MessageFrame::FrameSchema::SMALL_SIMPLE_CHECKSUM, MAX_FRAME_PAYLOAD_SIZE), 
                                inboundMessage(MAX_FRAME_PAYLOAD_SIZE), 
                                outboundMessage(MAX_FRAME_PAYLOAD_SIZE){}
            //~ArduinoBoard();

            bool begin(Stream* stream); //will return false if fails to begin
            void loop();

            //messaging stuff
            void setErrorInfo(ArduinoMessage* message, ErrorCode errorCode, byte errorSubCode);
            void setResponseInfo(ArduinoMessage* response, ArduinoMessage* message, byte sender);
            bool receiveMessage(); //true if message has been received, false otherwise
            void sendMessage();
            void handleMessage(ArduinoMessage* message, ArduinoMessage* response);
    };

    static ArduinoBoard ArduinoBoard;
}
#endif