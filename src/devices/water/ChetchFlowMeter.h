#ifndef CHETCH_FLOW_METER_H
#define CHETCH_FLOW_METER_H

#include "devices/ChetchCounter.h"

namespace Chetch{
    class FlowMeter : public Counter {
        public:
            enum FlowRateUnits : byte{
                ML_PER_SECOND = 1, //default reading due to assumed sensitivity/range issues
                LITERS_PER_SECOND = 2, 
                LITERS_PER_MINUTE = 3,
            };

        private:
            double calibrationCoeff = 2.25; //MLPS: This default value is for YF-S201 ... might want to create a list/enum/defines for this

        public:
            FlowMeter(byte pin, byte interruptMode = 0, unsigned long tolerance = 0);

            double getFlowRate(FlowRateUnits units = FlowRateUnits::ML_PER_SECOND);

            void setReportInfo(ArduinoMessage* message) override;
    }; //end class
}//end namespace
#endif