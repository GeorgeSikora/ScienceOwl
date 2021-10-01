#include <Arduino.h>
#include "Sensors.h"

void read_adc_bat(uint16_t *voltage)
{
    uint32_t in = 0;
    for (int i = 0; i < ADC_BATTERY_LEVEL_SAMPLES; i++)
    {
        in += (uint32_t)analogRead(PIN_ADC_BAT);
    }
    in = (int)in / ADC_BATTERY_LEVEL_SAMPLES;

    uint16_t bat_mv = ((float)in / 4096) * 3600 * 2;

    *voltage = bat_mv;
}

void read_adc_solar(uint16_t *voltage)
{
    uint32_t in = 0;
    for (int i = 0; i < ADC_BATTERY_LEVEL_SAMPLES; i++)
    {
        in += (uint32_t)analogRead(PIN_ADC_SOLAR);
    }
    in = (int)in / ADC_BATTERY_LEVEL_SAMPLES;

    uint16_t bat_mv = ((float)in / 4096) * 3600 * 2;

    *voltage = bat_mv;
}