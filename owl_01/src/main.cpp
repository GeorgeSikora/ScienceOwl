/* required libraries */
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <StreamUtils.h>

#define TINY_GSM_MODEM_SIM7000

#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#define DUMP_AT_COMMANDS


#include <WiFi.h>
#include <HTTPClient.h>

#include "TinyGsmClient.h"
#include "ThingsBoard.h"

/* custom libraries */
#include <Sensors.h>
#include <Storage.h>
#include <Webserver.h>
#include <Arduino.h>

/* main config */
#define DEBUG /* DEBUG MODE */
//#define FILES_TREE /* SD Card Files Tree chain */

#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 60       /* ESP32 should sleep more seconds  (note SIM7000 needs ~20sec to turn off if sleep is activated) */

/* DHT22 Temp & Humidity sensor */
#define DHTPIN 32
#define DHTTYPE 22

/* LED pin */
#define LED 12

/* SD Card */
#define SD_MISO  2
#define SD_MOSI 15
#define SD_SCLK 14
#define SD_CS   13

/* TTGO T-SIM pin definitions */
#define MODEM_RST 5
#define MODEM_PWKEY 4
#define MODEM_DTR 25
#define MODEM_TX 26
#define MODEM_RX 27

// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega, Leonardo, Micro
#define SerialAT Serial1

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, Serial);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

/* webserver instance */
Webserver ws = Webserver();

/* temp & humidity sensor */
DHT dht(DHTPIN, DHTTYPE);

/* custom name displayed in router DHCP list */
String hostname = "ScienceOwlT7000";

/* Put IP Address details */
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

bool modemConnected = false;

RTC_DATA_ATTR int bootCount = 0;

const char* ssid = "WifiSikora"; 
const char* password = "mostori871";

/* target URL link for sending data */
String serverName = "http://192.168.0.101/esp32/test1.php";

struct WIFI_NETWORK {
    char* ssid;
    char* password;
    long unsigned int timeoutCount;
    long unsigned int connectionCount;
};

WIFI_NETWORK wifiNetworks[8];
int wifiNetworksSize = 0;

struct T_FILE_TREE {
    char* name;
    int size;
    bool isDirectory;
    int parentId;
};

T_FILE_TREE tree[128];
int treeIndex = 0;

void shutdown();
void modem_sleep();
void modem_wake();
void modem_on();
void modem_off();
void wait_till_ready();
void printWakeupReason();

void makeFilesTree(File dir, int parentId);

int getWifiNetworksSize();
void promoteWifiNetwork(int index);
void printWifiNetworks();
void loadWifiNetworks();
void saveWifiNetworks();

void printDhtStatus();

int getWifiNetworksSize()
{
    int index = 0;
    while (wifiNetworks[index].ssid != NULL)
    {
        index++;
    }
    return index;
}

void promoteWifiNetwork(int index)
{
    if (index <= 0 || index >= wifiNetworksSize) 
    {
        return;
    }

    WIFI_NETWORK target = wifiNetworks[index];

    #ifdef DEBUG
        Serial.println("===== Promoting Wifi Network =====");
        Serial.println("Target wifi network to promote:");
        Serial.printf("[%d] ssid: %s password: %s\n", index, target.ssid, target.password);
    #endif

    for (int i = index; i >= 1; i--)
    {
        wifiNetworks[i] = wifiNetworks[i-1];
    }

    wifiNetworks[0] = target;
}

void printWifiNetworks()
{
    Serial.println("===== Wifi Networks List =====");

    for (int i = 0; i < wifiNetworksSize; i++)
    {
        WIFI_NETWORK wn = wifiNetworks[i];

        Serial.printf("[%d] ssid: %s password: %s\n", i, wn.ssid, wn.password);
    }
}

