#include <Arduino.h>
#include <Modem.h>
#include <TinyGsmClient.h>

#ifdef DUMP_AT_COMMANDS
    #include <StreamDebugger.h>
    StreamDebugger debugger(SerialAT, Serial);
    TinyGsm modem(debugger);
#else
    TinyGsm modem(SerialAT);
#endif

Modem::Modem(System s)
{
    sys = s;
}

void Modem::on()
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
        reset();
        sys.shutdown();
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

void Modem::off()
{
    //if you turn modem off while activating the fancy sleep modes it takes ~20sec, else its immediate
    Serial.println("Going to sleep now with modem turned off");
    //modem.gprsDisconnect();
    //modem.radioOff();
    modem.sleepEnable(false); // required in case sleep was activated and will apply after reboot
    modem.poweroff();
}

void Modem::wake()
{
    Serial.println("Wake up modem from sleep");
    // DTR low to wake serial
    pinMode(MODEM_DTR, OUTPUT);
    digitalWrite(MODEM_DTR, LOW);
    delay(50);
    //wait_till_ready();
}

// fancy low power mode - while connected
void Modem::sleep() // will have an effect after reboot and will replace normal power down
{
    Serial.println("Going to sleep now with modem in power save mode");
    // needs reboot to activa and takes ~20sec to sleep
    //modem.PSM_mode();    //if network supports will enter a low power sleep PCM (9uA)
    //modem.eDRX_mode14(); // https://github.com/botletics/SIM7000-LTE-Shield/wiki/Current-Consumption#e-drx-mode
    modem.sleepEnable(); //will sleep (1.7mA), needs DTR or PWRKEY to wake
    pinMode(MODEM_DTR, OUTPUT);
    digitalWrite(MODEM_DTR, HIGH);
}

void Modem::reset()
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

void Modem::waitTillReady() // NOT WORKING - Attempt to minimize waiting time
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
