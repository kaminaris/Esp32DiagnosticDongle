#include "AppSerial.h"

#include "UARTWrapper/UARTWrapper.h"

extern ExtADC ADS5V;
extern ExtADC ADS20V;
extern ExtADC ADS200V;

NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pTxCharacteristic;
NimBLECharacteristic* pDebugCharacteristic;
NimBLECharacteristic* pCanCharacteristic;
NimBLECharacteristic* pSamplingCharacteristic;
NimBLECharacteristic* pUartCharacteristic;
NimBLECharacteristic* pI2CCharacteristic;
NimBLECharacteristic* pSPICharacteristic;

struct BasicResponse okResponse = {(uint8_t)ResponseCode::OK};
struct BasicResponse failResponse = {(uint8_t)ResponseCode::FAIL};
struct BasicResponse unknownPacketResponse = {(uint8_t)ResponseCode::UNKNOWN_PACKET};
struct ProgressResponse progressResponse = {.r = (uint8_t)ResponseCode::PROGRESS, .progress = 0};

bool canStarted = false;
auto canQueue = xQueueCreate(10, sizeof(CanFramePacket));

UARTWrapper uartWrapper;

bool i2cStarted = false;
auto i2cQueue = xQueueCreate(10, sizeof(I2cDataPacket));

void onReceive(int packetSize) {
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
	xQueueSend(canQueue, &frame, 0);
}

void sendCanQueue(void* p) {
	struct CanFramePacket frame = {};
	while (true) {
		if (xQueueReceive(canQueue, &frame, 0) == pdPASS) {
			pCanCharacteristic->setValue((uint8_t*)&frame, sizeof(frame));
			pCanCharacteristic->notify();
		}

		delay(10);

		if (!canStarted) {
			break;
		}
	}

	vTaskDelete(nullptr);
}

void i2cOnReceive(int bytes) {
	struct I2cDataPacket frame = {.dataLength = 0, .data = {}};
	Serial.printf("Received %d bytes from i2C\n", bytes);

	while (Wire.available()) {
		frame.data[frame.dataLength] = Wire.read();
		frame.dataLength += 1;
	}

	xQueueSend(i2cQueue, &frame, 0);
}

void sendI2CQueue(void* p) {
	struct I2cDataPacket frame = {};
	while (true) {
		if (xQueueReceive(i2cQueue, &frame, 0) == pdPASS) {
			pI2CCharacteristic->setValue((uint8_t*)&frame, sizeof(frame));
			pI2CCharacteristic->notify();
		}

		delay(10);

		if (!i2cStarted) {
			break;
		}
	}

	vTaskDelete(nullptr);
}