void loadWifiNetworks()
{
    File file = SD.open("/wifi_networks.json");

    if (!file) 
    {
        #ifdef DEBUG
            Serial.println("ERROR! File \"wifi_networks.json\" on SD card doesn't exists!");
        #endif

        return;
    }

    String jsonStr = "";
    while (file.available())
    {
        jsonStr += (char)file.read();
    }
    file.close();

    #ifdef DEBUG
        Serial.println("===== strWifiNetworks =====");
        Serial.printf("Maximum alloc memory: %d\n", ESP.getMaxAllocHeap());
        Serial.printf("WifiNetworks string length: %d\n", jsonStr.length());
        Serial.println(jsonStr);
    #endif

    // compute the required size
    const size_t CAPACITY = 1024;

    // allocate the memory for the document
    StaticJsonDocument<CAPACITY> doc;

    // parse a JSON array
    deserializeJson(doc, jsonStr);

    // extract the values
    JsonArray array = doc.as<JsonArray>();
    int i = 0;
    for (JsonVariant v : array) 
    {
        JsonObject obj = v.as<JsonObject>();

        const char* ssid = obj["ssid"];
        const char* password = obj["password"];
        const long unsigned int timeoutCount = obj["timeoutCount"];
        const long unsigned int connectionCount = obj["connectionCount"];

        wifiNetworks[i].ssid = strdup(ssid);
        wifiNetworks[i].password = strdup(password);
        wifiNetworks[i].timeoutCount = timeoutCount;
        wifiNetworks[i].connectionCount = connectionCount;
        
        #ifdef DEBUG
            Serial.printf("ssid: %s password: %s timeoutCount: %lu connectionCount: %lu \n", ssid, password, timeoutCount, connectionCount);
        #endif

        i++;
    }

    wifiNetworksSize = array.size();
}

void saveWifiNetworks()
{
    #ifdef DEBUG
        Serial.println("===== Saving WiFi Networks =====");
    #endif

    // compute the required size
    const size_t CAPACITY = 1024;

    // allocate the memory for the document
    StaticJsonDocument<CAPACITY> doc;

    // create an empty array
    JsonArray array = doc.to<JsonArray>();

    for (int i = 0; i < wifiNetworksSize; i++)
    {
        JsonObject obj = array.createNestedObject();

        obj["ssid"] = wifiNetworks[i].ssid;
        obj["password"] = wifiNetworks[i].password;
        obj["connectionCount"] = wifiNetworks[i].connectionCount;
    }

    File file = SD.open("/wifi_networks.json", FILE_WRITE);
    WriteLoggingStream loggedFile(file, Serial);
    serializeJson(doc, loggedFile);
    file.close();

    #ifdef DEBUG
        Serial.println("Successfully saved on SD card");
    #endif
}

void printDhtStatus()
{
    float t = dht.readTemperature(), h = dht.readHumidity();

    if (t != t || h != h)
    {
        Serial.printf("○ Not detected...\n");
    }
    else
    {
        Serial.printf("● Detected › Temp: %.1f°C Hum: %.1f%%\n", dht.readTemperature(), dht.readHumidity());
    }
}

