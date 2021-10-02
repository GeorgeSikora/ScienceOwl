#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include <StorageManager.h>

StorageManager::StorageManager(uint8_t sck, uint8_t miso, uint8_t mosi, uint8_t ss)
{
    sd_sck = sck; // sck = Serial Clock
    sd_miso = miso; // miso = Master In Slave Out
    sd_mosi = mosi; // mosi = Master Out Slave In
    sd_ss = ss; // ss/cs = slave select/chip select
}

bool StorageManager::initSD()
{
    SPI.begin(sd_sck, sd_miso, sd_mosi);

    return SD.begin(sd_ss);
}