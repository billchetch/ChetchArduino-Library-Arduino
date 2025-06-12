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

			switch (protocol) {
				case SAMSUNG: //20
					irSender.sendSamsung(address, ircommand, 0);
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
				send(message->get<unsigned int>(0), //protocol
					message->get<unsigned int>(1), //address
					message->get<unsigned int>(2)); //ircommand
				
				handled = true;
				break;

		} //end command  switch
			
		return handled;
	}

	bool IRTransmitter::send(unsigned int protocol, unsigned int address, unsigned int ircommand){

		switch (protocol) {  //See IRProtocol.h and IRProtocol.hpp
			case SAMSUNG: //20
				sendFlag = true;
				break;

			default:
				return false;
			}

		this->protocol = protocol;
		this->address = address;
		this->ircommand = ircommand;

		return true;
	}
} //end namespace