void setup() 
{
    /***** SERIAL COMMUNICATION ****/
    
    #ifdef DEBUG
        Serial.begin(115200); 
        Serial.println(".......... setup ..........");
    #endif
    
    /***** PINS SETUP *****/

    pinMode(PIN_ADC_BAT, INPUT);
    pinMode(PIN_ADC_SOLAR, INPUT);
    
    /***** BOOT INIT *****/

    ++bootCount;
    
    #ifdef DEBUG
        Serial.println("===== Boot =====");
        Serial.printf("Count: %d\n", bootCount);
        printWakeupReason();
    #endif

    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER)
    {
        modem_wake();
    }
    else
    {
        modem_on();
        Serial.println(" > Modem On --> True");
        //modem_wake();
        //modem_reset();
    }

    /***** SLEEP TIMER *****/
    
    #ifdef DEBUG
        Serial.println("===== Sleep timer =====");
        Serial.printf("Every: %d seconds", TIME_TO_SLEEP);
    #endif

    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

    /***** SD CARD *****/

    #ifdef DEBUG
        Serial.println("===== SD Card =====");
    #endif

    SPI.begin(SD_SCLK, SD_MISO, SD_MOSI);

    if (!SD.begin(SD_CS)) 
    {
        #ifdef DEBUG
            Serial.println("○ Not detected...");
        #endif
    }
    else 
    {
        #ifdef DEBUG
            uint32_t cardSize = SD.cardSize() / (1024 * 1024);
            Serial.printf("● Detected › Size: %d MB\n", cardSize);
        #endif

        #ifdef FILES_TREE
            File root = SD.open("/");

            makeFilesTree(root, 0);

            for(int i = 0; tree[i].name != NULL; i++)
            {
                Serial.printf("[%d] name: %s size: %d isDirectory: %d parentId: %d\n", i, tree[i].name, tree[i].size, tree[i].isDirectory, tree[i].parentId);
            }
        #endif
    }
    
    /***** CONNECT TO WIFI *****/

    unsigned long timeoutTimer = 0;

    loadWifiNetworks();
    
    #ifdef DEBUG
        Serial.println("===== WiFi =====");
        Serial.printf("Size of stored wifi networks: %d\n", wifiNetworksSize);
        Serial.println("Trying connect to wifi network points:");
    #endif

    for (int i = 0; i < wifiNetworksSize; i++)
    {
        WIFI_NETWORK wn = wifiNetworks[i];

        timeoutTimer = millis();

        #ifdef DEBUG
            Serial.printf("[%d] ssid: %s password: %s ", i, wn.ssid, wn.password);
        #endif

        // Try to connect WiFi
        WiFi.begin(wn.ssid, wn.password);

        while (1) 
        {
            if (WiFi.status() == WL_CONNECTED)
            {
                wn.connectionCount++;
                promoteWifiNetwork(i);
                
                #ifdef DEBUG
                    Serial.printf(" Connected!\n");
                #endif

                break;
            }

            if (millis() > timeoutTimer + 6000)
            {
                wn.timeoutCount++;

                #ifdef DEBUG
                    Serial.printf(" Connection timeout\n");
                #endif
                break;
            }

            delay(300);
            Serial.print(".");
        }

        if (WiFi.status() == WL_CONNECTED) break;
    }

    #ifdef DEBUG
        printWifiNetworks();
    #endif

    saveWifiNetworks();

    /***** CREATE WIFI ACCESS POINT ****/

    if (WiFi.status() != WL_CONNECTED) // if WiFi is not connected
    {
        WiFi.softAP(ssid, password);
        WiFi.softAPConfig(local_ip, gateway, subnet);

        timeoutTimer = millis();

        while (1)
        {
            // TODO: check  && !clientConnected
            if (millis() > timeoutTimer + 15 * 60000) // 15 minutes timeout
            {
                #ifdef DEBUG
                    Serial.printf("\nConnection timeout\n");
                #endif
                break;
            }
        }
    }

    /***** WIFI CONFIG & INFO *****/

    WiFi.setHostname(hostname.c_str()); //define hostname

    #ifdef DEBUG
        Serial.println("");
        Serial.print("Connected to WiFi network with IP Address: ");
        Serial.println(WiFi.localIP());
    #endif

    /***** WEBSERVER *****/
    #ifdef DEBUG
        Serial.println("===== WebSever =====");
    #endif

    ws.begin();

    /***** DHT SENSOR ****/

    dht.begin();

    #ifdef DEBUG
        Serial.println("===== DHT Sensor =====");
        printDhtStatus();
    #endif
}

void loop() 
{
    uint16_t v_bat = 0;
    uint16_t v_solar = 0;
    
    float hum = dht.readHumidity();
    float temp = dht.readTemperature();
    
    read_adc_bat(&v_bat);
    read_adc_solar(&v_solar);
    
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;

        String serverPath = serverName + "?ubat=" + v_bat + "&usol=" + v_solar +"&temp=" + temp + "&hum=" + hum;
        
        // Your Domain name with URL path or IP address with path
        http.begin(serverPath.c_str());
        
        // Send HTTP GET request
        int httpResponseCode = http.GET();
        
        if (httpResponseCode > 0) 
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
    else 
    {
        Serial.println("WiFi Disconnected");
        delay(1000);
    }
}

void shutdown()
{
    //modem_sleep();
    //modem_off(); // <-- TOTO

    #ifdef DEBUG
        Serial.println("Going to sleep now");
        Serial.flush();
    #endif

    esp_deep_sleep_start();
}

void printWakeupReason()
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

void makeFilesTree(File dir, int parentId)
{
    while (true) 
    {
        File entry =  dir.openNextFile();

        if (!entry) break;

        char* filePath = (char*)(entry.name());
        char* fileName = strrchr(filePath, '/') + 1;
        bool isDirectory = entry.isDirectory();

        tree[treeIndex].name = strdup(fileName);
        tree[treeIndex].isDirectory = isDirectory;
        tree[treeIndex].size = entry.size();
        tree[treeIndex].parentId = parentId;
        treeIndex++;

        if (isDirectory) 
        {
            makeFilesTree(entry, treeIndex-1);
        }

        entry.close();
    }
}