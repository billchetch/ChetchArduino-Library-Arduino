#include <Arduino.h>
#define USE_IRREMOTE_HPP_AS_PLAIN_INCLUDE
#include <IRremote.hpp>
#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>

namespace Chetch{
	class IRReceiver : public ArduinoDevice {
		public:
			static const byte MESSAGE_ID_IRCODERECEIVED = 200;
			
			static const byte EVENT_START_RECORDING = 1;
			static const byte EVENT_STOP_RECORDING = 2;
			static const byte EVENT_IRCODERECEIVED = 3;

		private:
			byte receivePin;
			IRrecv irReceiver;
			bool recording = false;
		
		public:
			IRReceiver(byte pin);
			
			void configure(byte pin);
			bool begin() override;
        	bool executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response) override;
			void loop() override;

			void setStatusInfo(ArduinoMessage* message) override;
			void populateOutboundMessage(ArduinoMessage* message, byte messageID) override;
  };
} //end namespace	