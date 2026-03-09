#include "ChetchFlowMeter.h"

namespace Chetch{

    FlowMeter::FlowMeter(byte pin, byte interruptMode, unsigned long tolerance) : Counter(pin, interruptMode, 1000, tolerance)
    {

    }

    double FlowMeter::getFlowRate(FlowRateUnits units){
        double mlps = calibrationCoeff* getHz();
        switch(units){
            case LITERS_PER_MINUTE:
                return mlps*60.0 / 1000.0;
            
            case ML_PER_SECOND:
                return mlps;

            default:
                return mlps;

        }
    }
}
