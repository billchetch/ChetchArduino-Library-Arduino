#ifndef CHETCH_ARDUINO_DEVICE_H
#define CHETCH_ARDUINO_DEVICE_H

#include <Arduino.h>
#include "ChetchArduinoMessage.h"


namespace Chetch{
    class ArduinoBoard;

    class ArduinoDevice{
        public:
            enum DeviceCommand{
                NONE = 0,
                COMPOUND,
                TEST,
                ENABLE,
                DISABLE,
                SET_REPORT_INTERVAL,
                START,
                STOP,
                PAUSE,
                RESET,
                ON,
                OFF,
                MOVE,
                ROTATE,
                PRINT,
                SET_CURSOR,
                DIZPLAY, //changed S to Z to avoid a define constant name clash
                CLEAR,
                SILENCE,
                SEND,
                TRANSMIT,
                SAVE,
                ACTIVATE,
                DEACTIVATE,
                RESUME,
                ZERO,
                ANALYSE,
            };

        public:
            static const byte MESSAGE_ID_REPORT = 1;

            typedef bool (*EventListener)(ArduinoDevice*, byte);

        public:
            ArduinoBoard* Board = NULL;
            byte id = 0;

        private:
            EventListener eventListener = NULL;

        protected:
            unsigned long lastMillis = 0;
            int reportInterval = -1; //measured in millis. negative or zero means no reporting
            unsigned long timerInterval = 0; //in micros. set this to a positive value to register with timer events
            int timerTicks = 0; //this is calculated and set by ADm based on timer Hz and timerInterval value
        
        public:
            void addEventListener(EventListener listener);
            bool raiseEvent(byte eventID);

            void setReportInterval(int interval) { reportInterval = interval; }
            
            bool enqueueMessageToSend(byte messageID);
            
            virtual void handleInboundMessage(ArduinoMessage* message, ArduinoMessage* response);
            virtual void populateOutboundMessage(ArduinoMessage* message, byte messageID);
            
            void setErrorInfo(ArduinoMessage* message, byte errorSubCode);
            virtual void setReportInfo(ArduinoMessage* message){};
            virtual void setStatusInfo(ArduinoMessage* message);

            virtual bool executeCommand(DeviceCommand command, ArduinoMessage* message, ArduinoMessage* response);

            virtual bool begin(){ return true; }; //a hook
            virtual void loop();
    };
} //end namespace scope
#endif