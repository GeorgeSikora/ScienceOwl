#include <WiFi.h>
#include <HTTPClient.h>

#define PIN_ADC_BAT 35
#define PIN_ADC_SOLAR 36
#define ADC_BATTERY_LEVEL_SAMPLES 100

#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 60       /* ESP32 should sleep more seconds  (note SIM7000 needs ~20sec to turn off if sleep is activated) */
RTC_DATA_ATTR int bootCount = 0;

const char* ssid = "WifiSikora";
const char* password = "mostori871";

//Your Domain name with URL path or IP address with path
String serverName = "http://192.168.0.101/esp32/test1.php";

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 5000;

void shutdown();
void print_wakeup_reason();
void read_adc_bat(uint16_t *voltage);
void read_adc_solar(uint16_t *voltage);

void setup() {
  Serial.begin(115200); 
  pinMode(PIN_ADC_BAT, INPUT);
  pinMode(PIN_ADC_SOLAR, INPUT);

  
  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));
  //Print the wakeup reason for ESP32
  print_wakeup_reason();
  /*
  First we configure the wake up source
  We set our ESP32 to wake up every 5 seconds
  */
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");


  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
 
  Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");
}

void loop() {

  uint16_t v_bat = 0;
  uint16_t v_solar = 0;
  
  read_adc_bat(&v_bat);
  read_adc_solar(&v_solar);
  /*
  read_adc_bat(&v_bat);
  Serial.print("BAT: ");
  Serial.print(v_bat);

  read_adc_solar(&v_solar);
  Serial.print(" SOLAR: ");
  Serial.println(v_solar);
  */

  
  //Send an HTTP POST request every 10 minutes
  if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      HTTPClient http;

      String serverPath = serverName + "?ubat=" + v_bat + "&usol=" + v_solar;
      
      // Your Domain name with URL path or IP address with path
      http.begin(serverPath.c_str());
      
      // Send HTTP GET request
      int httpResponseCode = http.GET();
      
      if (httpResponseCode>0) 
      {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
      }
      else 
      {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      // Free resources
      http.end();
      shutdown();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
}

void shutdown()
{

  //modem_sleep();
  //modem_off(); // <-- TOTO

  delay(1000);
  Serial.flush();
  esp_deep_sleep_start();
  Serial.println("Going to sleep now");
  delay(1000);
  Serial.flush();
  esp_deep_sleep_start();
}

void print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("Wakeup caused by external signal using RTC_IO");
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    Serial.println("Wakeup caused by external signal using RTC_CNTL");
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Wakeup caused by timer");
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("Wakeup caused by touchpad");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    Serial.println("Wakeup caused by ULP program");
    break;
  default:
    Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
    break;
  }
}

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
