#include "ChetchWaterMonitor.h"

namespace Chetch{
    WaterMonitor::WaterMonitor(byte nodeID, byte serialPin) : CANBusNode(nodeID, serialPin),
                        //display(LCD_COLS, LCD_ROWS, LCD_REFRESH) ,
                        tds(TDS_ANALOG_PIN, TDS_SAMPLE_INTERVAL),
                        tempArray(TEMP_SENSORS_PIN, TEMP_SENSORS_READ_INTERVAL, TEMP_SENSORS_RESOLUTION),
                        flowMeter(FLOWMETER_COUNT_PIN, FLOWMETER_INTERRUPT_MODE)
    {
        //Add event handlers
        /*display.setReportInterval(DISPLAY_UPDATE_INTERVAL); //Setting report interval allows for an interval (rather than direct call) based update
        display.addEventListener([](ArduinoDevice* device, byte eventID, byte eventTag){
            WaterMonitor* wm = (WaterMonitor*)device->Board;
            if(eventID == ArduinoDevice::EVENT_REPORT_READY){
                wm->updateDisplay(DisplayMode::MONITORING);
            }
            return false;
        });
        display.addDisplayHandler([](ArduinoDevice* dd, byte updateTag, bool displayInitialised){
            WaterMonitor* wm = (WaterMonitor*)dd->Board;
            return wm->renderDisplay((DisplayMode)updateTag, displayInitialised);
        });*/

        //Add devices
        //addDevice(&display);
        addDevice(&tds);
        addDevice(&tempArray);
        addDevice(&flowMeter);
    }

    /*
    void WaterMonitor::updateDisplay(DisplayMode displayMode){
        display.updateDisplay(displayMode); //to enter into the loop
    }

    bool WaterMonitor::renderDisplay(DisplayMode displayMode, bool displayInitialised){
        if(displayInitialised){
            display.clearDisplay();
        }
    }
    */

} //end namespace
