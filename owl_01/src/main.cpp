/* required libraries */
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <StreamUtils.h>

/* custom libraries */
#include <Sensors.h>
#include <Storage.h>
#include <Webserver.h>
#include <Arduino.h>

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

RTC_DATA_ATTR int bootCount = 0;

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

const char* ssid = "WifiSikora"; 
const char* password = "mostori871";

/* target URL link for sending data */
String serverName = "http://192.168.0.101/esp32/test1.php";

/*

[TODO]: timeoutCount !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

*/
struct WIFI_NETWORK {
    char* ssid;
    char* password;
    long unsigned int connectionCount;
};

int wifiNetworksSize = 3;
WIFI_NETWORK wifiNetworks[8] = {
    { // 0
        ssid: strdup("xdd"),
        password: strdup("lolko"),
        connectionCount: 0,
    },
    { // 1
        ssid: strdup("bobik"),
        password: strdup("awdw"),
        connectionCount: 0,
    },
    { // 2
        ssid: strdup("WifiSikora"),
        password: strdup("mostori871"),
        connectionCount: 0,
    },
};

struct T_FILE_TREE {
    char* name;
    int size;
    bool isDirectory;
    int parentId;
};

T_FILE_TREE tree[128];
int treeIndex = 0;

void shutdown();
void print_wakeup_reason();
void printDirectory(File dir, int parentId);
void getInnerFiles(T_FILE_TREE tree[], File dir, int index);

int getWifiNetworksSize();
void promoteWifiNetwork(int index);
void printWifiNetworks();
void loadWifiNetworks();
void saveWifiNetworks();

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

    Serial.println("===== Promoting Wifi Network =====");

    WIFI_NETWORK target = wifiNetworks[index];
    Serial.println("Target wifi network to promote:");
    Serial.printf("[%d] ssid: %s password: %s\n", index, target.ssid, target.password);

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
        Serial.println("ERROR! File \"wifi_networks.json\" on SD card doesn't exists!");
        return;
    }

    String jsonStr = "";
    while (file.available())
    {
        jsonStr += (char)file.read();
    }
    file.close();

    Serial.println("===== strWifiNetworks =====");
    Serial.printf("Maximum alloc memory: %d\n", ESP.getMaxAllocHeap());
    Serial.printf("WifiNetworks string length: %d\n", jsonStr.length());
    Serial.println(jsonStr);

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
        const long unsigned int connectionCount = obj["connectionCount"];

        wifiNetworks[i].ssid = strdup(ssid);
        wifiNetworks[i].password = strdup(password);
        wifiNetworks[i].connectionCount = connectionCount;
        
        Serial.printf("ssid: %s password: %s connectionCount: %lu \n", ssid, password, connectionCount);

        i++;
    }

    wifiNetworksSize = array.size();
}

void saveWifiNetworks()
{
    Serial.println("===== saveWifiNetworks =====");
    // compute the required size
    const size_t CAPACITY = 1024;

    // allocate the memory for the document
    StaticJsonDocument<CAPACITY> doc;

    // create an empty array
    JsonArray array = doc.to<JsonArray>();

    for (int i = 0; i < wifiNetworksSize; i++)
    {
        Serial.printf("saveWifiNetworks index %d", i);
        JsonObject obj = array.createNestedObject();

        obj["ssid"] = wifiNetworks[i].ssid;
        obj["password"] = wifiNetworks[i].password;
        obj["connectionCount"] = wifiNetworks[i].connectionCount;
    }

    File file = SD.open("/wifi_networks.json", FILE_WRITE);
    WriteLoggingStream loggedFile(file, Serial);
    serializeJson(doc, loggedFile);
    file.close();
}

void setup() 
{
    Serial.begin(115200); 
    Serial.println("Setup...");

    Serial.println("===== SD Card =====");
    SPI.begin(SD_SCLK, SD_MISO, SD_MOSI);

    if (!SD.begin(SD_CS)) 
    {
        Serial.println("○ Not detected...");
    }
    else 
    {
        uint32_t cardSize = SD.cardSize() / (1024 * 1024);
        Serial.printf("● Detected › Size: %d MB\n", cardSize);

        File root = SD.open("/");
        printDirectory(root, 0);

        for(int i = 0; tree[i].name != NULL; i++)
        {
            Serial.printf("[%d] name: %s size: %d isDirectory: %d parentId: %d\n", i, tree[i].name, tree[i].size, tree[i].isDirectory, tree[i].parentId);
        }
    }

    loadWifiNetworks();

    unsigned long timeoutTimer = 0;

    Serial.println("===== WiFi =====");

    wifiNetworksSize = getWifiNetworksSize();
    Serial.printf("Size of stored wifi networks: %d\n", wifiNetworksSize);
    Serial.println("Trying connect to wifi network points:");

    for (int i = 0; i < wifiNetworksSize; i++)
    {
        WIFI_NETWORK wn = wifiNetworks[i];

        timeoutTimer = millis();

        // Try to connect WiFi
        Serial.printf("[%d] ssid: %s password: %s ", i, wn.ssid, wn.password);
        WiFi.begin(wn.ssid, wn.password);

        while (1) 
        {
            if (WiFi.status() == WL_CONNECTED)
            {
                Serial.printf(" Connected!\n");
                promoteWifiNetwork(i);
                break;
            }

            if (millis() > timeoutTimer + 6000)
            {
                Serial.printf(" Connection timeout\n");
                break;
            }

            delay(300);
            Serial.print(".");
        }

        if (WiFi.status() == WL_CONNECTED) break;
    }

    printWifiNetworks();
    saveWifiNetworks();

    // WiFi is not connected
    if (WiFi.status() != WL_CONNECTED)
    {
        // SoftAP WiFi
        WiFi.softAP(ssid, password);
        WiFi.softAPConfig(local_ip, gateway, subnet);

        timeoutTimer = millis();

        while (1)
        {
            // TODO: check  && !clientConnected
            if (millis() > timeoutTimer + 15 * 60000) // 15 minutes timeout
            {
                Serial.printf("\nConnection timeout\n");
                break;
            }
        }
    }

    WiFi.setHostname(hostname.c_str()); //define hostname
    
    Serial.println("");
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());

    Serial.println("===== WebSever =====");
    ws.begin();
    delay(5000);

    Serial.println("===== DHT Sensor =====");
    dht.begin();
    float t = dht.readTemperature(), h = dht.readHumidity();
    if (t != t || h != h)
    {
        Serial.printf("○ Not detected...\n");
    }
    else
    {
        Serial.printf("● Detected › Temp: %.1f°C Hum: %.1f%%\n", dht.readTemperature(), dht.readHumidity());
    }

    pinMode(PIN_ADC_BAT, INPUT);
    pinMode(PIN_ADC_SOLAR, INPUT);
    
    //Increment boot number and print it every reboot
    Serial.println("===== Boot =====");
    ++bootCount;
    Serial.printf("Count: %d\n", bootCount);
    //Print the wakeup reason for ESP32
    print_wakeup_reason();
    /*
    First we configure the wake up source
    We set our ESP32 to wake up every 5 seconds
    */
    Serial.println("===== Sleep timer =====");
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    Serial.printf("Every: %d seconds", TIME_TO_SLEEP);
    
    Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");
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

void printDirectory(File dir, int parentId)
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
            printDirectory(entry, treeIndex-1);
        }

        entry.close();
    }
}