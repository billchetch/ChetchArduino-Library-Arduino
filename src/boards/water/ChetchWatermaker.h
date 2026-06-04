#ifndef CHETCH_WATERMAKER_H
#define CHETCH_WATERMAKER_H

/*
WATERMAKER

Description:
Combines a mode selector with some pressure switches and a start/stop button to turn on solenoids and pumps according to basic logic of 
a RO watermaker for extracting fresh water from salt water.

Comms:
1. CanBus data via MCP2515 .. Pins = 5 (4 pins for SPI + 1 pin for indicator led)
2. Serial Pin slave .. Pins = 1
TOTAL PINS = 6

Display:
1. LCD: Pins = 2 (I2C interface)
TOTAL PINS = 2

Inputs:
1. Selector:  Make, Expel Air, Rinse .. Pins = 3 
2. Start button .. Pins = 1
3. Lower Pressure Switch .. Pins = 1
4. High Pressure Switch .. Pins = 1
TOTAL PINS = 6

Outputs:
1. Water source (i.e. solenoid valves): To open salt water (for making fresh water) or fresh water (for rinsing) .. Pins = 2
2. Feeder pump;  Control pump that feeds water from the source .. Pins = 1
3. HPP:  The high pressure pump (that should only come on if the LPS is on) .. Pins = 1
TOTAL PINS = 4


NOTES:

Date: 2026-02-16
TODO: The solenoids are currently separate switches but they should be a pinselector device because only one can be on at any one time.
However as of writing the PinSelector is only written tested for Passive switches


History:

2026-02-16: Created
2026-
*/

#include "ChetchArduinoBoard.h"
#include "boards/ChetchCANBusNode.h"

#include "devices/displays/ChetchLCDI2C.h"
#include "devices/ChetchSwitchDevice.h"
#include "devices/ChetchSelectorSwitch.h"

#define LCD_COLS 20
#define LCD_ROWS 4
#define LCD_REFRESH LCDI2C::RefreshRate::REFRESH_5HZ
#define DISPLAY_UPDATE_INTERVAL 500 //setReportInterval
#define REPORT_INTERVAL 1000

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

            enum WMErrorCode : byte{
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
                DISPLAY_MODE_NOT_SET= 0,
                RUNNING,
                ERROR ,
                NORMAL,
                DIAGNOSTIC
            };

            struct RunSession{
                OperationalMode opMode = OperationalMode::NOT_SET;
                unsigned long startedOn = 0;
                unsigned long stoppedOn = 0;
                unsigned int count= 0;
            };
            
        private:
            OperationalMode currentMode = OperationalMode::NOT_SET;
            WMErrorCode errorCode = WMErrorCode::NO_ERROR;

            RunSession sessions[SELECTION_SIZE];
            RunSession* currentSession;

            DisplayMode lastDisplayMode = DisplayMode::DISPLAY_MODE_NOT_SET;

            MCP2515Device::NodeDependency* waterMonitorNode = NULL;
            bool waterMonitorPresent = false;
            unsigned long waterMonitorLastUpdate = 0;
            
            int ppm = 0.0;
            double temp = 0.0;
            double flowRate1 = 0.0;
            double waterProduced = 0.0;

        public:
            //Display
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
            
        private:
            void renderWaterMonitor();

        public:
            Watermaker(byte nodeID, byte serialPin, byte waterMonitorNodeID);

            bool begin(MessageIO* io = NULL) override;
            void loop() override;

            bool isRunning();
            bool hasError();
            void selectMode(OperationalMode operationalMode);
            OperationalMode getCurrentMode(){ return currentMode; }
            void start();
            void stop();
            void reset();
            void error(WMErrorCode ec);
            void updateDisplay(DisplayMode displayMode = DisplayMode::DISPLAY_MODE_NOT_SET);
            bool renderDisplay(DisplayMode displayMode, bool displayInitialised = false);

            void setReportInfo(ArduinoMessage* message) override;
            void onReportReady() override;

            void handleReceivedBusMessage(byte sourceNodeID, ArduinoMessage* message, byte* canData) override;
    };
} //end namespace
#endif

