#include <Settings.h>

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
#include <System.h>
#include <Modem.h>
#include <StorageManager.h>
#include <WifiNetworks.h>
#include <WebServer.h>
#include <Sensors.h>
#include <Arduino.h>

/* System instance */
System sys = System();

/* Modem instance */
Modem mod = Modem(sys);

/* StorageManager instance */
StorageManager sm = StorageManager(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);

/* WifiNetworks instance */
WifiNetworks wns = WifiNetworks();

/* WebServer instance */
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

/* target URL link for sending data */
String serverName = "http://192.168.0.101/esp32/test1.php";

struct T_FILE_TREE {
    char* name;
    int size;
    bool isDirectory;
    int parentId;
};

T_FILE_TREE tree[128];
int treeIndex = 0;

void printWakeupReason();

void makeFilesTree(File dir, int parentId);

void shutdown();

void printDhtStatus();

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

    bootCount++;
    
    #ifdef DEBUG
        Serial.println("===== Boot =====");
        Serial.printf("Count: %d\n", bootCount);
        printWakeupReason();
    #endif

    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER)
    {
        mod.wake();
    }
    else
    {
        mod.on();
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

    if (sm.initSD()) 
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
    else 
    {
        #ifdef DEBUG
            Serial.println("○ Not detected...");
        #endif
    }
    
    /***** CONNECT TO WIFI *****/

    unsigned long timeoutTimer = 0;

    wns.load();
    
    #ifdef DEBUG
        Serial.println("===== WiFi =====");
        Serial.printf("Size of stored wifi networks: %d\n", wns.size);
        Serial.println("Trying connect to wifi network points:");
    #endif

    for (int i = 0; i < wns.size; i++)
    {
        WIFI_NETWORK wn = wns.getByIndex(i);

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
                wns.promote(i);
                
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
        wns.print();
    #endif

    wns.save();

    /***** CREATE WIFI ACCESS POINT ****/

    if (WiFi.status() != WL_CONNECTED) // if WiFi is not connected
    {
        WiFi.softAP(AP_SSID, AP_PASSWORD);
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

    //ws.begin();

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
