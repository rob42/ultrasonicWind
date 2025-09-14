#ifndef WEBSERVERNODE_H
#define WEBSERVERNODE_H
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Arduino_JSON.h>
#include <functional>
#include <PicoSyslog.h>

extern PicoSyslog::Logger syslog;

class WebServer
{
public:
    
    // Create AsyncWebServer object on port 80
    AsyncWebServer server;

    // Create an Event Source on /events
    AsyncEventSource events;
    // Json Variable to Hold Sensor Readings
    JSONVar readings;

    unsigned long webLastTime = 0;
    unsigned long webTimerDelay = 1000;
    WebServer() : server(80), events("/events") {}
    void init()
    {
        // Initialize LittleFS
        if (!LittleFS.begin())
        {
            syslog.println("An error has occurred while mounting LittleFS");
            return;
        }
        syslog.println("LittleFS mounted successfully");


        syslog.print("Starting webserver...");
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                  { request->send(LittleFS, "/index.html", "text/html"); });
        server.serveStatic("/", LittleFS, "/");
        server.on("/readings", HTTP_GET, [this](AsyncWebServerRequest *request)
                  {
            String json = JSON.stringify(readings);
            request->send(200, "application/json", json); });
        events.onConnect([](AsyncEventSourceClient *client)
                {
            if(client->lastId()) {
                Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
            }
            client->send("hello!", NULL, millis(), 10000); });
        server.addHandler(&events);
        server.begin();
        syslog.println("OK");
    }
    void update()
    {
        if ((millis() - webLastTime) > webTimerDelay)
        {
            events.send("ping", NULL, millis());
            events.send(JSON.stringify(readings).c_str(), "new_readings", millis());
            webLastTime = millis();
        }
    }

    void setSensorReadings(float awa, float aws){
        readings["awa"] = awa; // String(bme.readTemperature());
        readings["aws"] = aws; // String(bme.readHumidity());
    }

};
#endif
