#ifndef ZENOHNODE_H
#define ZENOHNODE_H

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>
#include <zenoh-pico.h>
#include <PicoSyslog.h>


static int idx = 0;

typedef void (*ZenohMessageCallback)(const char* topic, const char* payload, size_t len);


/*
  ZenohNode
  Lightweight Arduino-facing wrapper for a Zenoh node.
  This header declares the renamed class ZenohNode (previously ArduinoZenoh).
*/

class ZenohNode {
public:
  PicoSyslog::Logger syslog;
  ZenohNode();
  ~ZenohNode();

 // void setSyslogIP(const char* ip);
  // Initialize the Zenoh node. Optionally provide a locator/url. mode, and keyExpr
  // Returns true on success.
  bool begin(const char* locator = nullptr,const char* mode = "client",const char* keyExpr = "test/test");

  // Stop the node and free resources.
  void end();

  // Publish a raw payload to a topic.
  // Returns true on success.
  bool publish(const char* topic, const char* payload, size_t len);

  // Convenience overload for null-terminated payloads (strings).
  bool publish(const char* topic, const char* payload);

  // Subscribe to a topic; callback will be invoked for received messages.
  // Returns true on success.
  bool subscribe(const char* topic, ZenohMessageCallback cb);

  // Must be called periodically from loop() to process incoming data.
  void poll();

  // Check whether the node is currently running.
  bool isRunning() const;

private:
  bool running;
  ZenohMessageCallback callback;
  

  // Internal helpers (stubs / placeholders)
  bool initTransport(const char* locator);
  void handleIncoming();
};

#endif // ZENOHNODE_H