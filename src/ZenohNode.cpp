#include "ZenohNode.h"

z_owned_session_t z_s;
z_owned_publisher_t z_pub;

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
  // Initialize Zenoh Session and other parameters
    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_config_loan_mut(&config), Z_CONFIG_MODE_KEY, ZENOH_MODE);
    if (strcmp(LOCATOR, "") != 0) {
        if (strcmp(ZENOH_MODE, "client") == 0) {
            zp_config_insert(z_config_loan_mut(&config), Z_CONFIG_CONNECT_KEY, LOCATOR);
        } else {
            zp_config_insert(z_config_loan_mut(&config), Z_CONFIG_LISTEN_KEY, LOCATOR);
        }
    }

    // Open Zenoh session
    Serial.print("Opening Zenoh Session...");
    if (z_open(&z_s, z_config_move(&config), NULL) < 0) {
        Serial.println("Unable to open session!");
        return false;
    }
    Serial.println("OK");

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_session_loan_mut(&z_s), NULL) < 0 || zp_start_lease_task(z_session_loan_mut(&z_s), NULL) < 0) {
        Serial.println("Unable to start read and lease tasks\n");
        z_session_drop(z_session_move(&z_s));
        return false;
    }

    // Declare Zenoh publisher
    Serial.print("Declaring publisher for ");
    Serial.print(KEYEXPR);
    Serial.println("...");
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str_unchecked(&ke, KEYEXPR);
    if (z_declare_publisher(z_session_loan(&z_s), &z_pub, z_view_keyexpr_loan(&ke), NULL) < 0) {
        Serial.println("Unable to declare publisher for key expression!");
        return false;
    }
    Serial.println("OK");
    Serial.println("Zenoh setup finished!");

    delay(300);


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

bool ZenohNode::publish(const char* topic, const char* payloadBuf, size_t len)
{
  if (!running) return false;
  // Replace with actual publish logic.
  Serial.print("ZenohNode: publish to ");
  Serial.print(topic);
  Serial.print(" (");
  Serial.print(len);
  Serial.println(" bytes)");

  z_owned_bytes_t payload;
  z_bytes_copy_from_str(&payload, payloadBuf);

  if (z_publisher_put(z_publisher_loan(&z_pub), z_bytes_move(&payload), NULL) < 0) {
      Serial.println("Error while publishing data");
  }

  // For now we assume publish succeeds.
  return true;
}

bool ZenohNode::publish(const char* topic, const char* payload)
{
  return publish(topic, (const char*)payload, strlen(payload));
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