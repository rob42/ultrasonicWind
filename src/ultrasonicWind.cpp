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

// #define KEY_ANGLE_APPARENT "environment/wind/angleApparent"
// #define KEY_SPEED_APPARENT "environment/wind/speedApparent"

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

bool calculateTrueWind()
{
  if (readings[KEY_ENVIRONMENT_WIND_ANGLEAPPARENT].isNull() || readings[KEY_ENVIRONMENT_WIND_SPEEDAPPARENT].isNull() || readings[KEY_ENVIRONMENT_WIND_SPEEDAPPARENT].as<double>() < 0.1)
    return false;
  if (readings[KEY_NAVIGATION_SPEEDOVERGROUND].isNull())
    return false;

  // ok we have the data
  /*
   * Y = 90 - D
   * a = AW * ( cos Y )
   * bb = AW * ( sin Y )
   * b = bb - BS
   * True-Wind Speed = (( a * a ) + ( b * b )) 1/2
   * True-Wind Angle = 90-arctangent ( b / a )
   */
  double trueDirection = 0.0;
  double trueWindSpeed = 0.0;
  double apparentDir = readings[KEY_ENVIRONMENT_WIND_ANGLEAPPARENT];
  double apparentWnd = readings[KEY_ENVIRONMENT_WIND_SPEEDAPPARENT];
  double vesselSpd = readings[KEY_NAVIGATION_SPEEDOVERGROUND];

  apparentDir = fmod(apparentDir, TWO_PI);
  boolean port = apparentDir > PI;
  if (port)
  {
    apparentDir = TWO_PI - apparentDir;
  }

  /*
   * // Calculate true heading diff and true wind speed - JAVASCRIPT
   * tan_alpha = (Math.sin(angle) / (aspeed - Math.cos(angle)));
   * alpha = Math.atan(tan_alpha);
   *
   * tdiff = rad2deg(angle + alpha);
   * tspeed = Math.sin(angle)/Math.sin(alpha);
   */
  double aspeed = fmax(apparentWnd, vesselSpd);
  if (apparentWnd > 0 && vesselSpd > 0.0)
  {
    aspeed = apparentWnd / vesselSpd;
  }
  double angle = apparentDir;
  double tan_alpha = (sin(angle) / (aspeed - cos(angle)));
  double alpha = atan(tan_alpha);
  double tAngle = alpha + angle;
  if (isnan(tAngle) || isinf(tAngle))
  {
    // no result
    readings[KEY_ENVIRONMENT_WIND_ANGLETRUEGROUND] = trueDirection;
    readings[KEY_ENVIRONMENT_WIND_SPEEDTRUE] = trueWindSpeed;
    return false;
  }
  if (port)
  {
    trueDirection = (TWO_PI - tAngle);
  }
  else
  {
    trueDirection = tAngle;
  }
  readings[KEY_ENVIRONMENT_WIND_ANGLETRUEGROUND] = fmod(trueDirection, TWO_PI);
  //        windCalc[1] = tAngle % TWO_PI;

  if (apparentWnd < 0.1 || vesselSpd < 0.1)
  {
    trueWindSpeed = max(apparentWnd, vesselSpd);
    readings[KEY_ENVIRONMENT_WIND_SPEEDTRUE] = trueWindSpeed;
    return true;
  }
  double tspeed = sin(angle) / sin(alpha);
  if (isnan(tspeed) || isinf(tspeed))
  {
    return true;
  }
  trueWindSpeed = abs(tspeed * vesselSpd);
  readings[KEY_ENVIRONMENT_WIND_SPEEDTRUE] = trueWindSpeed;

  return true;
}
// Set latest wind data (angle radians, speed m/s)
void setWindData(double angleRad, double speedMs)
{

  // also expose to webserver sensor map in degrees and raw speed
  double angleDeg = angleRad * 180.0 / M_PI;
  webServerNode.setSensorData("awa", angleDeg);
  webServerNode.setSensorData("aws", speedMs * 1.943844); // knots

  // setup values for zenoh
  zenoh.publish(KEY_ENVIRONMENT_WIND_ANGLEAPPARENT, angleRad);
  zenoh.publish(KEY_ENVIRONMENT_WIND_SPEEDAPPARENT, speedMs);

  // keep these for true wind calcs later
  readings[KEY_ENVIRONMENT_WIND_ANGLEAPPARENT] = angleRad;
  readings[KEY_ENVIRONMENT_WIND_SPEEDAPPARENT] = speedMs;
  // do true wind
  if (calculateTrueWind())
  {
    zenoh.publish(KEY_ENVIRONMENT_WIND_ANGLETRUEGROUND, readings[KEY_ENVIRONMENT_WIND_ANGLETRUEGROUND].as<double>());
    zenoh.publish(KEY_ENVIRONMENT_WIND_SPEEDTRUE, readings[KEY_ENVIRONMENT_WIND_SPEEDTRUE].as<double>());
  }
}

