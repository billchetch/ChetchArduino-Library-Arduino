#include "ChetchTimer.h"

namespace Chetch{

    ISRTimer* Timer::timer = NULL;
    byte Timer::instanceCount = 0;
    Timer* Timer::instances[Timer::MAX_INSTANCES];
    
    int Timer::addInstance(Timer* instance){
        uint32_t interval = instance->getInterval();

        if(instanceCount >= MAX_INSTANCES || TIMER_NUMBER <= 0 || interval == 0){
            lastError = TimerError::FAILED_TO_ADD_INSTANCE;
            return -1;
        } else {
            if(instanceCount == 0){
                cli();
                timer = ISRTimer::create(TIMER_NUMBER, TIMER_PRESCALE_BY, ISRTimer::TimerMode::COMPARE);
                sei();

                for(byte i = 0; i < MAX_INSTANCES; i++){
                    instances[i] = NULL;
                }
            }

            //validate this duration
            uint32_t comp = timer->microsToTicks(interval);
            if(comp > MAX_COMP_VALUE || compe == 0){
                lastError = TimerError::INVALID_INTERVAL;
                return -1; 
            }


            //get first available slot
            byte idx = 0;
            for(byte i = 0; i < MAX_INSTANCES; i++){
                if(instances[i] == NULL){
                    idx = i;
                    break;
                }
            }

            instances[idx] = instance;
            instanceCount++;

            /*Serial.print("Interval in micros is: ");
            Serial.println(interval);
            Serial.print("We are prescaling by: ");
            Serial.println(TIMER_PRESCALE_BY);
            Serial.print("One timer tick in micros is thus: ");
            Serial.println(timer->ticksToMicros(1));
            Serial.print("So the timer will fire after this many ticks: ");
            Serial.println(comp);
            Serial.print("Now add a listener to the timer wiht ID (index + 1): ");
            Serial.println(idx + 1);*/

            if(!timer->addListener(idx + 1, &Timer::handleTimerElapsed, ISRTimer::LOWEST_PRIORITY, comp)){
                lastError = TimerError::FAILED_TO_ADD_INSTANCE;
                return -1;
            } else {
                return (int)idx;
            }
        }
    }
    
    void Timer::removeInstance(Timer* instance){
        for(byte i = 0; i < MAX_INSTANCES; i++){
            if(instances[i] == instance){
                timer->removeListener(i);
                instances[i] = NULL;
                instanceCount--;

                if(instanceCount == 0 && timer->isEnabled())timer->disable();
                break;
            }
        }
    }

    void Timer::handleTimerElapsed(uint8_t id){
        static Timer* t = NULL;
        static uint8_t idx = id - 1;
        if(idx < MAX_INSTANCES){
            t = instances[idx];
            if(t != NULL)t->onTimer();
        }
    }

    Timer::Timer(uint32_t interval){
        this->interval = interval;

    }

    Timer::~Timer(){
        removeInstance(this);
    }

    bool Timer::begin(){
        if(addInstance(this) < 0){
            return false;
        }
        
        return true;
    }

    void Timer::loop(){
        ArduinoDevice::loop();

        static unsigned long ms = 0;    

        if(!timer->isEnabled()){
            timer->enable();
            ms = millis();
        }

        if(elapsed){
            elapsed = false;
            raiseEvent(EVENT_TIMER_ELAPSED);
            Serial.print("ms: ");
            Serial.println(millis() - ms);
            ms = millis();
        }
    }

    void Timer::onTimer(){
        elapsed = true;
    }
} //end namespace
