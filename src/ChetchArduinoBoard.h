
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

            void begin(Stream* stream);
            void loop();
            bool receiveMessage(); //true if message has been received, false otherwise
    };

    static ArduinoBoard ArduinoBoard;
}
#endif