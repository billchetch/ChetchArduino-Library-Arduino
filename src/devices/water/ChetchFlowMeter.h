#ifndef CHETCH_FLOW_METER_H
#define CHETCH_FLOW_METER_H

#include "devices/ChetchCounter.h"

namespace Chetch{
    class FlowMeter : public Counter {
        public:
            enum FlowRateUnits : byte{
                LITERS_PER_MINUTE = 1,
                ML_PER_SECOND = 2,
            };

        private:
            double calibrationCoeff = 2.25;

        public:
            FlowMeter(byte pin, byte interruptMode = 0, unsigned long tolerance = 0);

            double getFlowRate(FlowRateUnits units = FlowRateUnits::LITERS_PER_MINUTE);
    }; //end class
}//end namespace
#endif