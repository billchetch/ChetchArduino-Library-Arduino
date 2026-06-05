#include "ChetchWaterMonitor.h"

namespace Chetch{
    WaterMonitor::WaterMonitor(byte nodeID, byte serialPin) : CANBusNode(nodeID, serialPin),
                        tds(TDS_ANALOG_PIN, TDS_SAMPLE_INTERVAL),
                        tempArray(TEMP_SENSORS_PIN, TEMP_SENSORS_READ_INTERVAL, TEMP_SENSORS_RESOLUTION),
                        flowMeter1(FLOWMETER1_COUNT_PIN, FlowMeter::FlowRateUnits::LITERS_PER_MINUTE, FLOWMETER_INTERRUPT_MODE)
    {
        /*tds.addSamplingCompleteListener([](AnalogSampler *smpl){
            sendBusMessage(smpl, ArduinoDevice::MESSAGE_ID_REPORT);
        });*/
        tds.setReportInterval(TDS_SAMPLE_INTERVAL);

        tempArray.setReportInterval(TEMP_SENSORS_READ_INTERVAL);
        tempArray.addReadListener([](DS18B20Array *ta, byte sensorCount, float* temps){
            WaterMonitor* wm = (WaterMonitor*)ta->Board;
            if(sensorCount == 1){
                wm->setTemperature(temps[0]);
            }
        });

        flowMeter1.addFlowRateListener([](FlowMeter* fmtr, double flowRate){
            fmtr->enqueueMessageToSend(FlowMeter::MESSAGE_ID_FLOW_RATE);
        });

        //Add devices
        addDevice(&tds); //ID = 10
        addDevice(&tempArray); //ID = 11
        addDevice(&flowMeter1); //ID = 12
    }

} //end namespace
