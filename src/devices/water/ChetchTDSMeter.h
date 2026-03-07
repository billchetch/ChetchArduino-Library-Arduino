#ifndef CHETCH_TDS_METER_H
#define CHETCH_TDS_METER_H

#include "devices/ChetchAnalogSampler.h"

namespace Chetch{
    class TDSMeter : public AnalogSampler {
        protected:

        public:
            TDSMeter(byte analogPin, unsigned int sampleInterval, uint16_t sampleSize = 1);

            void onSamplingComplete() override;

    }; //end class
} //end namespace
#endif