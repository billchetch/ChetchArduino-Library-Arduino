#ifndef CHETCH_ADM_COUNTER_H
#define CHETCH_ADM_COUNTER_H

#include <Arduino.h>
#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>

namespace Chetch{
    class Counter : public ArduinoDevice {
        public:
            enum class ErrorSubCode{
                FAILED_INTTERUPT_MODE = 101, //check pin is interrupt possible pin (see CINterrupt)
            };

            static const byte EVENT_ASSIGNED_VALUES = 101;

        private:
            static const byte MAX_INSTANCES = 4;
            static byte instanceCount;
            static Counter* instances[MAX_INSTANCES];
            
            byte instanceIndex = 0; //passed to interrupt
            byte pin = 0;
            byte interruptMode = 0; //can be RISING, FALLING, CHANGE (0 for no interrupt and use loop instead)
            volatile uint8_t * inreg;
            uint8_t bitMask;

            bool pinStateToCount = LOW; //Only used if we are looping i.e. NOT using an Interrupt
            unsigned long tolerance = 0; //in millis
            volatile bool countStarted = false;
            unsigned long countStartedOn = 0; //in micros as when count started
            unsigned long assignValuesAfter = 0; //time for which to assign values
            
            volatile unsigned long firstCountOn = 0; //in micros when the first interrupt fired
            volatile unsigned long lastCountOn = 0; //in micros when the last interrupt fired
            volatile unsigned long count = 0;
            volatile unsigned long countedOn = 0;
            volatile bool pinState = LOW;



        public:
            //These store the values at reportInteval moments so as to be refernced rather than the 'hot' values which will rapidly change
            unsigned long lastCount = 0;
            unsigned long lastDuration = 0;

        private:
            void resetCount();
            
        public: 
            static void handleInterrupt(uint8_t pin, uint8_t tag);
            static int addInstance(Counter* instance);
            

            //NOTE: that intteruptMode can be 0 (we use a loop instead), or RISING, CHANGE, FALLING.  If we use an interrupt the pinStateToCount is ignored
            Counter(byte pin, byte interruptMode = 0, unsigned long assignValuesAfter = 1000, unsigned long tolerance = 0, bool pinStateToCount = HIGH);
            ~Counter();

            void populateOutboundMessage(ArduinoMessage* message, byte messageID) override;
            virtual void assignValues(); //make virtual so we can override for functionality like RPM
            double getHz();

            bool begin() override;
            void loop() override;
            
            void onInterrupt();
            unsigned long getCount();
    }; //end class
} //end namespae
#endif