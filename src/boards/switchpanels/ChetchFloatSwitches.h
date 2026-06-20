#ifndef CHETCH_FLOATSWITCHES_H
#define CHETCH_FLOATSWITCHES_H

#include "ChetchArduinoBoard.h"
#include "boards/ChetchCANBusNode.h"

//#include "devices/displays/ChetchLCDI2C.h"
#include "devices/ChetchSwitchDevice.h"
#include "devices/water/ChetchFloatSwitch.h"


#define LCD_COLS 20
#define LCD_ROWS 4
#define LCD_REFRESH LCDI2C::RefreshRate::REFRESH_5HZ
#define DISPLAY_UPDATE_INTERVAL 500 //setReportInterval

#define RESET_SWITCH_PIN 3

#define DIESEL_LEVEL_FIRST_PIN 4 //4,5
#define BILGE_LEVEL_FIRST_PIN 6 //6, 7, 8

#define DIESEL_PUMP_PIN A1
#define BILGE_PUMP_PIN A2

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

            SwitchDevice resetSwitch;

            FloatSwitch dieselLevel;
            FloatSwitch bilgeLevel;
            
            SwitchDevice dieselPump;
            SwitchDevice bilgePump;

        public:
            FloatSwitches(byte nodeID, byte serialPin);

            void reset();            
    }; //end class
} //end namespcae
#endif