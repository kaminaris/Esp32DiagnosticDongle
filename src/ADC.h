#pragma once

#include <hal/adc_types.h>
#include <driver/adc.h>
#include <esp_log.h>

class ADC {
public:
	static esp_err_t start(uint8_t pin);
};
