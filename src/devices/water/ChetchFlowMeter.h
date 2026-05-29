#ifndef CHETCH_FLOW_METER_H
#define CHETCH_FLOW_METER_H

#include "devices/ChetchCounter.h"

namespace Chetch{
    class FlowMeter : public Counter {
        public:
            static const byte MESSAGE_ID_FLOW_RATE = 2;
            
            enum FlowRateUnits : byte{
                USE_DEFAULT = 0,
                ML_PER_SECOND = 1, //default reading due to assumed sensitivity/range issues
                LITERS_PER_SECOND = 2, 
                LITERS_PER_MINUTE = 3,
            };

            typedef void (*FlowRateListener)(FlowMeter*, double);  //object, flow rate

        private:
            double calibrationCoeff = 2.25; //MLPS: This default value is for YF-S201 ... might want to create a list/enum/defines for this
            FlowRateUnits defaultUnits;
            FlowRateListener rateListener = NULL;

        public:
            FlowMeter(byte pin, FlowRateUnits units = FlowRateUnits::ML_PER_SECOND, byte interruptMode = 0, unsigned long tolerance = 0);

            void addFlowRateListener(FlowRateListener listener){ rateListener = listener; }
            double getFlowRate(FlowRateUnits units = FlowRateUnits::USE_DEFAULT);
            
            void assignValues() override;
            void populateOutboundMessage(ArduinoMessage* message, byte messageID) override;
            
            void setReportInfo(ArduinoMessage* message) override;

            bool executeCommand(DeviceCommand command, ArduinoMessage* message, ArduinoMessage* response) override;

    }; //end class
}//end namespace
#endif