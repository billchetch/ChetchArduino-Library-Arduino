#ifndef CHETCH_FLOAT_SWITCH_H
#define CHETCH_FLOAT_SWITCH_H

#include "devices/ChetchSwitchArray.h"

namespace Chetch{
    class FloatSwitch : public SwitchArray {
        private:
            
        protected:
         
        public:
            //FloatSwitch(byte lowPin, int tolerance = 20, bool onState = LOW);

            FloatSwitch(byte lowPin, int tolerance = 20, bool onState = LOW);

            //void trigger() override;

            bool isLow(){ return getOnFlags() == 0x00; }
            bool isMid(){ return getOnFlags() == 0x01; }
            bool isHigh(){ return getOnFlags() == 0x03; }
            //void populateOutboundMessage(ArduinoMessage* message, byte messageID) override;
            //void setReportInfo(ArduinoMessage* message) override;

    }; //end class
}//end namespace
#endif