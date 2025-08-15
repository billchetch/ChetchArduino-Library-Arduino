#include "ChetchTimer.h"

namespace Chetch{

    ISRTimer* Timer::timer = NULL;
    byte Timer::instanceCount = 0;
    Timer* Timer::instances[Timer::MAX_INSTANCES];
    
    int Timer::addInstance(Timer* instance){
        uint32_t interval = instance->getInterval();

        if(instanceCount >= MAX_INSTANCES || TIMER_NUMBER <= 0 || interval == 0){
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
            uint32_t comp = timer->microsToTicks(interval) - 1;
            if(comp > MAX_COMP_VALUE || comp == 0){
                return -2; 
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

            if(timer->addListener(idx + 1, &Timer::handleTimerElapsed, ISRTimer::LOWEST_PRIORITY, comp)){
                Serial.print("Interval in micros is: ");
                Serial.println(interval);
                Serial.print("We are prescaling by: ");
                Serial.println(TIMER_PRESCALE_BY);
                Serial.print("One timer tick in micros is thus: ");
                Serial.println(timer->ticksToMicros(1));
                Serial.print("So the timer will fire after every following ticks: ");
                Serial.println(comp + 1);
                Serial.print("Which is exactly this many micros: ");
                Serial.println(timer->ticksToMicros(comp + 1));
                return (int)idx;
            } else {
                return -1;
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
        int result = addInstance(this);
        switch(result){
            case -1:
                lastError = Timer::FAILED_TO_ADD_INSTANCE; break;

            case -2:
                lastError = Timer::INVALID_INTERVAL; break;

            default:
                lastError = Timer::NO_ERROR; break;
        }

        if(lastError == Timer::NO_ERROR){
            return true;
        } else {
            return false;
        }
    }

    void Timer::loop(){
        ArduinoDevice::loop();

        if(!timer->isEnabled()){
            timer->enable();
        }

        if(elapsed){
            elapsed = false;
            raiseEvent(EVENT_TIMER_ELAPSED);
        }
    }

    void Timer::onTimer(){
        elapsed = true;
        if(timerListener != NULL){
            //NOTE: be aware that this is executing in an interrupt vector
            //also be aware of the possible need for SEI (noInterrupts) and CLI (interrupts)
            timerListener();
        }
    }
} //end namespace
