
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
#include "ChetchArduinoIO.h"

namespace Chetch{
    class ArduinoBoard{
        public:
            static const byte DEFAULT_BOARD_ID = 1; //the target ID for messaging
            static const byte START_DEVICE_IDS_AT = 8;

            enum class ErrorCode{
                NO_ERROR = 0,
                
            };

        private:
            byte id = DEFAULT_BOARD_ID;
            ArduinoDevice* devices[MAX_DEVICES];
            byte deviceCount = 0;
            byte currentdevice = 0;

            ArduinoIOBase* io = NULL;

            unsigned long unixTimestamp = 0;
            int timezoneOffset = 0;

        protected:
            bool begun = false;

        public:
            //Constructor/Destructor
            ArduinoBoard();
            
            ArduinoIOBase* getIO(){ return io; }

            byte getID(){ return id; }
            void setID(byte id){ this->id = id; }
        
            void addDevice(ArduinoDevice* device);
            ArduinoDevice* getDeviceByID(byte id);
            ArduinoDevice* getDeviceAt(byte idx);
            byte getDeviceCount(){ return deviceCount; };
            int getFreeMemory();
            
            virtual bool begin(ArduinoIOBase* io = NULL); //will return false if fails to begin
            virtual void loop();
            
            void setTime(unsigned long unixts, int tzoffset){ unixTimestamp = unixts; timezoneOffset = tzoffset; }
            unsigned long getUnixTime() { return unixTimestamp + (millis() / 1000); }
            int getTimezoneOffset() { return timezoneOffset; }
    };
}
#endif