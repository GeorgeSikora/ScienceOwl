#include <Arduino.h>

#define DEBUG

#define PIN_ADC_BAT 35
#define PIN_ADC_SOLAR 36
#define ADC_BATTERY_LEVEL_SAMPLES 100

#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 60       /* ESP32 should sleep more seconds  (note SIM7000 needs ~20sec to turn off if sleep is activated) */

/* DHT22 Temp & Humidity sensor */
#define DHTPIN 32
#define DHTTYPE 22

/* SD Card */
#define SD_MISO  2
#define SD_MOSI 15
#define SD_SCLK 14
#define SD_CS   13