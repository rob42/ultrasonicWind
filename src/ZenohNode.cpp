#include "ZenohNode.h"

z_owned_session_t s;
z_owned_publisher_t pub;
z_owned_subscriber_t sub;
ZenohMessageCallback callback;

ZenohNode::ZenohNode()
  : running(false) //, callback(nullptr)
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
    if (z_open(&s, z_config_move(&config), NULL) < 0) {
        syslog.println("Unable to open session!");
        return false;
    }
    syslog.println("OK");
    
    syslog.print("Start read and lease tasks...");
    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_session_loan_mut(&s), NULL) < 0 || zp_start_lease_task(z_session_loan_mut(&s), NULL) < 0) {
        syslog.println("Unable to start read and lease tasks\n");
        z_session_drop(z_session_move(&s));
        return false;
    }
    syslog.println("OK");
    
    declarePublisher(keyExpr);
    syslog.println("OK");
    syslog.println("Zenoh setup finished!");

    delay(300);


  bool ok = true;
  running = ok;
  return ok;
}

bool ZenohNode::declarePublisher(const char* keyExpr){
  // Declare Zenoh publisher
    syslog.print("Declaring publisher for ");
    syslog.print(keyExpr);
    syslog.println("...");
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str_unchecked(&ke, keyExpr);
    if (z_declare_publisher(z_session_loan(&s), &pub, z_view_keyexpr_loan(&ke), NULL) < 0) {
        syslog.println("Unable to declare publisher for key expression!");
        return false;
    }
    syslog.println("OK");
    return true;
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

  if (z_publisher_put(z_publisher_loan(&pub), z_bytes_move(&payload), NULL) < 0) {
      syslog.println("Error while publishing data");
      return false;
  }
  // Assume publish succeeds.
  return true;
}

bool ZenohNode::publish(const char* topic, const char* payload)
{
  return publish(topic, (const char*)payload, strlen(payload));
}

void ZenohNode::data_handler(z_loaned_sample_t *sample, void *arg) {
    z_view_string_t keystr;
    z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);
    z_owned_string_t value;
    z_bytes_to_string(z_sample_payload(sample), &value);

    syslog.print(" >> [Subscription listener] Received (");
    syslog.print(z_string_data(z_view_string_loan(&keystr)));
    syslog.print(", ");
    syslog.print(z_string_data(z_string_loan(&value)));
    syslog.println(")");
    
   
    callback(z_string_data(z_view_string_loan(&keystr)),  
          z_string_data(z_string_loan(&value)),
          z_string_len(z_string_loan(&value)));

    z_string_drop(z_string_move(&value));
}

bool ZenohNode::subscribe(const char* topic, ZenohMessageCallback cb)
{

  if (!running) return false;
  // Store callback 
    callback = cb;

    // Declare Zenoh subscriber
    syslog.print("Declaring Subscriber on ");
    syslog.print(topic);
    syslog.println(" ...");
    z_owned_closure_sample_t sample;
    
    z_closure_sample(&sample, data_handler, NULL, NULL);
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str_unchecked(&ke, topic);
    if (z_declare_subscriber(z_session_loan(&s), &sub, z_view_keyexpr_loan(&ke), z_closure_sample_move(&sample),
                             NULL) < 0) {
        syslog.println("Unable to declare subscriber.");
        return false;
    }
    syslog.println("OK");
  return true;
}

bool ZenohNode::isRunning() const
{
  return running;
}
