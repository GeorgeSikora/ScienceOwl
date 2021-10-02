#include <Arduino.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <StreamUtils.h>

#include <WifiNetworks.h>

int WifiNetworks::calcSize()
{
    int index = 0;
    while (wifiNetworks[index].ssid != NULL)
    {
        index++;
    }
    return index;
}

void WifiNetworks::promote(int index)
{
    if (index <= 0 || index >= size) 
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

void WifiNetworks::print()
{
    Serial.println("===== Wifi Networks List =====");

    for (int i = 0; i < size; i++)
    {
        WIFI_NETWORK wn = wifiNetworks[i];

        Serial.printf("[%d] ssid: %s password: %s\n", i, wn.ssid, wn.password);
    }
}

void WifiNetworks::load()
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

    size = array.size();
}

void WifiNetworks::save()
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

    for (int i = 0; i < size; i++)
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

WIFI_NETWORK WifiNetworks::getByIndex(int index)
{
    return wifiNetworks[index];
}