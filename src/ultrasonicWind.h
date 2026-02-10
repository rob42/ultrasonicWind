#ifndef ULTRASONICWIND_H
#define ULTRASONICWIND_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ZenohNode.h>
#include <WifiNode.h>
#include <NMEA2000Node.h>
#include <WindNode.h>
#include <WebServerNode.h>
#include <PicoSyslog.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <zenohBase.h>

// remote syslog server for logs
#define RSYSLOG_IP "192.168.1.125"
//zenoh

// Peer mode values (comment/uncomment as needed)
#define ZENOH_MODE "peer"
#define ZENOH_LOCATOR "udp/224.0.0.123:7447#iface=eth0" //in peer mode it MUST have #iface=eth0
//scout doesnt work in peer, flips to tcp and crashes.

// Client mode values (comment/uncomment as needed)
//#define ZENOH_MODE "client"
//#define ZENOH_LOCATOR "tcp/192.168.1.125:7447" 
//#define ZENOH_LOCATOR "" // If empty, it will scout

#define KEY_ENVIRONMENT_WIND_ANGLEAPPARENT  "environment/wind/angleApparent"
#define KEY_ENVIRONMENT_WIND_SPEEDAPPARENT  "environment/wind/speedApparent"

//#define KEY_ENVIRONMENT_WIND_ANGLETRUEGROUND  "environment/wind/angleTrueGround"
//#define KEY_ENVIRONMENT_WIND_ANGLETRUEWATER  "environment/wind/angleTrueWater"
//#define KEY_ENVIRONMENT_WIND_SPEEDTRUE  "environment/wind/speedTrue"

#endif