#include <ADS1X15.h>
#include <Arduino.h>
#include <Wire.h>

#include "ADC/ExtADC.h"
#include "AppSerial.h"

ExtADC ADS5V(0x48, 14);
ExtADC ADS20V(0x49, 13);
ExtADC ADS200V(0x4A, 12);

/*
void Scanner() {
	Serial.println();
	Serial.println("I2C scanner. Scanning ...");
	byte count = 0;

	Wire.begin();
	for (byte i = 8; i < 120; i++) {
		Wire.beginTransmission(i);	// Begin I2C transmission Address (i)
		// Receive 0 = success (ACK response)
		if (Wire.endTransmission() == 0) {
			Serial.print("Found address: ");
			Serial.print(i, DEC);
			Serial.print(" (0x");
			Serial.print(i, HEX);  // PCF8574 7 bit address
			Serial.println(")");
			count++;
		}
	}
	Serial.print("Found ");
	Serial.print(count, DEC);  // numbers of devices
	Serial.println(" device(s).");
}
*/

uint8_t sampleBuffer[7];
void channelConversionReadyCh(uint8_t rail, uint16_t ch[numberOfChannels]) {
	sampleBuffer[0] = rail;
	memcpy(sampleBuffer + 1, ch, 6);
	appSerial.notifySampling((const uint8_t*)sampleBuffer, 7);
}

void channelConversionReadyCh1(uint16_t ch[numberOfChannels]) {
	channelConversionReadyCh(0, ch);
}
void channelConversionReadyCh2(uint16_t ch[numberOfChannels]) {
	channelConversionReadyCh(1, ch);
}
void channelConversionReadyCh3(uint16_t ch[numberOfChannels]) {
	channelConversionReadyCh(2, ch);
}

void setup() {
	Serial.begin(115200);
	Serial.println("Booting Diagnostic dongle");
	AppSerial::setup();
	Serial.println("Booted!");

	Wire.begin(33, 32);
	ADS5V.init();
	ADS20V.init();
	ADS200V.init();

	// ADS5V.startConversion();
	// ADS20V.startConversion();
	// ADS200V.startConversion();

	ADS5V.readyCallback = channelConversionReadyCh1;
	ADS20V.readyCallback = channelConversionReadyCh2;
	ADS200V.readyCallback = channelConversionReadyCh3;
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
