#ifndef CHETCH_ARDUINO_IO_H
#define CHETCH_ARDUINO_IO_H

#if defined(ARDUINO_AVR_UNO)
	//Uno specific code
	#define MAX_QUEUE_SIZE 8
    #define MAX_FRAME_PAYLOAD_SIZE 32
#elif defined(ARDUINO_AVR_MEGA2560)
	//Mega 2560 specific code
	#define MAX_QUEUE_SIZE 8
    #define MAX_FRAME_PAYLOAD_SIZE 64
#elif defined(ARDUINO_SAM_DUE)
	#define MAX_QUEUE_SIZE 8
    #define MAX_FRAME_PAYLOAD_SIZE 32
#elif defined(ESP8266)
	#define MAX_QUEUE_SIZE 8
    #define MAX_FRAME_PAYLOAD_SIZE 50
#elif defined(ARDUINO_AVR_NANO)
    #define MAX_QUEUE_SIZE 8
    #define MAX_FRAME_PAYLOAD_SIZE 32
#else
    #define MAX_QUEUE_SIZE 8
    #define MAX_FRAME_PAYLOAD_SIZE 32
//#error Unsupported hardware
#endif

#include <Arduino.h>
#include "ChetchMessageIO.h"
#include "ChetchMessageFrame.h"
#include "ChetchArduinoMessage.h"
#include "ChetchArduinoDevice.h"

namespace Chetch{
    class ArduinoIO : public MessageIO{
        public:
            struct MessageQueueItem{
                ArduinoDevice* device;
                byte messageID; 
                byte messageTag;  //currently not used 1/2/25
            };

            enum class IOErrorCode{
                NO_ERROR = 0,
                MESSAGE_FRAME_ERROR = 10, //To indicate this is a Frame error
                MESSAGE_ERROR = 20,
                MESSAGE_TYPE_PROHIBITED = 32, //if a particular target rejects a message type
                TARGET_NOT_VALID = 29,
                TARGET_NOT_SUPPLIED = 30,
                TARGET_NOT_FOUND = 31,
                NO_DEVICE_ID = 40,
                DEVICE_LIMIT_REACHED = 41,
                DEVICE_ID_ALREADY_USED = 42,
                DEVICE_NOT_FOUND = 43,
                DEVICE_ERROR = 100, //To indicate this is an error from the device (not Board)
            };

        private:
            Stream* stream;

            MessageFrame frame;
            ArduinoMessage inboundMessage;
            ArduinoMessage outboundMessage;
            int queueStart = 0;
            int queueCount = 0;
            MessageQueueItem messageQueue[MAX_QUEUE_SIZE];

        public:
            ArduinoIO(Stream* stream = NULL);

            void begin(Stream* stream = NULL);
            bool enqueueMessageToSend(void* device, byte messageID, byte messageTag = 0) override;
            void loop() override;

            //messaging stuff
            bool receiveMessage(); //true if message has been received, false otherwise
            void sendMessage();

            void setErrorInfo(ArduinoMessage* message, IOErrorCode errorCode, byte errorSubCode);
            void setErrorInfo(ArduinoMessage* message, byte errorCode, byte errorSubCode);
            void setResponseInfo(ArduinoMessage* response, ArduinoMessage* message, byte sender);
              
            MessageQueueItem dequeueMessageToSend();
            bool isMessageQueueFull();
            bool isMessageQueueEmpty();
    };
}
#endif
