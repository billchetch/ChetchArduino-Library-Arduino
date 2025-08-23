
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
#include "ChetchArduinoDevice.h"


namespace Chetch{
    class ArduinoBoard{
        public:
            static const byte DEFAULT_BOARD_ID = 1; //the target ID for messaging
            static const byte START_DEVICE_IDS_AT = 8;
            static const int MAX_QUEUE_SIZE = MAX_DEVICES;

            struct MessageQueueItem{
                ArduinoDevice* device;
                byte messageID; 
                byte messageTag;  //currently not used 1/2/25
            };

            enum class ErrorCode{
                NO_ERROR = 0,
                MESSAGE_FRAME_ERROR = 10, //To indicate this is a Frame error
                MESSAGE_ERROR = 20,
                TARGET_NOT_VALID = 29,
                TARGET_NOT_SUPPLIED = 30,
                TARGET_NOT_FOUND = 31,
                MESSAGE_TYPE_PROHIBITED = 32, //if a particular target rejects a message type
                NO_DEVICE_ID = 40,
                DEVICE_LIMIT_REACHED = 41,
                DEVICE_ID_ALREADY_USED = 42,
                DEVICE_NOT_FOUND = 43,
                DEVICE_ERROR = 100, //To indicate this is an error from the device (not Board)
            };


        private:
            byte id = DEFAULT_BOARD_ID;
            ArduinoDevice* devices[MAX_DEVICES];
            byte deviceCount = 0;
            byte currentdevice = 0;

            MessageFrame frame;
            ArduinoMessage inboundMessage;
            ArduinoMessage outboundMessage;

            int queueStart = 0;
            int queueCount = 0;
            MessageQueueItem messageQueue[MAX_QUEUE_SIZE];

            bool begun = false;
            Stream* stream;

        public:
            //Constructor/Destructor
            ArduinoBoard();
            //~ArduinoBoard();
            byte getID(){ return id; }
            void setID(byte id){ this->id = id; }

            void addDevice(ArduinoDevice* device);
            ArduinoDevice* getDeviceByID(byte id);
            ArduinoDevice* getDeviceAt(byte idx);
            byte getDeviceCount(){ return deviceCount; };
            int getFreeMemory();

            bool begin(Stream* stream); //will return false if fails to begin
            void loop();

            //messaging stuff
            void setErrorInfo(ArduinoMessage* message, ErrorCode errorCode, byte errorSubCode);
            void setResponseInfo(ArduinoMessage* response, ArduinoMessage* message, byte sender);
            bool receiveMessage(); //true if message has been received, false otherwise
            void sendMessage();
            void handleInboundMessage(ArduinoMessage* message, ArduinoMessage* response);

            bool enqueueMessageToSend(ArduinoDevice* device, byte messageID, byte messageTag = 0);
            MessageQueueItem dequeueMessageToSend();
            bool isMessageQueueFull();
            bool isMessageQueueEmpty();
    };

    extern ArduinoBoard Board;
}
#endif