module;

#include <Arduino.h>

export module TestModule;

export class TestModule {
	public:
	static void prr(uint8_t x) {
		Serial.print(x);
	}
};