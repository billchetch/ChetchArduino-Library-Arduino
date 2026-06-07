
#ifndef CHETCH_ARDUINO_BOARD_H
#define CHETCH_ARDUINO_BOARD_H

#if defined(ARDUINO_AVR_UNO)
	//Uno specific code
	#define MAX_DEVICES 16	
    #define BOARD_NAME "UNO"	
#elif defined(ARDUINO_AVR_MEGA2560)
	//Mega 2560 specific code
	#define MAX_DEVICES 32
    #define BOARD_NAME "MEGA"
#elif defined(ARDUINO_SAM_DUE)
	#define MAX_DEVICES 16
    #define BOARD_NAME "SAM"
#elif defined(ESP8266)
	#define MAX_DEVICES 16
    #define BOARD_NAME "ESP8266"
#elif defined(ARDUINO_AVR_NANO)
    #define MAX_DEVICES 16
    #define BOARD_NAME "NANO"
#else
    #define MAX_DEVICES 16
    #define BOARD_NAME "OTHER"
    //#error Unsupported hardware
#endif

#include <Arduino.h>
#include "ChetchArduinoDevice.h"
#include "ChetchMessageIO.h"

namespace Chetch{
    class ArduinoBoard : public ArduinoMessageHandler{
        public:
            static const byte DEFAULT_BOARD_ID = 1; //the target ID for messaging
            static const byte START_DEVICE_IDS_AT = 8;
            static const byte MESSAGE_ID_REPORT = 1;
            static const byte EVENT_REPORT_READY = 100;

        private:
            ArduinoDevice* devices[MAX_DEVICES];
            byte deviceCount = 0;
            byte currentdevice = 0;

            MessageIO* io = NULL;

            unsigned long unixTimestamp = 0;
            int timezoneOffset = 0;

        
        protected:
            bool begun = false;
            unsigned int reportInterval = 0;
            unsigned long lastMillis = 0;

        public:
            //Constructor/Destructor
            ArduinoBoard();
            
            MessageIO* getIO(){ return io; }

            void addDevice(ArduinoDevice* device);
            ArduinoDevice* getDeviceByID(byte id);
            ArduinoDevice* getDeviceAt(byte idx);
            byte getDeviceCount(){ return deviceCount; };
            int getFreeMemory();
            
            void setReportInterval(unsigned int interval){ reportInterval = interval; }
            virtual void onReportReady();

            virtual bool begin(MessageIO* io = NULL); //will return false if fails to begin
            virtual void loop();

            void handleInboundMessage(ArduinoMessage* message, ArduinoMessage* response) override;
            void populateOutboundMessage(ArduinoMessage* message, byte messageID) override;
            void onOutboundMessageSent(ArduinoMessage* message, byte messageID) override {};

            virtual void setStatusInfo(ArduinoMessage* message);
            virtual void setReportInfo(ArduinoMessage* message);
            
            void setTime(unsigned long unixts, int tzoffset){ unixTimestamp = unixts; timezoneOffset = tzoffset; }
            unsigned long getUnixTime() { return unixTimestamp + (millis() / 1000); }
            int getTimezoneOffset() { return timezoneOffset; }
    };
}
#endif