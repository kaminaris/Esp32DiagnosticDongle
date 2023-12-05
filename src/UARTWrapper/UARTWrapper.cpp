module;

#include <Arduino.h>

#include "NimBLECharacteristic.h"

export module UARTWrapper;

export struct __attribute__((packed)) UartDataPacket {
	uint32_t dataLength;
	uint8_t data[256];
};

export struct __attribute__((packed)) UartBeginRequest {
	uint32_t baud;
	uint32_t config;
};

export class UARTWrapper {
	public:
	bool started;
	// NimBLECharacteristic* bleCharacteristic;
	// QueueDefinition* queue;

	UARTWrapper() {
		started = false;
		// bleCharacteristic = nullptr;
		// queue = xQueueGenericCreate(10, sizeof(UartDataPacket), queueQUEUE_TYPE_BASE);
	}

	void begin(UartBeginRequest* request, NimBLECharacteristic* bleChar) {
		// bleCharacteristic = bleChar;
		// if (started) {
		// 	started = false;
		// 	Serial2.end(true);
		// 	// Allow task to finish before starting up again
		// 	delay(20);
		// }
		//
		// // auto request = (UartBeginRequest*)data;
		// Serial2.begin(request->baud, request->config, 16, 17);
		// // auto receiveWrapper = std::bind(&UARTWrapper::onReceive, this);
		// // Serial2.onReceive(receiveWrapper, true);
		// Serial.printf("Starting UART baud: %d", request->baud);
		//
		// xTaskCreatePinnedToCore(sendQueue, "UartSend", 2048, this, 0, nullptr, 0);
		// started = true;
	}

	void onReceive() {
		// size_t available;
		// UartDataPacket frame = {.dataLength = 0, .data = {}};
		//
		// while ((available = Serial2.available())) {
		// 	auto read = Serial2.read(&frame.data[frame.dataLength], available);
		// 	frame.dataLength += read;
		// 	if (read != available) {
		// 		Serial.printf("Failed to read all data %d %d\n", read, available);
		// 		return;
		// 	}
		// }
		//
		// xQueueSend(queue, &frame, 0);
	}

	static void sendQueue(void* p) {
		// auto instance = (UARTWrapper*)p;
		//
		// UartDataPacket frame = {};
		// while (true) {
		// 	if (xQueueReceive(instance->queue, &frame, 0) == pdPASS) {
		// 		instance->bleCharacteristic->setValue((uint8_t*)&frame, sizeof(frame));
		// 		instance->bleCharacteristic->notify();
		// 	}
		//
		// 	delay(10);
		//
		// 	if (!instance->started) {
		// 		break;
		// 	}
		// }
		//
		// vTaskDelete(nullptr);
	}

	void end() {
		// if (!started) {
		// 	return;
		// }
		// started = false;
		// Serial2.end(true);
	}

	void send(UartDataPacket* pPacket) {
		// Serial2.write(pPacket->data, pPacket->dataLength);
	}
};