void MyCallbacks::onWrite(NimBLECharacteristic* pCharacteristic) {
	auto rxValue = pCharacteristic->getValue();

	if (rxValue.length() > 0) {
		auto* data = (uint8_t*)(rxValue.data() + 1);
		// Serial.println("*********");
		//  Serial.print("Received Value: ");
		//  for (int i = 0; i < rxValue.length(); i++) Serial.print(rxValue[i]);

		auto packetType = rxValue[0];
		// Serial.printf("Received packet: %d", packetType);

		switch (packetType) {
			case PacketType::PIN_CONFIGURE: {
				auto* request = (PinConfigPacket*)data;

				pinMode(request->pin, request->mode);

				AppSerial::respondOk();
				break;
			}

			case PacketType::PIN_WRITE: {
				auto* request = (PinWritePacket*)data;

				digitalWrite(request->pin, request->value);

				AppSerial::respondOk();
				break;
			}

			case PacketType::PIN_READ: {
				auto* request = (PinWritePacket*)data;	// we can use same packet

				auto result = digitalRead(request->pin);

				struct PinReadResult readResponse = {.pin = request->pin, .value = result};

				pTxCharacteristic->setValue((uint8_t*)&readResponse, sizeof(readResponse));
				pTxCharacteristic->notify();
				break;
			}

			case PacketType::PIN_ANALOG_READ: {
				auto* request = (PinWritePacket*)data;	// we can use same packet

				auto result = analogRead(request->pin);

				struct PinReadResult readResponse = {.pin = request->pin, .value = result};

				pTxCharacteristic->setValue((uint8_t*)&readResponse, sizeof(readResponse));
				pTxCharacteristic->notify();
				break;
			}

			case PacketType::CAN_BEGIN: {
				auto* request = (CanBeginPacket*)data;	// we can use same packet

				// Board has CAN on gpio pins on 2 and 4
				CAN.setPins(GPIO_NUM_2, GPIO_NUM_4);
				auto result = CAN.begin(request->baud);
				appSerial.printf("Starting CANBUS baud: %ld, result: %d", request->baud, result);

				CAN.onReceive(onReceive);
				CAN.loopback();

				if (result == 1) {
					xTaskCreatePinnedToCore(sendCanQueue, "CanSend", 2048, nullptr, 0, nullptr, 0);
					canStarted = true;
				}

				AppSerial::respondResult(result);
				break;
			}

			case PacketType::CAN_END: {
				canStarted = false;
				CAN.end();

				AppSerial::respondOk();
				break;
			}

			case PacketType::CAN_SEND: {
				auto* request = (CanFramePacket*)data;
				// appSerial.printf(
				// 	"Sending CANBUS packet id: %ld, size: %d, ext: %d, rtr: %d, rl: %d, tst: %d\n",
				// 	request->id,
				// 	request->packetSize,
				// 	request->extended,
				// 	request->remoteTransmissionRequest,
				// 	request->requestLength,
				// 	sizeof(long)
				// );
				int result;
				if (request->extended) {
					result = CAN.beginExtendedPacket(
						request->id, request->requestLength, request->remoteTransmissionRequest
					);
				}
				else {
					result = CAN.beginPacket(request->id, request->requestLength, request->remoteTransmissionRequest);
				}

				if (result != 1) {
					AppSerial::respondResult(result);
					break;
				}

				for (int i = 0; i < request->packetSize; i++) {
					CAN.write(request->data[i]);
				}

				result = CAN.endPacket();

				AppSerial::respondResult(result);
				break;
			}

			case PacketType::SAMPLE_CONTROL: {
				auto* request = (SamplingControlPacket*)data;

				ADS5V.toggleConversion(request->channel1);
				ADS20V.toggleConversion(request->channel2);
				ADS200V.toggleConversion(request->channel3);

				AppSerial::respondOk();
				break;
			}

			case PacketType::UART_BEGIN: {
				auto request = (UartBeginRequest*)data;
				uartWrapper.begin(request, pUartCharacteristic);

				AppSerial::respondOk();
				break;
			}

			case PacketType::UART_END: {
				uartWrapper.end();

				AppSerial::respondOk();
				break;
			}

			case PacketType::UART_SEND: {
				auto request = (UartDataPacket*)data;
				uartWrapper.send(request);

				AppSerial::respondOk();
				break;
			}

			case PacketType::I2C_BEGIN: {
				if (i2cStarted) {
					i2cStarted = false;
					Wire.end();
					// Allow task to finish before starting up again
					delay(20);
				}

				auto request = (I2cBeginRequest*)data;
				auto success = Wire.begin(21, 22, request->frequency);
				if (success) {
					Wire.onReceive(i2cOnReceive);
					appSerial.printf("Starting I2C frequency: %d", request->frequency);

					xTaskCreatePinnedToCore(sendI2CQueue, "I2CSend", 2048, nullptr, 0, nullptr, 0);
					i2cStarted = true;

					AppSerial::respondOk();
				}
				else {
					AppSerial::respondFail();
				}

				break;
			}

			case PacketType::I2C_END: {
				if (!i2cStarted) {
					AppSerial::respondOk();
					break;
				}
				i2cStarted = false;
				Wire.end();

				AppSerial::respondOk();
				break;
			}

			case PacketType::I2C_SEND: {
				auto request = (I2cDataPacket*)data;
				Serial.printf("Writing I2C addr: %d, bytes: %d\n", request->address, request->dataLength);
				Wire.beginTransmission(request->address);
				Wire.write(request->data, request->dataLength);
				auto result = Wire.endTransmission();

				AppSerial::respondResult(result);
				break;
			}

			case PacketType::I2C_TRANSACTION: {
				auto request = (I2cTransactionPacket*)data;

				Wire.beginTransmission(request->address);
				Wire.write(request->data, request->dataLength);
				Wire.endTransmission(false);

				auto rxCount = Wire.requestFrom(request->address, request->requestLength, true);
				auto readFromBufferCount = Wire.readBytes(request->result, rxCount);

				if (readFromBufferCount != request->requestLength) {
					AppSerial::respondFail();
				}
				else {
					Serial.printf("WIN transaction %d\n", rxCount);
					pTxCharacteristic->setValue((uint8_t*)request, sizeof(I2cTransactionPacket));
					pTxCharacteristic->notify();
				}
				break;
			}

			case PacketType::I2C_SCAN: {
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

				pTxCharacteristic->setValue((uint8_t*)&result, sizeof(result));
				pTxCharacteristic->notify();

				break;
			}

				// System responses

			case PacketType::PING: {
				auto* request = (PingPacket*)data;

				if (request->q == 128) {
					AppSerial::respondOk();
				}
				else {
					AppSerial::respondFail();
				}
				break;
			}

			case PacketType::GET_CHIP_INFO: {
				uint64_t mac = ESP.getEfuseMac();

				String info = "";

				info += "\nChip model: " + String(ESP.getChipModel());
				info += "\nChip cores: " + String(ESP.getChipCores());
				info += "\nChip frequency: " + String(ESP.getCpuFreqMHz()) + "Mhz";
				info += "\nChip version: " + String(ESP.getChipRevision());

				info += "\nRAM size: " + String((ESP.getHeapSize() / 1024.0), 0) + "kB";
				info += "\nPSRAM size: " + String((ESP.getPsramSize() / (1024.0 * 1024.0)), 0) + "MB";

				info += "\nFlash size: " + String((ESP.getFlashChipSize() / (1024.0 * 1024.0)), 0) + "MB";
				info += "\nFlash speed: " + String((ESP.getFlashChipSpeed() / 1000000.0), 0) + "Mhz";

				info += "\nSDK version: " + String(ESP.getSdkVersion());
				info += "\nFirmware size: " + String((ESP.getSketchSize() / (1024.0 * 1024.0)), 0) + "MB";
				info += "\nMAC address: ";

				for (int i = 0; i < 6; i++) {
					if (i > 0) {
						info += "-";
					}
					info += String(byte(mac >> (i * 8) & 0xFF), HEX);
				}

				appSerial.println(info);
				AppSerial::respondOk();
				break;
			}

			case PacketType::FIRMWARE_UPDATE: {
				auto* request = (FirmwareUpdateRequest*)data;

				// auto chunkCrc = CRC32.crc32(request->d, request->size);
				// if (chunkCrc != request->checksum) {
				// 	appSerial.printf(
				// 		"Chunk CRC does not match! Calculated: %08X Given: %08X \n", chunkCrc, request->checksum
				// 	);
				//
				// 	Update.rollBack();
				// 	Update.abort();
				// 	AppSerial::respondFail();
				// 	break;
				// }

				if (request->chunk == 1) {
					if (!Update.begin(request->totalSize, U_FLASH)) {
						appSerial.println("Cannot start flash update!");
						AppSerial::respondFail();
						break;
					}
					Update.write(request->d, request->size);
				}
				else if (request->chunk == request->chunks) {
					Update.write(request->d, request->size);
					if (Update.end()) {
						appSerial.println("Update finished!");
						AppSerial::respondOk();
					}
					else {
						appSerial.printf("Update error: %d\n", Update.getError());
						AppSerial::respondFail();
					}
				}
				else {
					Update.write(request->d, request->size);
				}

				if (request->chunk % 50 == 0) {
					progressResponse.progress =
						static_cast<uint8_t>((request->chunk / static_cast<double>(request->chunks)) * 100.0);

					appSerial.printf(
						"firmware update chunk: %d/%d progress: %d%%\n",
						request->chunk,
						request->chunks,
						progressResponse.progress
					);

					pTxCharacteristic->setValue((uint8_t*)&progressResponse, sizeof(progressResponse));
					pTxCharacteristic->notify();
				}

				// do not respond
				// AppSerial::respondOk();
				break;
			}

			case PacketType::RESTART: {
				delay(1000);
				ESP.restart();
				break;
			}

			default: {
				AppSerial::respondUnknownPacket();
				break;
			}
		}
	}
}

