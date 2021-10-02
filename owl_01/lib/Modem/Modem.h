#include <Arduino.h>
#include <Settings.h>
#include <System.h>

#ifndef modem_h
#define modem_h

class Modem
{
    public:
        Modem(System s);
        void on();
        void off();
        void wake();
        void sleep();
        void reset();
        void waitTillReady();
    private:
        System sys;
};

#endif