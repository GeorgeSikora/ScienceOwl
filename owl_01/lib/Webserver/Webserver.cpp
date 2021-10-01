#include <Arduino.h>

#include "Webserver.h"

/* webserver libraries */
#ifdef ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
#else
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

const char* http_username = "admin";
const char* http_password = "admin";

const char* PARAM_INPUT_1 = "state";

const int output = 2;

AsyncWebServer server(80);

String processor(const String&);
String outputState();

Webserver::Webserver()
{
}

void Webserver::begin()
{
    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        if(!request->authenticate(http_username, http_password))
            return request->requestAuthentication();
        request->send_P(200, "text/html", index_html, processor);
    });
    
    server.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(401);
    });

    server.on("/logged-out", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", logout_html, processor);
    });

    // Send a GET request to <ESP_IP>/update?state=<inputMessage>
    server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {

        if(!request->authenticate(http_username, http_password))
            return request->requestAuthentication();

        String inputMessage;
        String inputParam;
        // GET input1 value on <ESP_IP>/update?state=<inputMessage>
        if (request->hasParam(PARAM_INPUT_1)) 
        {
            inputMessage = request->getParam(PARAM_INPUT_1)->value();
            inputParam = PARAM_INPUT_1;
            digitalWrite(output, inputMessage.toInt());
        }
        else 
        {
            inputMessage = "No message sent";
            inputParam = "none";
        }
        Serial.println(inputMessage);
        request->send(200, "text/plain", "OK");
    });

    server.begin();
}

// Replaces placeholder with button section in your web page
String processor(const String& var)
{
    //Serial.println(var);
    
    if (var == "BUTTONPLACEHOLDER")
    {
        String buttons ="";
        String outputStateValue = outputState();
        buttons+= "<p><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" " + outputStateValue + "><span class=\"slider\"></span></label></p>";
        return buttons;
    }

    if (var == "STATE")
    {
        if(digitalRead(output))
        {
            return "ON";
        }
        else
        {
            return "OFF";
        }
    }
    return String();
}

String outputState()
{
    if (digitalRead(output))
    {
        return "checked";
    }
    else
    {
        return "";
    }
    return "";
}