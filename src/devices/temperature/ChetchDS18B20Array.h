#ifndef CHETCH_ADM_DS18B20_H
#define CHETCH_ADM_DS18B20_H

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>

namespace Chetch{
    class DS18B20Array : public ArduinoDevice {
        public:
            typedef void (*ReadListener)(DS18B20Array*, byte, float*);  //object, number of sensors, temps

        private:
            byte oneWirePin = 0;
            byte sensorCount = 0;
            byte resolution = 0;
            unsigned int resolutionInterval = 0;
            bool temperatureHasChanged = false; //if at least one temperature has changed since last read
            bool requestTemperatures = false;
            unsigned long lastRequested = 0;
            unsigned int requestInterval = 1000; //in millis
            
            OneWire oneWire;
            DallasTemperature dallasTemp;
            DeviceAddress* deviceAddresses = NULL;

            //uint8_t** deviceAddresses = NULL; //depends on number of sensors attached
            float* temperatures = NULL;

            ReadListener readListener = NULL;

        public: 
            DS18B20Array(byte owPin, unsigned int requestInterval = 1000, byte resolution = 9);
            ~DS18B20Array();
            
            void addReadListener(ReadListener listener){ readListener = listener; }

            bool begin() override;
            //void populateMessageToSend(byte messageID, ADMMessage* message) override;
            void setStatusInfo(ArduinoMessage* message) override;
            void setReportInfo(ArduinoMessage* message) override;
            void loop() override;
            
            byte getSensorCount();
            float* getTemperatures();
            void readTemperatures();
    }; //end class
} //end namespae
#endif