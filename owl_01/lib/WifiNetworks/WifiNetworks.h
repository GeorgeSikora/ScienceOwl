#include <Arduino.h>
#include <Settings.h>

#ifndef wifinetwork_h
#define wifinetwork_h

struct WIFI_NETWORK {
    char* ssid;
    char* password;
    long unsigned int timeoutCount;
    long unsigned int connectionCount;
};

class WifiNetworks
{
    public:
        WifiNetworks();
        int calcSize();
        void load();
        void save();
        void print();
        void promote(int index);
        WIFI_NETWORK getByIndex(int index);
        int size = 0;
    private:
        WIFI_NETWORK wifiNetworks[8];
};

#endif