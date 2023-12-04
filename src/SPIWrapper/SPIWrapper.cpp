#include "SPIWrapper.h"

void SPIWrapper::begin() {
	if (started) {
		started = false;
		SPI.end();
		// Allow task to finish before starting up again
		delay(20);
	}

	SPI.begin(18, 19, 23, 5);
	pinMode(5, OUTPUT);

	Serial.println("Starting SPI");

	started = true;
}

void SPIWrapper::end() {
	if (!started) {
		return;
	}

	started = false;
	SPI.end();
}

void SPIWrapper::transaction(SPIDataPacket* pPacket) {
	Serial.printf(
		"SPI TR cl: %d bo: %d, dm: %d, l: %d\n",
		pPacket->clock,
		pPacket->bitOrder,
		pPacket->dataMode,
		pPacket->dataLength
	);

	SPI.beginTransaction(SPISettings(pPacket->clock, pPacket->bitOrder, pPacket->dataMode));
	digitalWrite(SPI.pinSS(), LOW);

	for (int i = 0; i < pPacket->dataLength; i++) {
		pPacket->result[i] = SPI.transfer(pPacket->data[i]);
	}

	digitalWrite(SPI.pinSS(), HIGH);
	SPI.endTransaction();

	for (int i = 0; i < pPacket->dataLength; i++) {
		Serial.printf("%d ", pPacket->result[i]);
	}
	Serial.println();
}