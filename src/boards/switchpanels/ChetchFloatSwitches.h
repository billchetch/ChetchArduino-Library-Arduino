#ifndef CHETCH_FLOATSWITCHES_H
#define CHETCH_FLOATSWITCHES_H

#include "ChetchArduinoBoard.h"
#include "boards/ChetchCANBusNode.h"

//#include "devices/displays/ChetchLCDI2C.h"
#include "devices/ChetchSwitchDevice.h"
#include "devices/fluids/ChetchFloatSwitch.h"


#define LCD_COLS 20
#define LCD_ROWS 4
#define LCD_REFRESH LCDI2C::RefreshRate::REFRESH_5HZ
#define DISPLAY_UPDATE_INTERVAL 500 //setReportInterval

#define RESET_SWITCH_PIN A4
#define RESET_ERROR_PIN A5 
#define NORMAL_ERROR_PIN A3 
#define ACTIVITY_LIGHT_PIN A2

#define DIESEL_LEVEL_FIRST_PIN 2 //2,3,4
#define BILGE_LEVEL_FIRST_PIN 5 //5, 6

#define DIESEL_PUMP_PIN A0
#define BILGE_PUMP_PIN A1

namespace Chetch{

    class FloatSwitches : public CANBusNode{
        public:
            /*enum ErrorCode : byte{
                NO_ERROR = 0,
                GENERAL_ERROR = 1, 
            };*/

        private:
            //ErrorCode errorCode = ErrorCode::NO_ERROR;

        public:
            //Devices
            ///LCDI2C display;

            SwitchDevice resetError;
            SwitchDevice resetSwitch;
            SwitchDevice normalError;
            SwitchDevice activityLight;

            FloatSwitch dieselLevel;
            FloatSwitch bilgeLevel;
            
            SwitchDevice dieselPump;
            SwitchDevice bilgePump;

        public:
            FloatSwitches(byte nodeID, byte serialPin);

            bool begin(MessageIO* io = NULL) override; //will return false if fails to begin

            void halt();            
            void reset();
            void pump(SwitchDevice* pump, bool on);
            
    }; //end class
} //end namespcae
#endif