#ifndef MODBUSNODE_H
#define MODBUSNODE_H
#include <Modbus.h>
#include <HardwareSerial.h>
#include <PicoSyslog.h>


// modbus
#define MODBUS_SLAVE_ID 1 // default is 0xFF, change via modbus-cli, ~/.local/bin/modbus -s 0 -b 9600 /dev/ttyUSB0 0=1 -v
#define MODBUS_TIMEOUT 50
#define WIND_SPEED_REG 0x000C // Assuming starting register address for wind speed
#define DIRECTION_REG 0x000D  // Assuming starting register address for direction
#define MODE 5                // DE/RE pin, not used
#define DEBUG 1

extern PicoSyslog::Logger syslog;

class ModbusNode {
public:
    
    HardwareSerial& modSerial;
    Modbus modbus;
    int aws_raw = 0;
    int awa_raw = 0;
    float aws_ms = 0;
    float aws = 0;
    float awa = 0;
    float last_awa = 0;
    long t = 0;
    bool blink = LOW;
    int ledPin;
    ModbusNode(HardwareSerial& serial, int ledPin) : modSerial(serial), modbus(serial), ledPin(ledPin){}

    void init(int rxPin, int txPin, int mode, int timeout) {
        syslog.println("Starting modbus");
        modSerial.begin(9600, SERIAL_8N1, rxPin, txPin);
        modbus.init(mode, false);
        modbus.setTimeout(timeout);
        t = millis();
        pinMode(ledPin, OUTPUT);
        digitalWrite(ledPin, LOW);
        syslog.println("Started modbus ; OK");
    }
    void query(int slaveId, int windSpeedReg, int directionReg, int debug) {
        if ((millis() - t) < 999L) {
            return;
        }
        blink = !blink;
        digitalWrite(ledPin, blink);
        t = millis();
        if (debug) syslog.println("Query modbus");
        if (modbus.requestFrom(slaveId, 0x03, windSpeedReg, 2) > 0) {
            if (debug) syslog.println(" found..");
            aws_raw = modbus.uint16(0);
            aws_ms = aws_raw / 100.0;
            aws = aws_ms * 1.943844;
            awa_raw = modbus.uint16(1);
            last_awa = awa;
            awa = (awa_raw / 10.0);
        } else {
            syslog.println("Modbus read failed, retrying...");
        }
    }
    
};
#endif