void handleSog(const char *topic, const char *payload, size_t len)
{
  sprintf("Handling msg: %s =  %s\n", topic, payload);
  readings[KEY_NAVIGATION_SPEEDOVERGROUND] = strtod(payload, NULL);
}

void handleHeadingTrue(const char *topic, const char *payload, size_t len)
{
  sprintf("Handling msg: %s =  %s\n", topic, payload);
  readings[KEY_NAVIGATION_HEADINGTRUE] = strtod(payload, NULL);
}

void handleHeadingMagnetic(const char *topic, const char *payload, size_t len)
{
  sprintf("Handling msg: %s =  %s\n", topic, payload);
  readings[KEY_NAVIGATION_HEADINGMAGNETIC] = strtod(payload, NULL);
}

void handleMagneticDeviation(const char *topic, const char *payload, size_t len)
{
  sprintf("Handling msg: %s =  %s\n", topic, payload);
  readings[KEY_NAVIGATION_MAGNETICDEVIATION] = strtod(payload, NULL);
}

// *****************************************************************************
void setup()
{
  // Initialize base subsystems (WiFi, OTA, WebServer, Zenoh, Syslog)
  ArduinoOTA.setHostname(NODENAME);
  syslog.app = NODENAME;
  baseInit();

  // initialize wind and nmea nodes
  windNode.init(ESP32_MOD_RX_PIN, ESP32_MOD_TX_PIN, MODE, MODBUS_TIMEOUT);
  const long unsigned transmitMessages[] = {130306L, 0}; // Wind
  nmea2000Node.setTransmitMessages(transmitMessages);
  nmea2000Node.init();
  nmea2000Node.setOnOpen(OnN2kOpen);
  nmea2000Node.open();

  zenoh.declarePublisher(KEY_ENVIRONMENT_WIND_ANGLEAPPARENT);
  zenoh.declarePublisher(KEY_ENVIRONMENT_WIND_SPEEDAPPARENT);
  zenoh.subscribe(KEY_NAVIGATION_SPEEDOVERGROUND, handleSog);
  zenoh.subscribe(KEY_NAVIGATION_HEADINGTRUE, handleHeadingTrue);
  zenoh.subscribe(KEY_NAVIGATION_HEADINGMAGNETIC, handleHeadingMagnetic);
  zenoh.subscribe(KEY_NAVIGATION_MAGNETICDEVIATION, handleMagneticDeviation);
}

// *****************************************************************************
void loop()
{
  windNode.query(MODBUS_SLAVE_ID, WIND_SPEED_REG, DIRECTION_REG);
  if (windScheduler.IsTime())
  {
    windScheduler.UpdateNextTime();
    double angleRad = DegToRad(deAverageAwa());
    nmea2000Node.sendWindApparent(angleRad, windNode.aws_ms, false);
    // update base with latest wind so Zenoh and webserver can publish it
    setWindData(angleRad, windNode.aws_ms);
  }else{
    // If not sent this cycle still update webserver with latest raw values
    setWindData(DegToRad(deAverageAwa()), windNode.aws_ms);
  }
  nmea2000Node.parseMessages();
  nmea2000Node.checkNodeAddress();

  // run base periodic tasks (Zenoh publish, OTA, web updates)
  baseLoopTasks();
}
