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


bool ZenohNode::begin(const char* locator, const char* mode, const char* keyExpr)
{
  // Initialize Zenoh Session and other parameters
  syslog.print("Initialize Zenoh Session and other parameters...");
    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_config_loan_mut(&config), Z_CONFIG_MODE_KEY, mode);
    if (strcmp(locator, "") != 0) {
        if (strcmp(mode, "client") == 0) {
            zp_config_insert(z_config_loan_mut(&config), Z_CONFIG_CONNECT_KEY, locator);
        } else {
            zp_config_insert(z_config_loan_mut(&config), Z_CONFIG_LISTEN_KEY, locator);
        }
    }
    syslog.println("OK");
    
    // Open Zenoh session
    syslog.print("Opening Zenoh Session...");
    if (z_open(&z_s, z_config_move(&config), NULL) < 0) {
        syslog.println("Unable to open session!");
        return false;
    }
    syslog.println("OK");
    
    syslog.print("Start read and lease tasks...");
    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_session_loan_mut(&z_s), NULL) < 0 || zp_start_lease_task(z_session_loan_mut(&z_s), NULL) < 0) {
        syslog.println("Unable to start read and lease tasks\n");
        z_session_drop(z_session_move(&z_s));
        return false;
    }
    syslog.println("OK");
    
    // Declare Zenoh publisher
    syslog.print("Declaring publisher for ");
    syslog.print(keyExpr);
    syslog.println("...");
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str_unchecked(&ke, keyExpr);
    if (z_declare_publisher(z_session_loan(&z_s), &z_pub, z_view_keyexpr_loan(&ke), NULL) < 0) {
        syslog.println("Unable to declare publisher for key expression!");
        return false;
    }
    syslog.println("OK");
    syslog.println("Zenoh setup finished!");

    delay(300);


  bool ok = initTransport(locator);
  running = ok;
  return ok;
}

void ZenohNode::end()
{
  if (!running) return;

  // Placeholder cleanup logic.
  syslog.println("ZenohNode: shutting down");
  running = false;
}

bool ZenohNode::publish(const char* topic, const char* payloadBuf, size_t len)
{
  if (!running) return false;
  // Replace with actual publish logic.
  syslog.print("ZenohNode: publish to ");
  syslog.print(topic);
  syslog.print(" (");
  syslog.print(len);
  syslog.println(" bytes)");

  z_owned_bytes_t payload;
  z_bytes_copy_from_str(&payload, payloadBuf);

  if (z_publisher_put(z_publisher_loan(&z_pub), z_bytes_move(&payload), NULL) < 0) {
      syslog.println("Error while publishing data");
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
  syslog.print("ZenohNode: subscribed to ");
  syslog.println(topic);
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
  syslog.println("ZenohNode: initializing transport (stub)");
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