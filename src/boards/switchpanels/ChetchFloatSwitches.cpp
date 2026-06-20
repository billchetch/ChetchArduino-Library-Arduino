#include "ChetchFloatSwitches.h"

namespace Chetch{
    FloatSwitches::FloatSwitches(byte nodeID, byte serialPin) : CANBusNode(nodeID, serialPin),
                        resetSwitch(SwitchDevice::SwitchMode::PASSIVE, RESET_SWITCH_PIN, 100, LOW),
                        dieselLevel(DIESEL_LEVEL_FIRST_PIN, true, true),
                        bilgeLevel(BILGE_LEVEL_FIRST_PIN, false, false),
                        dieselPump(SwitchDevice::SwitchMode::ACTIVE, DIESEL_PUMP_PIN, 100, LOW),
                        bilgePump(SwitchDevice::SwitchMode::ACTIVE, BILGE_PUMP_PIN, 100, LOW)
     {
        resetSwitch.addSwitchListener([](SwitchDevice* device, bool on){
            FloatSwitches* fsb = (FloatSwitches*)device->Board;
            fsb->reset();
        });
        
        dieselLevel.addSwitchListener([](SwitchDevice* device, bool on){
            FloatSwitch* fs = (FloatSwitch*)device;
            FloatSwitches* fsb = (FloatSwitches*)device->Board;
            SwitchDevice* pump = &fsb->dieselPump;
            if(fs->isLow()){
                pump->turn(true);
            } else if(fs->isHigh() || fs->isOverflow()){
                pump->turn(false);
            }
        });

        bilgeLevel.addSwitchListener([](SwitchDevice* device, bool on){
            FloatSwitch* fs = (FloatSwitch*)device;
            FloatSwitches* fsb = (FloatSwitches*)device->Board;
            SwitchDevice* pump = &fsb->bilgePump;
            if(fs->isHigh()){
                pump->turn(true);
            } else if(fs->isLow()){
                pump->turn(false);
            }
        });

        //FloatSwitches* fs = (FloatSwitches*)ta->Board;
            
        //Add devices
        addDevice(&resetSwitch); //ID = 10

        addDevice(&dieselLevel); //ID = 11
        addDevice(&bilgeLevel); //ID = 12

        addDevice(&dieselPump); //ID = 13
        addDevice(&bilgePump); //ID = 14
        
    }

    void FloatSwitches::reset(){
        if(dieselLevel.isOverflow()){
            dieselLevel.reset();
        }
    }

} //end namespace
