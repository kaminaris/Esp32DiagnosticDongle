#include "I2CWrapper.h"

template <typename T>
struct Callback;

template <typename Ret, typename... Params>
struct Callback<Ret(Params...)> {
	template <typename... Args>
	static Ret callback(Args... args) {
		return func(args...);
	}
	static std::function<Ret(Params...)> func;
};

template <typename Ret, typename... Params>
std::function<Ret(Params...)> Callback<Ret(Params...)>::func;

int I2CWrapper::begin(I2cBeginRequest* request, NimBLECharacteristic* bleChar) {
	bleCharacteristic = bleChar;

	if (started) {
		started = false;
		Wire.end();
		// Allow task to finish before starting up again
		delay(20);
	}

	using namespace std::placeholders;
	auto success = Wire.begin(21, 22, request->frequency);

	if (success) {
		Callback<void(int)>::func = std::bind(&I2CWrapper::onReceive, this, _1);
		auto func = static_cast<WireCallback>(Callback<void(int)>::callback);
		Wire.onReceive(func);
		Serial.printf("Starting I2C frequency: %d", request->frequency);

		xTaskCreatePinnedToCore(sendQueue, "I2CSend", 2048, this, 0, nullptr, 0);
		started = true;

		return ESP_OK;
	}

	return ESP_FAIL;
}
void I2CWrapper::onReceive(int bytes) {
	struct I2cDataPacket frame = {.dataLength = 0, .data = {}};
	Serial.printf("Received %d bytes from i2C\n", bytes);

	while (Wire.available()) {
		frame.data[frame.dataLength] = Wire.read();
		frame.dataLength += 1;
	}

	xQueueSend(queue, &frame, 0);
}

void I2CWrapper::sendQueue(void* p) {
	auto instance = (I2CWrapper*)p;

	struct I2cDataPacket frame = {};
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

void I2CWrapper::end() {
	if (!started) {
		return;
	}

	started = false;
	Wire.end();
}

uint8_t I2CWrapper::send(I2cDataPacket* request) {
	Serial.printf("Writing I2C addr: %d, bytes: %d\n", request->address, request->dataLength);
	Wire.beginTransmission(request->address);
	Wire.write(request->data, request->dataLength);
	return Wire.endTransmission();
}

int I2CWrapper::transaction(I2cTransactionPacket* request) {
	Wire.beginTransmission(request->address);
	Wire.write(request->data, request->dataLength);
	Wire.endTransmission(false);

	auto rxCount = Wire.requestFrom(request->address, request->requestLength, true);
	auto readFromBufferCount = Wire.readBytes(request->result, rxCount);

	if (readFromBufferCount != request->requestLength) {
		return ESP_FAIL;
	}
	else {
		return ESP_OK;
	}
}

I2CScanResult I2CWrapper::scan() {
	Serial.println("I2C scanner. Scanning ...");
	struct I2CScanResult result = {0};

	for (uint8_t i = 8; i < 120; i++) {
		Wire.beginTransmission(i);
		if (Wire.endTransmission() == 0) {
			Serial.printf("Found address: %d\n", i);
			result.addresses[result.found] = i;
			result.found++;
		}
	}

	return result;
}
