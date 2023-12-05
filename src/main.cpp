#include <Arduino.h>
#include <Wire.h>


import AppSerial;

AppSerial appSerial;

void setup() {
	Serial.begin(115200);
	Serial.println("Booting Diagnostic dongle");
	appSerial.setup();
	Serial.println("Booted!");

}

void loop() {
	// Scanner();
	// delay(100);

	delay(3000);
	//
	// Serial.print("20V rail: ");
	// for (unsigned short value : ADS20V.values) {
	// 	Serial.printf("%.4fv", value / 1269.69f);
	// 	// Serial.printf("%d", value);
	// 	Serial.print('\t');
	// }
	// Serial.println();
	//
	// Serial.print("200V rail: ");
	// for (unsigned short value : ADS200V.values) {
	// 	Serial.printf("%.4fv", value / 139.09f);
	// 	// Serial.printf("%d", value);
	// 	Serial.print('\t');
	// }
	// Serial.println();
	// Serial.println();
}
