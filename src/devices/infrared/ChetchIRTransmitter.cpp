#include "ChetchUtils.h"
#include "ChetchIRTransmitter.h"


namespace Chetch{

	IRTransmitter::IRTransmitter(byte pin) {
		configure(pin);
	}

	void IRTransmitter::configure(byte pin){
        transmitPin = pin;
    }

	bool IRTransmitter::begin(){
		irSender.begin(transmitPin);
		return true;
	}

	void IRTransmitter::setStatusInfo(ArduinoMessage* message){
        ArduinoDevice::setStatusInfo(message);
        
        message->add(transmitPin);
    }

	void IRTransmitter::loop() {
		ArduinoDevice::loop();

		if (sendFlag) {
			if (millis() - lastSend < SEND_INTERVAL_THRESHOLD)return;

			switch (ir2send.protocol) {
				case SAMSUNG: //20
					irSender.sendSamsung(ir2send.address, ir2send.command, 0);
					break;

				default:
					break;

			} //end protocol switch

			sendFlag = false;
			lastSend = millis();

			raiseEvent(EVENT_IRCODESENT);
		}
	}

	bool IRTransmitter::executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response) {
		bool handled = ArduinoDevice::executeCommand(command, message, response);
        if(handled)return true;

		switch (command) {
			case SEND:
				if(send(message->get<unsigned int>(1), //protocol
					message->get<unsigned int>(2), //address
					message->get<unsigned int>(3))){ //command
						return true;
				} else {
					setErrorInfo(response, ERROR_UNRECOGNISED_PROTOCOL);
					return false;
				}

		} //end command  switch
			
		return handled;
	}

	bool IRTransmitter::send(unsigned int protocol, unsigned int address, unsigned int ircommand){
		switch (protocol) {  //See IRProtocol.h and IRProtocol.hpp
			case SAMSUNG: //20
				sendFlag = true;
				break;

			default:
				sendFlag = true; //remove
				break;
			}

		ir2send.protocol = protocol;
		ir2send.address = address;
		ir2send.command = ircommand;

		return true;
	}
} //end namespace
