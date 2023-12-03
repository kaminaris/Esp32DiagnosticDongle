#include "ExtADC.h"

ExtADC::ExtADC(uint8_t address, uint8_t rdyPin) {
	readyPin = rdyPin;
	i2cAddress = address;
}

void ExtADC::init() {
	pinMode(readyPin, INPUT);
	attachInterruptArg(digitalPinToInterrupt(readyPin), ExtADC::adcReady, this, RISING);

	ads = new ADS1115(i2cAddress, &Wire1);

	ads->setMode(0);
	ads->setGain(1);	  // 6.144 volt
	ads->setDataRate(7);  // 0 = slow   4 = medium   7 = fast

	// SET ALERT RDY PIN
	ads->setComparatorThresholdHigh(0x8000);
	ads->setComparatorThresholdLow(0x0000);
	ads->setComparatorQueConvert(0);
}

void ExtADC::adcReady(void* instance) {
	((ExtADC*)instance)->conversionReady = true;
}

void ExtADC::toggleConversion(bool toggle) {
	if (toggle) {
		startConversion();
	}
	else {
		stopConversion();
	}
}

void ExtADC::startConversion() {
	if (conversionStarted) {
		return;
	}

	conversionStarted = true;
	currentChannel = 0;
	ads->readADC(0);
	xTaskCreatePinnedToCore(ExtADC::handleConversion, "ConversionTask", 2048, this, 1, &taskPointer, 0);
}

void ExtADC::stopConversion() {
	if (!conversionStarted) {
		return;
	}

	conversionStarted = false;
}

void ExtADC::handleConversion(void* instance) {
	auto extAdc = (ExtADC*)instance;

	while (true) {
		if (extAdc->conversionReady) {
			// Serial.println("RDY");
			extAdc->ads->readADC(extAdc->currentChannel);
			extAdc->values[extAdc->currentChannel] = extAdc->ads->getValue();

			// request next channel
			extAdc->currentChannel++;

			// Finished all channels
			if (extAdc->currentChannel >= numberOfChannels) {
				if (extAdc->readyCallback) {
					extAdc->readyCallback(extAdc->values);
				}

				extAdc->currentChannel = 0;
			}

			extAdc->conversionReady = false;
		}

		if (!extAdc->conversionStarted) {
			break;
		}
	}

	vTaskDelete(nullptr);
}
