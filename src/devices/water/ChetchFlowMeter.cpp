#include "ChetchFlowMeter.h"

namespace Chetch{

    FlowMeter::FlowMeter(byte pin, byte interruptMode, unsigned long tolerance) : Counter(pin, interruptMode, 1000, tolerance)
    {

    }

    double FlowMeter::getFlowRate(FlowRateUnits units){
        double mlps = calibrationCoeff* getHz();
        switch(units){
            case ML_PER_SECOND:
                return mlps;

            case LITERS_PER_SECOND:
                return mlps / 1000.0;
            
            case LITERS_PER_MINUTE:
                return mlps*0.06; (60/1000);

            default:
                return mlps;

        }
    }

    void FlowMeter::setReportInfo(ArduinoMessage* message){
        Counter::setReportInfo(message);

        message->add(getFlowRate());
    }
}
