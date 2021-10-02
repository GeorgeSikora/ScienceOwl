#include <Arduino.h>
#include <Settings.h>

#ifndef storagemanager_h
#define storagemanager_h

class StorageManager
{
    public:
        StorageManager(uint8_t sck, uint8_t miso, uint8_t mosi, uint8_t ss);
        bool initSD();
    private:
        /* SD card SPI pins */
        uint8_t sd_sck;
        uint8_t sd_miso;
        uint8_t sd_mosi; 
        uint8_t sd_ss;
};

#endif