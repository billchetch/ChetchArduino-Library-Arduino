#ifndef CHETCH_WATERMONITOR_H
#define CHETCH_WATERMONITOR_H

#include "ChetchArduinoBoard.h"
#include "boards/ChetchCANBusNode.h"

//#include "devices/displays/ChetchLCDI2C.h"
#include "devices/water/ChetchTDSMeter.h"
#include "devices/water/ChetchFlowMeter.h"
#include "devices/temperature/ChetchDS18B20Array.h"


#define LCD_COLS 20
#define LCD_ROWS 4
#define LCD_REFRESH LCDI2C::RefreshRate::REFRESH_5HZ
#define DISPLAY_UPDATE_INTERVAL 500 //setReportInterval


#define TDS_ANALOG_PIN A0 //This needs to be an Analog Pin
#define TDS_SAMPLE_INTERVAL 1000 //sample every X ms

#define FLOWMETER1_COUNT_PIN 4
//#define FLOWMETER2_COUNT_PIN 5
//....
#define FLOWMETER_INTERRUPT_MODE 0 //0 doesn't use an interrupt ... if an interrupt is required (e.e. frequent sampling then modify this accordingly (See base class ChetchCounter.h)

#define TEMP_SENSORS_PIN 3 //OneWire pin
#define TEMP_SENSORS_READ_INTERVAL 1000 //read every X ms
#define TEMP_SENSORS_RESOLUTION 9 //this determines read accuracy (9 = 0.5C accuracy)

namespace Chetch{

    class WaterMonitor : public CANBusNode{
        public:
            enum ErrorCode : byte{
                NO_ERROR = 0,
                GENERAL_ERROR = 1, 
            };

            enum DisplayMode : byte{
                DISPLAY_MODE_NOT_SET= 0,
                MONITORING = 1,
                ERROR = 2,
            };

        private:
            ErrorCode errorCode = ErrorCode::NO_ERROR;

            //Devices
            ///LCDI2C display;
            TDSMeter tds;
            DS18B20Array tempArray;
            FlowMeter flowMeter1;
            

        public:
            WaterMonitor(byte nodeID, byte serialPin);

            double getPPM(){ return tds.getPPM(); }
            void setTemperature(double temp){ tds.setTemperature(temp); }
            double getTemperature(){ return tempArray.getTemperature(); }
            double getFlowRate1(){ return flowMeter1.getFlowRate(); }

            //bool hasError();
            //void updateDisplay(DisplayMode displayMode = DisplayMode::DISPLAY_MODE_NOT_SET);
            //bool renderDisplay(DisplayMode displayMode, bool displayInitialised = false);            

    }; //end class
} //end namespcae
#endif