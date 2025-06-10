#include <Arduino.h>
#define USE_IRREMOTE_HPP_AS_PLAIN_INCLUDE
#include <IRremote.hpp>
#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>

namespace Chetch{
	class IRTransmitter : public ArduinoDevice {
	public:
		//in millis the time allowed between sends ... a shorter period send is ignored as the ADM should call and set repeat flag
		static const unsigned int SEND_INTERVAL_THRESHOLD = 250; 
		static const byte ERROR_UNRECOGNISED_PROTOCOL = 1;

	private:
		IRsend irSender;
		byte transmitPin = 0; 
		bool sendFlag = false;
		bool repeatFlag = false;
		unsigned int protocol = 0;
		unsigned int address = 0;
		unsigned int command = 0;
		unsigned long lastSend = 0;
		unsigned int repeatCount = 0;

	public:
		IRTransmitter(byte transmitPin);
		
		void configure(byte pin);
		bool begin() override;
        void loop() override;
        bool executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response) override;
            
		void setStatusInfo(ArduinoMessage* message) override;
	};
} //end namespace	