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

#define TINY_GSM_MODEM_SIM7000

#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#define DUMP_AT_COMMANDS // Allow to log the AT commands in console

#include <TinyGsmClient.h>

/* custom libraries */
#include <Sensors.h>
#include <Storage.h>
#include <Webserver.h>
#include <WifiNetworks.h>
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

/* Webserver instance */
Webserver ws = Webserver();

/* WifiNetworks instance */
WifiNetworks wns = WifiNetworks();

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

void printDhtStatus();

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

void modem_reset()
{
    Serial.println("Modem hardware reset");
    pinMode(MODEM_RST, OUTPUT);
    digitalWrite(MODEM_RST, LOW);
    delay(260); //Treset 252ms
    digitalWrite(MODEM_RST, HIGH);
    delay(4000); //Modem takes longer to get ready and reply after this kind of reset vs power on

    //modem.factoryDefault();
    //modem.restart(); //this results in +CGREG: 0,0
}

void modem_on()
{
    // Set-up modem  power pin
    pinMode(MODEM_PWKEY, OUTPUT);
    digitalWrite(MODEM_PWKEY, HIGH);
    delay(10);
    digitalWrite(MODEM_PWKEY, LOW);
    delay(1010); //Ton 1sec
    digitalWrite(MODEM_PWKEY, HIGH);

    //wait_till_ready();
    Serial.println("Waiting till modem ready...");
    delay(4510); //Ton uart 4.5sec but seems to need ~7sec after hard (button) reset
    //On soft-reset serial replies immediately.

    Serial.println("Wait...");
    SerialAT.begin(115200, SERIAL_8N1, MODEM_TX, MODEM_RX);
    modem.setBaud(115200);
    modem.begin();
    delay(10000);

    if (!modem.restart()) 
    {
        Serial.println(F(" [fail]"));
        Serial.println(F("************************"));
        Serial.println(F(" Is your modem connected properly?"));
        Serial.println(F(" Is your serial speed (baud rate) correct?"));
        Serial.println(F(" Is your modem powered on?"));
        Serial.println(F(" Do you use a good, stable power source?"));
        Serial.println(
        F(" Try useing File -> Examples -> TinyGSM -> tools -> AT_Debug to find correct configuration"));
        Serial.println(F("************************"));
        delay(10000);
        return;
    }
    Serial.println(F("Step 2: [OK] was able to open modem"));
    String modemInfo = modem.getModemInfo();
    Serial.println("Step 3: Modem details: ");
    Serial.println(modemInfo);

    Serial.println("Waiting for network...");
    if (!modem.waitForNetwork()) 
    {
        Serial.println(" fail");
        delay(10000);
        return;
    }
    Serial.println(" success");

    if (modem.isNetworkConnected()) 
    {
        Serial.println("Network connected");
    }
    Serial.print("Step 4: Waiting for network...");
    if (!modem.waitForNetwork(1200000L)) 
    {
        Serial.println(F(" [fail] while waiting for network"));
        Serial.println(F("************************"));
        Serial.println(F(" Is your sim card locked?"));
        Serial.println(F(" Do you have a good signal?"));
        Serial.println(F(" Is antenna attached?"));
        Serial.println(F(" Does the SIM card work with your phone?"));
        Serial.println(F("************************"));
        delay(10000);
        return;
    }
    Serial.println(F("Found network: [OK]"));

    Serial.print("Step 5: About to set network mode to LTE Only 38: ");
    // Might not be needed for your carrier 
    modem.setNetworkMode(38);

    delay(3000);

    Serial.print("Step 6: About to set network mode: to CAT=M");
    // Might not be needed for your carrier 
    modem.setPreferredMode(3);
    delay(500);

    Serial.print(F("Waiting for network..."));
    if (!modem.waitForNetwork(60000L))
    {
        Serial.println(" fail");
        modem_reset();
        shutdown();
    }
    Serial.println(" OK");

    Serial.print("Signal quality:");
    Serial.println(modem.getSignalQuality());
    delay(3000);

    // GPRS connection parameters are usually set after network registration
    Serial.println("Step 7: Connecting to Rogers APN at LTE Mode Only (channel--> 38): ");
    /*
    if (!modem.gprsConnect(apn, user, pass))
    {
        Serial.println(F(" [fail]"));
        Serial.println(F("************************"));
        Serial.println(F(" Is GPRS enabled by network provider?"));
        Serial.println(F(" Try checking your card balance."));
        Serial.println(F("************************"));
        delay(10000);
        return;
    }

    if (modem.isGprsConnected()) 
    {
        Serial.println(F("Step 8: Connected to network: [OK]"));
        IPAddress local = modem.localIP();
        Serial.print("Step 9: Local IP: ");
        Serial.println(local);
        modem.enableGPS();
        delay(3000);
        String IMEI = modem.getIMEI();
        Serial.print("Step 10: IMEI: ");
        Serial.println(IMEI);
    } 
    else 
    {
        Serial.println(F("Step 8: FAIL NOT Connected to network: "));
    }

    modemConnected = true;
    Serial.println("Modem Connected to Rogers' LTE (channel--> 38) CAT-M (preferred network). TLE CAT-M OK");
    */
}

void modem_off()
{
    //if you turn modem off while activating the fancy sleep modes it takes ~20sec, else its immediate
    Serial.println("Going to sleep now with modem turned off");
    //modem.gprsDisconnect();
    //modem.radioOff();
    modem.sleepEnable(false); // required in case sleep was activated and will apply after reboot
    modem.poweroff();
}

// fancy low power mode - while connected
void modem_sleep() // will have an effect after reboot and will replace normal power down
{
    Serial.println("Going to sleep now with modem in power save mode");
    // needs reboot to activa and takes ~20sec to sleep
    //modem.PSM_mode();    //if network supports will enter a low power sleep PCM (9uA)
    //modem.eDRX_mode14(); // https://github.com/botletics/SIM7000-LTE-Shield/wiki/Current-Consumption#e-drx-mode
    modem.sleepEnable(); //will sleep (1.7mA), needs DTR or PWRKEY to wake
    pinMode(MODEM_DTR, OUTPUT);
    digitalWrite(MODEM_DTR, HIGH);
}

void modem_wake()
{
    Serial.println("Wake up modem from sleep");
    // DTR low to wake serial
    pinMode(MODEM_DTR, OUTPUT);
    digitalWrite(MODEM_DTR, LOW);
    delay(50);
    //wait_till_ready();
}

void wait_till_ready() // NOT WORKING - Attempt to minimize waiting time
{
    for (int8_t i = 0; i < 100; i++) //timeout 100 x 100ms = 10sec
    {
        if (modem.testAT())
        {
            //Serial.println("Wait time:%F sec\n", i/10));
            Serial.printf("Wait time: %d\n", i);
            break;
        }
        delay(100);
    }
}
