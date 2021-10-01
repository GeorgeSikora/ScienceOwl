#include <Arduino.h>

#define PIN_ADC_BAT 35
#define PIN_ADC_SOLAR 36
#define ADC_BATTERY_LEVEL_SAMPLES 100

void read_adc_bat(uint16_t *voltage);
void read_adc_solar(uint16_t *voltage);