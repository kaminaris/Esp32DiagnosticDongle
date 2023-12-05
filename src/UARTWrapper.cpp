module;

#include <Arduino.h>

export module UARTWrapper;

export class UARTWrapper {
	public:
	struct __attribute__((packed)) UartDataPacket {
		uint32_t dataLength;
		uint8_t data[256];
	};

	struct __attribute__((packed)) UartBeginRequest {
		uint32_t baud;
		uint32_t config;
	};

	bool started;
	std::function<void(uint8_t*, size_t)> sendCallback;
	QueueDefinition* queue;

	explicit UARTWrapper(std::function<void(uint8_t*, size_t)> sendCb) {
		started = false;
		sendCallback = std::move(sendCb);
		queue = xQueueGenericCreate(10, sizeof(UartDataPacket), queueQUEUE_TYPE_BASE);
	}

	void begin(uint8_t* req, void* bleChar) {
		const auto request = reinterpret_cast<UartBeginRequest*>(req);
		// bleCharacteristic = bleChar;
		if (started) {
			started = false;
			Serial2.end(true);
			// Allow task to finish before starting up again
			delay(20);
		}

		// auto request = (UartBeginRequest*)data;
		Serial2.begin(request->baud, request->config, 16, 17);
		// auto receiveWrapper = std::bind(&UARTWrapper::onReceive, this);
		// Serial2.onReceive(receiveWrapper, true);
		Serial.printf("Starting UART baud: %d", request->baud);

		xTaskCreatePinnedToCore(sendQueue, "UartSend", 2048, this, 0, nullptr, 0);
		started = true;
	}

	void onReceive() {
		size_t available;
		UartDataPacket frame = {.dataLength = 0, .data = {}};

		while ((available = Serial2.available())) {
			auto read = Serial2.read(&frame.data[frame.dataLength], available);
			frame.dataLength += read;
			if (read != available) {
				Serial.printf("Failed to read all data %d %d\n", read, available);
				return;
			}
		}

		xQueueSend(queue, &frame, 0);
	}

	static void sendQueue(void* p) {
		const auto instance = static_cast<UARTWrapper*>(p);

		UartDataPacket frame = {};
		while (true) {
			if (xQueueReceive(instance->queue, &frame, 0) == pdPASS) {
				instance->sendCallback(reinterpret_cast<uint8_t*>(&frame), sizeof(frame));
				// instance->bleCharacteristic->setValue((uint8_t*)&frame, sizeof(frame));
				// instance->bleCharacteristic->notify();
			}

			delay(10);

			if (!instance->started) {
				break;
			}
		}

		vTaskDelete(nullptr);
	}

	void end() {
		if (!started) {
			return;
		}
		started = false;
		Serial2.end(true);
	}

	void send(uint8_t* p) {
		const auto pPacket = reinterpret_cast<UartDataPacket*>(p);
		Serial2.write(pPacket->data, pPacket->dataLength);
	}
};
