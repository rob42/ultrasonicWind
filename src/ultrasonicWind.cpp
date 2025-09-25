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

#include "ultrasonicWind.h"

// web server
//  Replace with your network credentials
const char *ssid = "Foxglove-2g";
const char *password = "foxglove16.4";

unsigned long zenohLastTime = 0;
unsigned long zenohTimerDelay = 1000;

//#define VALUE "[ARDUINO]{ESP32} Publication from Zenoh-Pico!"

PicoSyslog::Logger syslog("wind");
    
ZenohNode zenoh;

// modbus
HardwareSerial modSerial(1);

WindNode windNode(modSerial, LED_BLUE);

WifiNode wifiNode(ssid, password);

NMEA2000Node nmea2000Node;

WebServerNode webServerNode;

// Json Variable to Hold Sensor Readings
JSONVar readings;

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

double deAverageAwa()
{
  double diff = windNode.awa - windNode.last_awa;
  double new_awa = windNode.awa + diff;
  if (new_awa >= 360)
    new_awa = new_awa - 360;
  if (new_awa < 0)
    new_awa = new_awa + 360;
  return new_awa;
}


// Simple message callback matching ZenohMessageCallback
void onZenohMessage(const char *topic, const char *payload, size_t len)
{
  syslog.debug.print("Received on [");
  syslog.debug.print(topic);
  syslog.debug.print("]: ");

  // Print payload as text (safe only for text payloads)
  for (size_t i = 0; i < len; ++i)
  {
    syslog.debug.print(payload[i]);
  }
  syslog.debug.println();
}

void initZenoh()
{
  if (!zenoh.begin(ZENOH_LOCATOR,ZENOH_MODE, KEYEXPR))
  {
    syslog.error.println("Zenoh setup failed!");
    return;
  }
  // Subscribe to a topic
  if (zenoh.subscribe("navigation/courseOverGround", onZenohMessage) 
      && zenoh.subscribe("navigation/speedOverGround", onZenohMessage))
  {
    syslog.information.println("Subscribed to navigation/courseOverGround");
    syslog.information.println("Subscribed to navigation/speedOverGround");
  }
  else
  {
    syslog.error.println("Subscribe failed");
  }
}

void processZenoh()
{

  if ((millis() - zenohLastTime) > zenohTimerDelay)
  {
    readings["environment"]["wind"]["angleApparent"] = DegToRad(deAverageAwa()); 
    readings["environment"]["wind"]["speedApparent"] = windNode.aws_ms; 
    // Use the raw-payload publish overload

    if (!zenoh.publish(KEYEXPR, JSON.stringify(readings).c_str()))
    {
      syslog.error.println("Publish failed (node not running?)");
      if(!zenoh.isRunning()){
        initZenoh();
      }
    }
    zenohLastTime = millis();
  }
}

void initOTA()
{
  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname("wind");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      syslog.information.println("Start updating " + type);
    })
    .onEnd([]() {
      syslog.information.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      syslog.information.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      syslog.error.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) syslog.error.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) syslog.error.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) syslog.error.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) syslog.error.println("Receive Failed");
      else if (error == OTA_END_ERROR) syslog.error.println("End Failed");
    });

  ArduinoOTA.begin();
}

extern "C++" {
  void setup();
  void loop();
}

void setup()
{
  Serial.begin(115200);
  syslog.server = RSYSLOG_IP;
  syslog.default_loglevel = PicoSyslog::LogLevel::debug;

  wifiNode.init();
  //wait for connection
  while(!wifiNode.isConnected() || !wifiNode.ready){
    delay(10);
  }
  delay(1000);
  syslog.print("Wifi connected : ");
  syslog.println(wifiNode.getIP());
  
  initOTA();
//  delay(4000);
  webServerNode.init();

  windNode.init(ESP32_MOD_RX_PIN, ESP32_MOD_TX_PIN, MODE, MODBUS_TIMEOUT);

  nmea2000Node.init();
  nmea2000Node.setOnOpen(OnN2kOpen);
  nmea2000Node.open();

  initZenoh();

}

// *****************************************************************************
void loop()
{
  windNode.query(MODBUS_SLAVE_ID, WIND_SPEED_REG, DIRECTION_REG);
  if (windScheduler.IsTime())
  {
    windScheduler.UpdateNextTime();
    nmea2000Node.sendWind(DegToRad(deAverageAwa()),windNode.aws_ms, false);
  }
  webServerNode.setSensorData("awa",deAverageAwa());
  webServerNode.setSensorData("aws", windNode.aws);
  webServerNode.update();
  nmea2000Node.parseMessages();
  nmea2000Node.checkNodeAddress();
  processZenoh();
  
  ArduinoOTA.handle();
}
