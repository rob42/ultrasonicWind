/***********************************************************************/ /**
   \file   ultrasonicWind.ino
   \brief  NMEA2000 base file for ultrasonicWind. Send main data to the bus.


   This base file sends wind data
   to the NMEA2000 bus and exposes it over wifi/html

   Does not fullfill all NMEA2000 requirements.
 */

// #define N2k_SPI_CS_PIN 53    // If you use mcp_can and CS pin is not 53, uncomment this and modify definition to match your CS pin.
// #define N2k_CAN_INT_PIN 21   // If you use mcp_can and interrupt pin is not 21, uncomment this and modify definition to match your interrupt pin.
// #define USE_MCP_CAN_CLOCK_SET 8  // If you use mcp_can and your mcp_can shield has 8MHz chrystal, uncomment this.
#define ESP32_CAN_TX_PIN GPIO_NUM_21 // If you use ESP32 and do not have TX on default IO 16, uncomment this and and modify definition to match your CAN TX pin.
#define ESP32_CAN_RX_PIN GPIO_NUM_20 // If you use ESP32 and do not have RX on default IO 4, uncomment this and and modify definition to match your CAN RX pin.
#define ESP32_MOD_RX_PIN GPIO_NUM_2  // modbus RX
#define ESP32_MOD_TX_PIN GPIO_NUM_0  // modbux TX

#define LED_BLUE 8 // blue LED pin
#include <Arduino.h>
#include <N2kMsg.h>
#include <N2kMessages.h>
#include <Modbus.h>
#include <HardwareSerial.h>
#include <esp_mac.h>
#include <Preferences.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Arduino_JSON.h>
#include <ArduinoOTA.h>
#include "ZenohNode.h"
#include "WifiNode.h"
#include "NMEA2000Node.h"
#include "ModbusNode.h"
#include "WebServer.h"

// modbus
#define MODBUS_SLAVE_ID 1 // default is 0xFF, change via modbus-cli, ~/.local/bin/modbus -s 0 -b 9600 /dev/ttyUSB0 0=1 -v
#define MODBUS_TIMEOUT 50
#define WIND_SPEED_REG 0x000C // Assuming starting register address for wind speed
#define DIRECTION_REG 0x000D  // Assuming starting register address for direction
#define MODE 5                // DE/RE pin, not used
#define DEBUG 0

#define KEYEXPR "demo/example/zenoh-pico-pub"

ZenohNode zenoh;

// web server
//  Replace with your network credentials
const char *ssid = "Foxglove-2g";
const char *password = "foxglove16.4";

unsigned long zenohLastTime = 0;
unsigned long zenohTimerDelay = 1000;

// modbus
HardwareSerial modSerial(1);

ModbusNode modbusNode(modSerial, LED_BLUE);

WifiNode wifiNode(ssid, password);

NMEA2000Node nmea2000Node;

WebServer webServer;

// Define schedulers for messages. Define schedulers here disabled. Schedulers will be enabled
// on OnN2kOpen so they will be synchronized with system.
// We use own scheduler for each message so that each can have different offset and period.
// Setup periods according PGN definition (see comments on IsDefaultSingleFrameMessage and
// IsDefaultFastPacketMessage) and message first start offsets. Use a bit different offset for
// each message so they will not be sent at same time.
tN2kSyncScheduler windScheduler(false, 100, 500);

// *****************************************************************************
// Call back for NMEA2000 open. This will be called, when library starts bus communication.
// See NMEA2000.SetOnOpen(OnN2kOpen); on setup()
void OnN2kOpen()
{
  // Start schedulers now.
  windScheduler.UpdateNextTime();
}

float deAverageAwa()
{
  float diff = modbusNode.awa - modbusNode.last_awa;
  float new_awa = modbusNode.awa + diff;
  if (new_awa >= 360)
    new_awa = new_awa - 360;
  if (new_awa < 0)
    new_awa = new_awa + 360;
  return new_awa;
}


// Initialize LittleFS
void initLittleFS()
{
  if (!LittleFS.begin())
  {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  Serial.println("LittleFS mounted successfully");
}


// Simple message callback matching ZenohMessageCallback
void onZenohMessage(const char *topic, const uint8_t *payload, size_t len)
{
  Serial.print("Received on [");
  Serial.print(topic);
  Serial.print("]: ");

  // Print payload as text (safe only for text payloads)
  for (size_t i = 0; i < len; ++i)
  {
    Serial.write(payload[i]);
  }
  Serial.println();
}

void initZenoh()
{
  if (!zenoh.begin())
  {
    Serial.println("Zenoh setup failed!");
    while (1)
    {
      ;
    }
  }
  // Subscribe to a topic
  if (zenoh.subscribe("sensors/temperature", onZenohMessage))
  {
    Serial.println("Subscribed to sensors/temperature");
  }
  else
  {
    Serial.println("Subscribe failed");
  }
}

void processZenoh()
{
  // Must call poll regularly to let ZenohNode process incoming messages
  zenoh.poll();

  if ((millis() - zenohLastTime) > zenohTimerDelay)
  {
    char payload[64];
    int n = snprintf(payload, sizeof(payload), "uptime=%lu", millis() / 1000);
    // Use the raw-payload publish overload
    if (zenoh.publish("sensors/temperature", (const uint8_t *)payload, (size_t)n))
    {
      Serial.println("Published temperature update");
    }
    else
    {
      Serial.println("Publish failed (node not running?)");
    }
    zenohLastTime = millis();
  }
}



extern "C++" {
  void setup();
  void loop();
}

void setup()
{
  Serial.begin(115200);
  nmea2000Node.init();
  nmea2000Node.setOnOpen(OnN2kOpen);
  modbusNode.init(ESP32_MOD_RX_PIN, ESP32_MOD_TX_PIN, MODE, MODBUS_TIMEOUT);
  nmea2000Node.open();
  wifiNode.init();
  initLittleFS();
  initZenoh();
  webServer.init();
  ArduinoOTA.begin();
}

// *****************************************************************************
void loop()
{
  modbusNode.query(MODBUS_SLAVE_ID, WIND_SPEED_REG, DIRECTION_REG, DEBUG);
  if (windScheduler.IsTime())
  {
    windScheduler.UpdateNextTime();
    nmea2000Node.sendWind(DegToRad(deAverageAwa()),modbusNode.aws_ms);
  }
  webServer.setSensorReadings(deAverageAwa(), modbusNode.aws);
  nmea2000Node.parseMessages();
  nmea2000Node.checkNodeAddress();
  processZenoh();
  webServer.update();
  ArduinoOTA.handle();
}
