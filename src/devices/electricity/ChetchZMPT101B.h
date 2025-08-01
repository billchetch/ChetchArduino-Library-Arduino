#ifndef CHETCH_ADM_ZMPT101B_H
#define CHETCH_ADM_ZMPT101B_H

#include <Arduino.h>
#include <ChetchArduinoDevice.h>
#include <ChetchADMMessage.h>
#include <ChetchISRTimer.h>

#if defined(ARDUINO_AVR_MEGA2560)
#define TIMER_NUMBER 3
#define TIMER_PRESCALER 8 //'ticks'every 0.5 microseconds
#else
#define TIMER_NUMBER 0
#define TIMER_PRESCALER 0
#endif


namespace Chetch{
    /*
    * Important:  The ZMPT101B can flatten the top and bottom of the wave form if the adjustment screw gives too high a reading.
    * It is necessary to inspect the waveform (half-second intervals are good) and reduce the screw setting so as\ to get a smooth 
    * 'wave' and then scale back up to get the desired values using the 'scaleWaveform' value.
    * 
    */
    class ZMPT101B : public ArduinoDevice {
        public:
            enum Target{
                NONE = 0,
                VOLTAGE = 1,
                HZ = 2,
            };

            enum Direction {
                Stable = 0,
                Rising = 1,
                Falling = 2,
            };

            enum MessageField{
                PIN = 2,
                SAMPLE_SIZE,
                TARGET,
                TARGET_VALUE,
                TARGET_TOLERANCE,
                TARGET_LOWER_BOUND,
                TARGET_UPPER_BOUND,
                //SCALE_WAVEFORM,
                //FINAL_OFFSET,
            };

            static const byte MESSAGE_ID_ADJUSTMENT = 200;
            static const byte BUFFER_SIZE = 128;
            static const byte MAX_INSTANCES = 2;
            static const int EVENT_NEW_RESULTS = 1;
            static const int EVENT_TARGET_REACHED = 2;
            static const int EVENT_TARGET_LOST = 3;
            static const int EVENT_OUT_OF_TARGET_RANGE = 4;

        public: //TODO make private
            static ISRTimer* timer;
            static byte instanceCount;
            static byte currentInstance; //each time an ISR is fired this updates so as to read the next instance voltage
            static ZMPT101B* instances[];
            
            byte instanceIndex = 0;
            byte voltagePin = A0;
            
            bool useTimer = false;
            bool samplingPaused = false;
            volatile int buffer[BUFFER_SIZE];
            volatile int bufferIdx = 0;

            unsigned long sampleSize = 2000; // BUFFER_SIZE;

            //settings for readings
            int midPoint = 512; // 512;
            int hzThresholdVoltage = 100; //number of volts before we consider something above 0 volts
            //double scaleWaveform = 1.0;
            //double finalOffset = 0; //2.5;

            //key properties
            double voltage = 0;
            double hz = 0; 
            double precisionFactor = 10.0*1; //number of dps
            
            
            Target target = Target::NONE;
            double targetValue = -1; //if < 0 then no stabalising/adjustment is required
            double targetTolerance = 0; //deviations tolerated from target value before considered requiring adjustment
            double targetLowerBound = 0; //lower than this we don't consider
            double targetUpperBound = -1; //higher than this we don't consider
            
            Direction voltageDirection = Direction::Stable;
            Direction hzDirection = Direction::Stable;
            
            //event flags
            bool outOfRange = false;
            bool targetLost = false;
            bool targetReached = false; //flag to not raise repeated events


            //debug stuff
            unsigned long val1 = 0;
            unsigned long val2 = 0;
            unsigned long val3 = 0;
            unsigned long val4 = 0;
            unsigned long val5 = 0;

        public: 
            static ZMPT101B* create(byte id, byte cat, char *dn);
            static void handleTimerInterrupt();

            ZMPT101B(byte id, byte cat, char *dn);
            ~ZMPT101B() override;
            void setInstanceIndex(byte idx);
            
            //configure
            int getArgumentIndex(ADMMessage *message, MessageField field);
            bool configure(ADMMessage* message, ADMMessage* response) override;
            void status(ADMMessage* message, ADMMessage* response) override;
            void populateMessageToSend(byte messageID, ADMMessage* message) override;
            void setVoltagePin(byte pin);
            void setTargetParameters(Target t, double tv, double tt, double tlb = 0.0, double tub = -1.0);
            void setHzThresholdVoltage(int threshold);
            void loop() override;

            //results
            void onAnalogRead(uint16_t v);
            void pauseSampling(bool reset);
            void resumeSampling(bool reset);
            bool isSamplingPaused();
            void assignResults(double newVoltage, double newHz);
            double getVoltage();
            double getHz();
            char *getSummary();

            //monitoring stuff
            double getCurrentValue();
            bool inTargetRange();
            double adjustBy(); 
            Direction getDirection(double newVal, double oldValue, double tolerance = 0.0);
            Direction getDesiredDirection();
            Direction getCurrentDirection();

    }; //end class
} //end namespae
#endif