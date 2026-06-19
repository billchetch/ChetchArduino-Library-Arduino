#ifndef CHETCH_FLOAT_SWITCH_H
#define CHETCH_FLOAT_SWITCH_H

#include "devices/ChetchSwitchArray.h"

namespace Chetch{
    class FloatSwitch : public SwitchArray {
        public:

            enum FloatLevel : byte {
                FL_LOW = 0x00,
                FL_MID = 0x01,
                FL_HIGH = 0x03,
                FL_OVERFLOW = 0x07
            }; 

        private:

            bool useOverflow = false;
            bool requireReset = false;
            bool waitForReset = false;
            
        protected:
         
        public:
            FloatSwitch(byte lowPin, bool useOverflow = false, bool requireReset = false, int tolerance = 20, bool onState = LOW);

            
            bool isLow(){ return getOnFlags() == FloatLevel::FL_LOW; }
            bool isMid(){ return getOnFlags() == FloatLevel::FL_MID; }
            bool isHigh(){ return getOnFlags() == FloatLevel::FL_HIGH; }
            bool isOverflow(){ return getOnFlags() == FloatLevel::FL_OVERFLOW; }

            void reset();

            void loop() override;
            //void populateOutboundMessage(ArduinoMessage* message, byte messageID) override;
            //void setReportInfo(ArduinoMessage* message) override;


            void trigger() override;
            
    }; //end class
}//end namespace
#endif