#include <Arduino.h>
#include "AppSerial.h"

void setup() {
	Serial.begin(115200);

	AppSerial::setup();
}

void loop() {
}
