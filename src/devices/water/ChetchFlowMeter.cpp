#include "ChetchFlowMeter.h"

namespace Chetch{

    FlowMeter::FlowMeter(byte pin, FlowRateUnits units, byte interruptMode, unsigned long tolerance) : Counter(pin, interruptMode, 1000, tolerance)
    {
        defaultUnits = units;
    }

    double FlowMeter::getFlowRate(FlowRateUnits units){
        double mlps = calibrationCoeff * getHz();
        if(units == FlowRateUnits::USE_DEFAULT){
            units = defaultUnits;
        }
        Serial.print("Using units: ");
        Serial.println(units);
        switch(units){
            case ML_PER_SECOND:
                return mlps;

            case LITERS_PER_SECOND:
                return mlps / 1000.0;
            
            case LITERS_PER_MINUTE:
                return mlps*0.06; //0.06 = (60/1000);

            default:
                return mlps;

        }
    }

    void FlowMeter::assignValues(){
        Counter::assignValues();

        if(rateListener != NULL){
            rateListener(this, getFlowRate());
        }
    }

    void FlowMeter::setReportInfo(ArduinoMessage* message){
        Counter::setReportInfo(message);

        message->add(getFlowRate());
    }

    bool FlowMeter::executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response){
        bool handled = false;
        double calibratedFlowRate;
        double flowRate;
        switch(command){
            case CALIBRATE:
                calibratedFlowRate = message->get<double>(1);
                flowRate = getFlowRate();
                if(flowRate > 0 && calibratedFlowRate > 0){
                    calibrationCoeff = calibrationCoeff * (calibratedFlowRate / flowRate);
                    response->add(calibrationCoeff);
                    handled = true;
                } else {
                    handled = false;
                }
                break;

            default:
                handled = false;
                break;
        }
                
        return handled;
    }
}
