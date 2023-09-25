
#include <esp32-hal-gpio.h>
#include "ADC.h"

#define TIMES              1024
#define SAMPLE_FREQUENCY   80000

#define GET_UNIT(x)        ((x>>3) & 0x1)

#define ADC_RESULT_BYTE     2
#define ADC_CONV_LIMIT_EN   1                       // For ESP32, this should always be set to 1
#define ADC_CONV_MODE       ADC_CONV_SINGLE_UNIT_1  // ESP32 only supports ADC1 DMA mode
#define ADC_OUTPUT_TYPE     ADC_DIGI_OUTPUT_FORMAT_TYPE1

static uint16_t adc1_chan_mask = BIT(6) | BIT(7);
static uint16_t adc2_chan_mask = 0;
static adc_channel_t channel[2] = { ADC_CHANNEL_6, ADC_CHANNEL_7 };

static const char* TAG = "ADC DMA";

static void
continuous_adc_init(
	uint16_t adc1_chan_mask,
	uint16_t adc2_chan_mask,
	adc_channel_t* channel,
	uint8_t channel_num
) {
	adc_digi_init_config_t adc_dma_config = {
		.max_store_buf_size = 4096,
		.conv_num_each_intr = TIMES,
		.adc1_chan_mask = adc1_chan_mask,
		.adc2_chan_mask = adc2_chan_mask,
	};
	ESP_ERROR_CHECK(adc_digi_initialize(&adc_dma_config));

	adc_digi_configuration_t dig_cfg = {
		.conv_limit_en = ADC_CONV_LIMIT_EN,
		.conv_limit_num = 250,
		.sample_freq_hz = SAMPLE_FREQUENCY,
		.conv_mode = ADC_CONV_MODE,
		.format = ADC_OUTPUT_TYPE,
	};

	adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = { 0 };
	dig_cfg.pattern_num = channel_num;
	for (int i = 0; i < channel_num; i++) {
		uint8_t unit = GET_UNIT(channel[i]);
		uint8_t ch = channel[i] & 0x7;
		adc_pattern[i].atten = ADC_ATTEN_DB_11;
		adc_pattern[i].channel = ch;
		adc_pattern[i].unit = unit;
		adc_pattern[i].bit_width = 12;

		ESP_LOGI(TAG, "adc_pattern[%d].atten is :%d", i, adc_pattern[i].atten);
		ESP_LOGI(TAG, "adc_pattern[%d].channel is :%d", i, adc_pattern[i].channel);
		ESP_LOGI(TAG, "adc_pattern[%d].unit is :%d", i, adc_pattern[i].unit);
	}
	dig_cfg.adc_pattern = adc_pattern;
	ESP_ERROR_CHECK(adc_digi_controller_configure(&dig_cfg));
}

esp_err_t ADC::start(uint8_t pin) {
	int8_t channel = digitalPinToAnalogChannel(pin);

	if (channel < 0) {
		log_e("Pin %u is not ADC pin!", pin);
		return -1;
	}

	esp_err_t error_code;

	continuous_adc_init(adc1_chan_mask, adc2_chan_mask, channel, sizeof(channel) / sizeof(adc_channel_t));

	error_code = adc1_config_width(ADC_WIDTH_BIT_12);

	if (error_code != ESP_OK) {
		return error_code;
	}

	adc_digi_start();

	return ESP_OK;
}