void MyServerCallbacks::onConnect(NimBLEServer* server, ble_gap_conn_desc* desc) {
	// deviceConnected = true;
	appSerial.print("Client address: ");
	appSerial.println(NimBLEAddress(desc->peer_ota_addr).toString().c_str());
	server->updateConnParams(desc->conn_handle, 6, 6, 0, 60);
}

void MyServerCallbacks::onMTUChange(uint16_t MTU, ble_gap_conn_desc* desc) {
	appSerial.printf("MTU updated: %u for connection ID: %u\n", MTU, desc->conn_handle);
}

void MyServerCallbacks::onDisconnect(NimBLEServer* server) {
	// deviceConnected = false;
	NimBLEDevice::startAdvertising();
}

void AppSerial::setup() {
	// Create the BLE Device
	NimBLEDevice::init(BLE_DEV_NAME);
	NimBLEDevice::setPower(ESP_PWR_LVL_P6);
	NimBLEDevice::setMTU(512);

	// Create the BLE Server
	pServer = NimBLEDevice::createServer();
	pServer->setCallbacks(new MyServerCallbacks());

	// Create the BLE Service
	auto pService = pServer->createService(SERVICE_UUID);

	// Create a BLE Characteristic
	pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, NIMBLE_PROPERTY::NOTIFY);
	pDebugCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_DEBUG, NIMBLE_PROPERTY::NOTIFY);
	pCanCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_CAN, NIMBLE_PROPERTY::NOTIFY);
	pSamplingCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_SAMPLING, NIMBLE_PROPERTY::NOTIFY);
	pUartCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_UART, NIMBLE_PROPERTY::NOTIFY);
	pI2CCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_I2C, NIMBLE_PROPERTY::NOTIFY);
	pSPICharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_SPI, NIMBLE_PROPERTY::NOTIFY);

	auto pRxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, NIMBLE_PROPERTY::WRITE_NR);
	pRxCharacteristic->setCallbacks(new MyCallbacks());

	// Start the service
	pService->start();

	// Start the server
	pServer->start();

	NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
	pAdvertising->addServiceUUID(pService->getUUID());
	/** If your device is battery powered you may consider setting scan response
	 *  to false as it will extend battery life at the expense of less data sent.
	 */
	// pAdvertising->setScanResponse(true);
	// Start advertising
	pAdvertising->start();

	appSerial.println("Waiting a client connection to notify...");
}

