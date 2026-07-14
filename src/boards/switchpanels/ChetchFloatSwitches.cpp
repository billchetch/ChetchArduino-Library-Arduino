#include "ChetchFloatSwitches.h"

namespace Chetch{
    FloatSwitches::FloatSwitches(byte nodeID, byte serialPin) : CANBusNode(nodeID, serialPin),
                        resetError(SwitchDevice::SwitchMode::ACTIVE, RESET_ERROR_PIN, 100, HIGH),
                        resetSwitch(SwitchDevice::SwitchMode::PASSIVE, RESET_SWITCH_PIN, 100, LOW),
                        normalError(SwitchDevice::SwitchMode::ACTIVE, NORMAL_ERROR_PIN, 100, HIGH),
                        dieselLevel(DIESEL_LEVEL_FIRST_PIN, true, true, 100),
                        bilgeLevel(BILGE_LEVEL_FIRST_PIN, false, false),
                        dieselPump(SwitchDevice::SwitchMode::ACTIVE, DIESEL_PUMP_PIN, 100, HIGH),
                        bilgePump(SwitchDevice::SwitchMode::ACTIVE, BILGE_PUMP_PIN, 100, HIGH),
                        dieselPumpOverride(SwitchDevice::SwitchMode::PASSIVE, DIESEL_PUMP_OVERRIDE_PIN, 100, LOW),
                        bilgePumpOverride(SwitchDevice::SwitchMode::PASSIVE, BILGE_PUMP_OVERRIDE_PIN, 100, LOW)
     {

        resetSwitch.addSwitchListener([](SwitchDevice* device, bool on){
            FloatSwitches* fsb = (FloatSwitches*)device->Board;
            if(!on){
                Serial.println("Reset!");
                fsb->reset();
            }
        });
        
        dieselLevel.addArrayListener([](SwitchArray* device, byte pin, bool on){
            FloatSwitch* fs = (FloatSwitch*)device;
            FloatSwitches* fsb = (FloatSwitches*)device->Board;
            SwitchDevice* pump = &fsb->dieselPump;
            Serial.print("Diesel level: ");
            Serial.println(fs->getOnFlags());

            if(fs->requiresReset()){
                fsb->halt();
            } else if(!fsb->dieselPumpOverriden){
                if(fs->isLow()){
                    pump->turn(true);
                } else if(fs->isHigh()){
                    pump->turn(false);
                }
            }
        });

        dieselPumpOverride.addSwitchListener([](SwitchDevice* device, bool on){
            FloatSwitches* fsb = (FloatSwitches*)device->Board;
            SwitchDevice* pump = &fsb->dieselPump;

            Serial.print("Diesel pump override: ");
            Serial.println(on);

            if(on && !fsb->dieselLevel.requiresReset()){
                fsb->dieselPumpOverriden = true;
                pump->turn(true);
            } else if(fsb->dieselPumpOverriden){
                fsb->dieselPumpOverriden = false;
                pump->turn(false);
                fsb->dieselLevel.trigger();
            }
            
        });

        bilgeLevel.addArrayListener([](SwitchArray* device, byte pin, bool on){
            FloatSwitch* fs = (FloatSwitch*)device;
            FloatSwitches* fsb = (FloatSwitches*)device->Board;
            SwitchDevice* pump = &fsb->bilgePump;

            Serial.print("Bilge level:");
            Serial.println(fs->getOnFlags());

            if(fs->isHigh()){
                pump->turn(true);
            } else if(fs->isLow() && !fsb->bilgePumpOverriden){
                pump->turn(false);
            } 
        });

        bilgePumpOverride.addSwitchListener([](SwitchDevice* device, bool on){
            FloatSwitches* fsb = (FloatSwitches*)device->Board;
            SwitchDevice* pump = &fsb->bilgePump;

            Serial.print("Bilge pump override: ");
            if(on){
                fsb->bilgePumpOverriden = true;
                pump->turn(true);
            } else if(fsb->bilgePumpOverriden) {
                fsb->bilgePumpOverriden = false;
                pump->turn(false);
                fsb->bilgeLevel.trigger();
            }
        });

        //Add devices
        addDevice(&resetError); //ID = 10
        addDevice(&resetSwitch); //ID = 11
        addDevice(&normalError); //ID = 12

        addDevice(&dieselLevel); //ID = 13
        addDevice(&bilgeLevel); //ID = 14

        addDevice(&dieselPump); //ID = 15
        addDevice(&bilgePump); //ID = 16

        addDevice(&dieselPumpOverride); //ID = 17
        addDevice(&bilgePumpOverride); //ID = 18
        
    }

    bool FloatSwitches::begin(MessageIO* io){
        if(!CANBusNode::begin(io))return false;

        dieselLevel.setOnFlags(FloatSwitch::FloatLevel::FL_MID);
        return true;
    }

    void FloatSwitches::halt(){
        dieselPump.turn(false);
        dieselPumpOverriden = false;

        resetError.turn(true);
    }

    void FloatSwitches::reset(){
        if(dieselLevel.requiresReset()){
            dieselLevel.reset();
        }
        resetError.turn(false);
    }

    bool FloatSwitches::override(SwitchDevice* overrideSwitch, bool on){

    }

} //end namespace
