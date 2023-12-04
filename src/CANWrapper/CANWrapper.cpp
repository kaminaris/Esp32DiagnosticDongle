#include "CANWrapper.h"

int CANWrapper::begin(CanBeginPacket* request, NimBLECharacteristic* bleChar) {
	bleCharacteristic = bleChar;

	using namespace std::placeholders;
	// Board has CAN on gpio pins on 2 and 4
	CAN.setPins(2, 4);
	auto result = CAN.begin(request->baud);
	Serial.printf("Starting CANBUS baud: %ld, result: %d", request->baud, result);

	auto receiveWrapper = std::bind(&CANWrapper::onReceive, this, _1);
	CAN.onReceive(receiveWrapper);
	CAN.loopback();

	if (result == 1) {
		xTaskCreatePinnedToCore(sendQueue, "CanSend", 2048, this, 0, nullptr, 0);
		started = true;
	}

	return result;
}

void CANWrapper::onReceive(int packetSize) const {
	struct CanFramePacket frame = {
		.id = CAN.packetId(),
		.extended = CAN.packetExtended(),
		.remoteTransmissionRequest = CAN.packetRtr(),
		.requestLength = CAN.packetDlc(),
		.packetSize = packetSize,
		.data = {0},
	};

	uint8_t pos = 0;
	if (!CAN.packetRtr()) {
		while (CAN.available()) {
			frame.data[pos] = (uint8_t)CAN.read();
			// Serial.print((char)CAN.read());
			pos++;
		}
	}

	xQueueSend(queue, &frame, 0);
}

void CANWrapper::sendQueue(void* p) {
	auto instance = (CANWrapper*)p;

	struct CanFramePacket frame = {};
	while (true) {
		if (xQueueReceive(instance->queue, &frame, 0) == pdPASS) {
			instance->bleCharacteristic->setValue((uint8_t*)&frame, sizeof(frame));
			instance->bleCharacteristic->notify();
		}

		delay(10);

		if (!instance->started) {
			break;
		}
	}

	vTaskDelete(nullptr);
}

void CANWrapper::end() {
	started = false;
	CAN.end();
}

int CANWrapper::send(CanFramePacket* packet) {
	// appSerial.printf(
	// 	"Sending CANBUS packet id: %ld, size: %d, ext: %d, rtr: %d, rl: %d, tst: %d\n",
	// 	packet->id,
	// 	packet->packetSize,
	// 	packet->extended,
	// 	packet->remoteTransmissionpacket,
	// 	packet->packetLength,
	// 	sizeof(long)
	// );
	int result;
	if (packet->extended) {
		result = CAN.beginExtendedPacket(packet->id, packet->requestLength, packet->remoteTransmissionRequest);
	}
	else {
		result = CAN.beginPacket(packet->id, packet->requestLength, packet->remoteTransmissionRequest);
	}

	if (result != 1) {
		return result;
	}

	for (int i = 0; i < packet->packetSize; i++) {
		CAN.write(packet->data[i]);
	}

	result = CAN.endPacket();

	return result;
}