void AppSerial::respondOk() {
	pTxCharacteristic->setValue((uint8_t*)&okResponse, sizeof(okResponse));
	pTxCharacteristic->notify();
}

void AppSerial::respondFail() {
	pTxCharacteristic->setValue((uint8_t*)&failResponse, sizeof(failResponse));
	pTxCharacteristic->notify();
}

void AppSerial::respondResult(int result) {
	struct ResultResponse response = {.result = result};

	pTxCharacteristic->setValue((uint8_t*)&response, sizeof(response));
	pTxCharacteristic->notify();
}

void AppSerial::respondUnknownPacket() {
	pTxCharacteristic->setValue((uint8_t*)&unknownPacketResponse, sizeof(unknownPacketResponse));
	pTxCharacteristic->notify();
}

size_t AppSerial::write(uint8_t character) {
#ifdef DUAL_SERIAL
	Serial.write(character);
#endif
	if (pDebugCharacteristic == nullptr) {
		return 0;
	}

	pDebugCharacteristic->setValue(&character, 1);
	pDebugCharacteristic->notify();

	return 1;
}

size_t AppSerial::write(const uint8_t* buffer, size_t size) {
#ifdef DUAL_SERIAL
	Serial.write(buffer, size);
#endif
	if (pDebugCharacteristic == nullptr) {
		return 0;
	}

	pDebugCharacteristic->setValue(buffer, size);
	pDebugCharacteristic->notify();

	return size;
}

void AppSerial::notifySampling(const uint8_t* buffer, size_t size) {
	pSamplingCharacteristic->setValue(buffer, size);
	pSamplingCharacteristic->notify();
}

AppSerial appSerial;