#include "ChetchUtils.h"
#include "ChetchDS18B20Array.h"

namespace Chetch{
    
    DS18B20Array::DS18B20Array(byte owPin, unsigned int requestInterval, byte resolution) : ArduinoDevice(),
        oneWire(owPin),
        dallasTemp(&oneWire)
    
    {
        oneWirePin = owPin;
        this->requestInterval = requestInterval;
        this->resolution = resolution;

        /*
        From gemini:
        9-bit: 
            resolution — 93.75 ms 
        10-bit: 
            resolution — 187.5 ms
        11-bit: 
            resolution — 375 ms
        12-bit: 
            resolution — 750 ms 
        */
        switch(resolution){
            case 9:
                resolutionInterval = 100;
                break;
            case 10:
                resolutionInterval = 200;
                break;
            case 11:
                resolutionInterval = 400;
                break;
            case 12:
                resolutionInterval = 750;
                break;
            default:
                resolutionInterval = 750;
                break;
        }

        if(resolutionInterval > requestInterval){
            requestInterval = 2*resolutionInterval;
        }
    }

    DS18B20Array::~DS18B20Array() {
        if (sensorCount > 0) {
            delete[] deviceAddresses;
            delete[] temperatures;
        }
    }

    bool DS18B20Array::locateSensors(){
        sensorCount = dallasTemp.getDeviceCount();
        
        if(sensorCount == 0){
            return false;   
        }

        deviceAddresses = new DeviceAddress[sensorCount];
        temperatures = new float[sensorCount];

        for (byte i = 0; i < sensorCount; i++) {
            dallasTemp.getAddress(deviceAddresses[i], i);
            temperatures[i] = 0.0f;
        }

        return true;
    }

    bool DS18B20Array::begin(){
            
        dallasTemp.begin();

        dallasTemp.setResolution(resolution);
        dallasTemp.setWaitForConversion(false);

        requestTemperatures = locateSensors();

        return ArduinoDevice::begin();
    }

    void DS18B20Array::setStatusInfo(ArduinoMessage* message){
        ArduinoDevice::setStatusInfo(message);

        message->add(sensorCount);
    }

    void DS18B20Array::setReportInfo(ArduinoMessage* message){
        ArduinoDevice::setReportInfo(message);

        for(byte i = 0; i < sensorCount; i++) {
            message->add(temperatures[i]);
        }
    }

    byte DS18B20Array::getSensorCount(){
        return sensorCount;
    }

    float* DS18B20Array::getTemperatures(){
        return temperatures;
    }

    void DS18B20Array::readTemperatures(){
        for (byte i = 0; i < sensorCount; i++) {
            temperatures[i] = dallasTemp.getTempC(deviceAddresses[i]);
        }
    }

    void DS18B20Array::loop(){
        ArduinoDevice::loop();

        unsigned long sinceLastRequested = millis() - lastRequested;
        if(sensorCount == 0){
            if(sinceLastRequested > 1000){ //we check every second for sensors
                lastRequested = millis();
                requestTemperatures = locateSensors();
            }
            return;
        }

        //If here we have located sensors
        if(requestTemperatures){
            if(sinceLastRequested > requestInterval) {
                dallasTemp.requestTemperatures();
                lastRequested = millis();
                requestTemperatures = false; //do not request again until a read is done
            }
        } else if(sinceLastRequested > resolutionInterval){
            readTemperatures();
            requestTemperatures = true; //auto reset
            if(readListener != NULL){
                readListener(this, sensorCount, temperatures);
            }
        }
    }
} //end namespace
