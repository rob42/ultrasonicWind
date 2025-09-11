#include "ZenohNode.h"

ZenohNode::ZenohNode()
  : running(false), callback(nullptr)
{
}

ZenohNode::~ZenohNode()
{
  end();
}

bool ZenohNode::begin(const char* locator)
{
  // Placeholder initialization logic.
  // Replace with real Zenoh initialization code.
  if (locator && strlen(locator) > 0) {
    // Use locator if provided (debug print for now)
    Serial.print("ZenohNode: initializing with locator: ");
    Serial.println(locator);
  } else {
    Serial.println("ZenohNode: initializing with default configuration");
  }

  bool ok = initTransport(locator);
  running = ok;
  return ok;
}

void ZenohNode::end()
{
  if (!running) return;

  // Placeholder cleanup logic.
  Serial.println("ZenohNode: shutting down");
  running = false;
}

bool ZenohNode::publish(const char* topic, const uint8_t* payload, size_t len)
{
  if (!running) return false;
  // Replace with actual publish logic.
  Serial.print("ZenohNode: publish to ");
  Serial.print(topic);
  Serial.print(" (");
  Serial.print(len);
  Serial.println(" bytes)");
  // For now we assume publish succeeds.
  return true;
}

bool ZenohNode::publish(const char* topic, const char* payload)
{
  return publish(topic, (const uint8_t*)payload, strlen(payload));
}

bool ZenohNode::subscribe(const char* topic, ZenohMessageCallback cb)
{
  if (!running) return false;
  // Store callback and pretend subscription succeeded.
  callback = cb;
  Serial.print("ZenohNode: subscribed to ");
  Serial.println(topic);
  return true;
}

void ZenohNode::poll()
{
  if (!running) return;
  // In a real implementation this would poll the Zenoh library for messages.
  // Here we call handleIncoming() which will invoke the callback if set.
  handleIncoming();
}

bool ZenohNode::isRunning() const
{
  return running;
}

/* Internal placeholder implementations */

bool ZenohNode::initTransport(const char* /*locator*/)
{
  // Replace with actual transport initialization.
  Serial.println("ZenohNode: initializing transport (stub)");
  return true;
}

void ZenohNode::handleIncoming()
{
  // Placeholder that does nothing. If you want to simulate an incoming
  // message for testing, you can call callback here.
  if (callback) {
    // Example: no-op; real code would parse received message and invoke callback.
    // callback("example/topic", (const uint8_t*)"hello", 5);
  }
}