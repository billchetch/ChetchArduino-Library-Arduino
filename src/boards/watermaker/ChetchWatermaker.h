#ifndef CHETCH_WATERMAKER_H
#define CHETCH_WATERMAKER_H

#include "ChetchArduinoBoard.h"
#include "boards/ChetchCANBusNode.h"

#include "devices/displays/ChetchLCDI2C.h"
#include "devices/ChetchSwitchDevice.h"
#include "devices/ChetchSelectorSwitch.h"

#define LCD_COLS 20
#define LCD_ROWS 4
#define LCD_REFRESH LCDI2C::RefreshRate::REFRESH_5HZ

#define SELECTOR_FIRST_PIN 6  //Make water, Expel air, Rinse
#define SELECTION_SIZE 3 //see the options above

#define SWITCH_TOLERANCE 100

#define START_BUTTON_PIN A1 

#define LPS_PIN A2 
#define HPS_PIN A3 

#define OUTPUT_ONSTATE LOW
#define SALT_WATER_SOLENOID_PIN 5
#define FRESH_WATER_SOLENOID_PIN 4
#define FEEDER_PUMP_PIN 3
#define PRESSURE_PUMP_PIN 2

namespace Chetch{

    class Watermaker : public CANBusNode{
        public:
            enum OperationalMode : byte{
                NOT_SET = 0,
                MAKE_WATER = 6, //follows pin number
                EXPEL_AIR = 7, //
                RINSE = 8
            };

            enum ErrorCode : byte{
                NO_ERROR = 0,
                LOW_PRESSURE = 1,
                HIGH_PRESSURE = 2,
                FP_INCORRECT = 3,
                PP_INCORRECT = 4,
                LPS_INCORRECT = 5,
                HPS_INCORRECT = 6,
                CAN_BUS = 7, //Used for debugging
            };

            enum DisplayMode : byte{
                RUNNING = 1,
                ERROR = 2,
                CHANGE_MODE = 3,
                STARTED = 4,
                STOPPED = 5,
            };
            
        private:
            OperationalMode currentMode = OperationalMode::NOT_SET;
            ErrorCode errorCode = ErrorCode::NO_ERROR;

            unsigned long startedOn = 0;
            unsigned long stoppedOn = 0;
            unsigned int runCount = 0;

        public:
            LCDI2C display;

            //Inputs
            SelectorSwitch selector;
            SwitchDevice startButton;
            SwitchDevice lps;
            SwitchDevice hps;

            //Outputs
            SwitchDevice solenoidSalt;
            SwitchDevice solenoidFresh;
            SwitchDevice feederPump;
            SwitchDevice pressurePump;
            
        public:
            Watermaker(byte nodeID, byte serialPin);

            bool isRunning();
            bool hasError();
            void selectMode(OperationalMode operationalMode);
            OperationalMode getCurrentMode(){ return currentMode; }
            void start();
            void stop();
            void reset();
            void error(ErrorCode ec);

    };
} //end namespace
#endif

