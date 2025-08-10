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

            static volatile unsigned long handleInterruptCount;

        private:
            static const byte MAX_INSTANCES = 4;
            static byte instanceCount;
            static Counter* instances[MAX_INSTANCES];
            
            byte instanceIndex = 0; //passed to interrupt
            byte pin = 0;
            byte interruptMode = 0; //can be RISING, FALLING, CHANGE (0 for no interrupt)
            volatile uint8_t * inreg;
            uint8_t bitMask;

            bool pinStateToCount = LOW;
            unsigned long tolerance = 0; //in millis
            volatile bool countStarted = false;
            unsigned long countStartedOn = 0; //in micros as when count started
            volatile unsigned long firstCountOn = 0; //in micros when the first interrupt fired
            volatile unsigned long lastCountOn = 0; //in micros when the last interrupt fired
            volatile unsigned long count = 0;
            volatile unsigned long countedOn = 0;
            volatile bool pinState = LOW;
            
     
        public: 
            static void handleInterrupt(uint8_t pin, uint8_t tag);
            static bool addInstance(Counter* instance);
            

            Counter(byte pin, byte interruptMode = 0, unsigned long tolerance = 0, bool pinStateToCount = HIGH);
            ~Counter();

            void setInstanceIndex(byte idx);
            void setPin(byte pin);
            bool setInterruptMode(byte mode);
            void populateOutboundMessage(ArduinoMessage* message, byte messageID) override;
            void loop() override;
            
            void onInterrupt();
            unsigned long getCount();
    }; //end class
} //end namespae
#endif