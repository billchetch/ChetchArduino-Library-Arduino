#include "ChetchFloatSwitches.h"

namespace Chetch{
    FloatSwitches::FloatSwitches(byte nodeID, byte serialPin) : CANBusNode(nodeID, serialPin),
                        resetError(SwitchDevice::SwitchMode::ACTIVE, RESET_ERROR_PIN, 100, HIGH),
                        resetSwitch(SwitchDevice::SwitchMode::PASSIVE, RESET_SWITCH_PIN, 100, LOW),
                        normalError(SwitchDevice::SwitchMode::ACTIVE, NORMAL_ERROR_PIN, 100, HIGH),
                        activityLight(SwitchDevice::SwitchMode::ACTIVE, ACTIVITY_LIGHT_PIN, 100, HIGH),
                        dieselLevel(DIESEL_LEVEL_FIRST_PIN, true, true, 100),
                        bilgeLevel(BILGE_LEVEL_FIRST_PIN, false, false),
                        dieselPump(SwitchDevice::SwitchMode::ACTIVE, DIESEL_PUMP_PIN, 100, LOW),
                        bilgePump(SwitchDevice::SwitchMode::ACTIVE, BILGE_PUMP_PIN, 100, LOW)
     {

        resetSwitch.addSwitchListener([](SwitchDevice* device, bool on){
            FloatSwitches* fsb = (FloatSwitches*)device->Board;
            if(!on){
                Serial.println("Reset!");
                fsb->reset();
            }
        });
        
        dieselLevel.addSwitchListener([](SwitchDevice* device, bool on){
            FloatSwitch* fs = (FloatSwitch*)device;
            FloatSwitches* fsb = (FloatSwitches*)device->Board;
            SwitchDevice* pump = &fsb->dieselPump;
            Serial.print("Diesel level: ");
            Serial.println(fs->getOnFlags());

            if(fs->isLow()){
                fsb->pump(&fsb->dieselPump, true);
            } else if(fs->isHigh()){
                fsb->pump(&fsb->dieselPump, false);
            } else if(fs->isOverflow() || fs->isError()){
                fsb->halt();
            }
        });

        bilgeLevel.addSwitchListener([](SwitchDevice* device, bool on){
            FloatSwitch* fs = (FloatSwitch*)device;
            FloatSwitches* fsb = (FloatSwitches*)device->Board;
            
            //Serial.print("Bilge level:");
            //Serial.println(fs->getOnFlags());
            if(fs->isHigh()){
                fsb->pump(&fsb->bilgePump, true);
                //pump->turn(true);
            } else if(fs->isLow()){
                //pump->turn(false);
                fsb->pump(&fsb->bilgePump, false);
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
        
    }

    bool FloatSwitches::begin(MessageIO* io){
        if(!CANBusNode::begin(io))return false;

        dieselLevel.setOnFlags(FloatSwitch::FloatLevel::FL_MID);
        return true;
    }

    void FloatSwitches::halt(){
        pump(&dieselPump, false);
        
        resetError.turn(true);
    }

    void FloatSwitches::reset(){
        if(dieselLevel.requiresReset()){
            dieselLevel.reset();
        }
        resetError.turn(false);
    }

    void FloatSwitches::pump(SwitchDevice* pump, bool on){
        pump->turn(on);

        //Activity led on/off
    }

} //end namespace
