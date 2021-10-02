#include <Arduino.h>

#define DEBUG /* Serial console log */
#define FILES_TREE /* SD Card Files Tree chain */

#define DUMP_AT_COMMANDS // Allow to log the AT commands in console

#define TIME_TO_SLEEP 60       /* ESP32 should sleep more seconds  (note SIM7000 needs ~20sec to turn off if sleep is activated) */

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

/* LED pin */
#define LED 12

/* TTGO T-SIM pin definitions */
#define MODEM_RST 5
#define MODEM_PWKEY 4
#define MODEM_DTR 25
#define MODEM_TX 26
#define MODEM_RX 27

/* WiFi Access Point settings */
#define AP_SSID "ScienceOwl"
#define AP_PASSWORD "mostori871"

#define TINY_GSM_MODEM_SIM7000
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb

// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega, Leonardo, Micro
#define SerialAT Serial1

#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */