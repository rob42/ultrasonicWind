#ifndef WIFINODE_H
#define WIFINODE_H
#include <WiFi.h>
class WifiNode {
  public:
    const char* ssid;
    const char* password;
    WifiNode(const char* ssid, const char* password) : ssid(ssid), password(password) {}

    void onStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
        Serial.println("Connected to AP successfully!");
    }
    void onGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
    }
    void onStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
        Serial.println("Disconnected from WiFi access point");
        Serial.print("WiFi lost connection. Reason: ");
        Serial.println(info.wifi_sta_disconnected.reason);
        Serial.println("Trying to Reconnect");
        WiFi.begin(ssid, password);
        WiFi.setTxPower(WIFI_POWER_15dBm);
    }
    void init() {
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        delay(100);
        Serial.println();
        Serial.print("Scanning networks ");
        int n = WiFi.scanNetworks();
        Serial.println("Done. Printing network list ");
        for (int i = 0; i < n; i++) {
            Serial.print(WiFi.SSID(i));
            Serial.print("  ");
            Serial.println(WiFi.RSSI(i));
        }
        WiFi.scanComplete();
        WiFi.disconnect(true);
        delay(1000);
        WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info){ onStationConnected(event, info); }, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
        WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info){ onGotIP(event, info); }, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
        WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info){ onStationDisconnected(event, info); }, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
        WiFi.begin(ssid, password);
        WiFi.setTxPower(WIFI_POWER_15dBm);
        Serial.print("Connecting to WiFi ..");
    }
};
#endif
