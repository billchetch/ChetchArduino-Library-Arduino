#ifndef CHETCH_ARDUINO_TIMER_H
#define CHETCH_ARDUINO_TIMER_H

#include <Arduino.h>
#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>
#include <ChetchISRTimer.h>

/*
PRESCALER INFO
Normal prescaler values are 0, 1, 8, 64, 256, 1024 and will dived the clock by that time
*/

#if defined(ARDUINO_AVR_MEGA2560)
    #define TIMER_NUMBER 4
    #define TIMER_PRESCALE_BY 8
    #define MAX_COMP_VALUE 0xFFFF //because timer 4 is 16bit
#elif defined(ARDUINO_AVR_NANO)
    #define TIMER_NUMBER 1 //WARNING: Conflicts with Servo Library
    #define TIMER_PRESCALE_BY 1024
    #define MAX_COMP_VALUE 0xFFFF //because timer 1 is 16bit
#elif defined(ARDUINO_AVR_UNO)
    #define TIMER_NUMBER 1 //WARNING: Conflicts with Servo Library
    #define TIMER_PRESCALE_BY 1024 
    #define MAX_COMP_VALUE 0xFFFF //because timer 1 is 16bit
#else
    #define TIMER_NUMBER 0
    #define TIMER_PRESCALE_BY 0
    #define MAX_COMP_VALUE 0
#endif

namespace Chetch{
    class Timer : public ArduinoDevice {
        public:
            static const byte EVENT_TIMER_ELAPSED = 100;
            enum TimerError{
                NO_ERROR,
                INVALID_INTERVAL,
                FAILED_TO_ADD_INSTANCE
            };

            typedef void (*TimerListener)();

        private:
            static ISRTimer* timer; //the underlying timer
            static const byte MAX_INSTANCES = ISRTimer::MAX_CALLBACKS;
            static Timer* instances[];
            static byte instanceCount;

            TimerError lastError = NO_ERROR;
            uint32_t interval = 0;
            TimerListener timerListener = NULL;
            
            volatile bool elapsed = false;

        public: 
            static int addInstance(Timer* instance);
            static void removeInstance(Timer* instance);
            static void handleTimerElapsed(uint8_t id);

            Timer(uint32_t interval);
            ~Timer();

            TimerError getLastError(){ return lastError; }
            uint32_t getInterval(){ return interval; }
            
            void addTimerListener(TimerListener listener){ timerListener = listener; }

            bool begin() override;
            void loop() override;

            void onTimer();

    }; //end class
} //end namespae
#endif