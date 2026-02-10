/***********************************************************************/ /**
   \file   ultrasonicWind.ino
   \brief  NMEA2000 base file for ultrasonicWind. Send main data to the bus.


   
   This base file sends wind data
   to the NMEA2000 bus and exposes it over wifi/html

   Does not fullfill all NMEA2000 requirements.
 */

 #define NODENAME "wind"
// #define N2k_SPI_CS_PIN 53    // If you use mcp_can and CS pin is not 53, uncomment this and modify definition to match your CS pin.
// #define N2k_CAN_INT_PIN 21   // If you use mcp_can and interrupt pin is not 21, uncomment this and modify definition to match your interrupt pin.
// #define USE_MCP_CAN_CLOCK_SET 8  // If you use mcp_can and your mcp_can shield has 8MHz chrystal, uncomment this.
#define ESP32_CAN_TX_PIN GPIO_NUM_21 // If you use ESP32 and do not have TX on default IO 16, uncomment this and and modify definition to match your CAN TX pin.
#define ESP32_CAN_RX_PIN GPIO_NUM_20 // If you use ESP32 and do not have RX on default IO 4, uncomment this and and modify definition to match your CAN RX pin.
#define ESP32_MOD_RX_PIN GPIO_NUM_2  // modbus RX
#define ESP32_MOD_TX_PIN GPIO_NUM_0  // modbux TX

#define KEY_ANGLE_APPARENT "environment/wind/angleApparent" 
#define KEY_SPEED_APPARENT "environment/wind/speedApparent" 

#define LED_BLUE 8 // blue LED pin

#include "ultrasonicWind.h"


// modbus
HardwareSerial modSerial(1);

WindNode windNode(modSerial, LED_BLUE);

NMEA2000Node nmea2000Node;

// Define schedulers for messages. They are declared in base but keep local reference for clarity.
tN2kSyncScheduler windScheduler(false, 100, 500);

// *****************************************************************************
// Call back for NMEA2000 open. This will be called, when library starts bus communication.
// See NMEA2000.SetOnOpen(OnN2kOpen); on setup()
void OnN2kOpen()
{
  // Start schedulers now.
  windScheduler.UpdateNextTime();
}

// Helper copying original deAverageAwa logic but using windNode values
static double deAverageAwa()
{
  double diff = windNode.awa - windNode.last_awa;
  double new_awa = windNode.awa + diff;
  if (new_awa >= 360)
    new_awa = new_awa - 360;
  if (new_awa < 0)
    new_awa = new_awa + 360;
  return new_awa;
}

// Set latest wind data (angle radians, speed m/s)
void setWindData(double angleRad, double speedMs)
{

  // also expose to webserver sensor map in degrees and raw speed
  double angleDeg = angleRad * 180.0 / M_PI;
  webServerNode.setSensorData("awa", angleDeg);
  webServerNode.setSensorData("aws", speedMs * 1.943844); //knots

  //setup values for zenoh
  readings[KEY_ANGLE_APPARENT] = angleRad;
  readings[KEY_SPEED_APPARENT] = speedMs;

}


// *****************************************************************************
void setup()
{
  // Initialize base subsystems (WiFi, OTA, WebServer, Zenoh, Syslog)
  ArduinoOTA.setHostname(NODENAME);
  syslog.app=NODENAME;
  baseInit();

  // initialize wind and nmea nodes
  windNode.init(ESP32_MOD_RX_PIN, ESP32_MOD_TX_PIN, MODE, MODBUS_TIMEOUT);
  const long unsigned transmitMessages[] = {130306L, 0}; // Wind
  nmea2000Node.setTransmitMessages(transmitMessages);
  nmea2000Node.init();
  nmea2000Node.setOnOpen(OnN2kOpen);
  nmea2000Node.open();

  zenoh.declarePublisher(KEY_ANGLE_APPARENT);
  zenoh.declarePublisher(KEY_SPEED_APPARENT);
}

// *****************************************************************************
void loop()
{
  windNode.query(MODBUS_SLAVE_ID, WIND_SPEED_REG, DIRECTION_REG);
  if (windScheduler.IsTime())
  {
    windScheduler.UpdateNextTime();
    double angleRad = DegToRad(deAverageAwa());
    nmea2000Node.sendWind(angleRad, windNode.aws_ms, false);
    // update base with latest wind so Zenoh and webserver can publish it
    setWindData(angleRad, windNode.aws_ms);
  }

  // If not sent this cycle still update webserver with latest raw values
  setWindData(DegToRad(deAverageAwa()), windNode.aws_ms);

  nmea2000Node.parseMessages();
  nmea2000Node.checkNodeAddress();

  // run base periodic tasks (Zenoh publish, OTA, web updates)
  baseLoopTasks();
}
