#ifndef ULTRASONICWIND_H
#define ULTRASONICWIND_H

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

// zenoh key that is published.
#define KEYEXPR "environment/wind"

#include <Arduino_JSON.h>
#include <ArduinoOTA.h>
#include "ZenohNode.h"
#include "WifiNode.h"
#include "NMEA2000Node.h"
#include "ModbusNode.h"
#include "WebServerNode.h"
#include <PicoSyslog.h>

//extern PicoSyslog::Logger syslog;
extern Preferences preferences;

#endif