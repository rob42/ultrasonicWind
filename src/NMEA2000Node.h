#ifndef NMEA2000NODE_H
#define NMEA2000NODE_H
#include <N2kMsg.h>
#include <NMEA2000_CAN.h>
#include <N2kMessages.h>
#include <Preferences.h>
#include <PicoSyslog.h>
class NMEA2000Node
{
public:
    Preferences preferences;
    PicoSyslog::Logger syslog;
    int nodeAddress;

    int seq;

    NMEA2000Node() : seq(0) {}

    int getStoredNodeAddress()
    {
        preferences.begin("nvs", false);
        nodeAddress = preferences.getInt("LastNodeAddress", 23);
        preferences.end();
        syslog.printf("nodeAddress=%d\n", nodeAddress);
        return nodeAddress;
    }
    uint32_t getUniqueId()
    {
        uint8_t chipid[6];
        uint32_t id = 0;
        int i = 0;
        esp_efuse_mac_get_default(chipid);
        for (i = 0; i < 6; i++)
            id += (chipid[i] << (7 * i));
        return id;
    }
    void init()
    {
        syslog.println("Starting NMEA2000 interface");
        NMEA2000.SetProductInformation("00000002", 100, "Simple wind monitor", "1.2.0.24 (2022-10-01)", "1.2.0.0 (2022-10-01)");
        NMEA2000.SetDeviceInformation(getUniqueId(), 130, 85, 140);
        NMEA2000.SetForwardStream(&Serial);
        NMEA2000.SetForwardType(tNMEA2000::fwdt_Text);
        nodeAddress = getStoredNodeAddress();
        syslog.print("NMEA2000 node address = ");
        syslog.println(nodeAddress, DEC);
        NMEA2000.SetMode(tNMEA2000::N2km_ListenAndNode, nodeAddress);
        NMEA2000.EnableForward(true);
        syslog.println("Started OK");
        static const unsigned long transmitMessages[] PROGMEM = {130306L, 0}; // Wind
        NMEA2000.ExtendTransmitMessages(transmitMessages);
        syslog.print("Extended messages : ");
        for(int i = 0; i < sizeof(transmitMessages)/sizeof(long); i++)
            {
            syslog.println(transmitMessages[i]);
            }
    }
    void setOnOpen(void (*onOpenFunc)())
    {
        NMEA2000.SetOnOpen(onOpenFunc);
    }
    void open()
    {
        NMEA2000.Open();
    }
    void parseMessages()
    {
        NMEA2000.ParseMessages();
    }
    void checkNodeAddress()
    {
        int sourceAddress = NMEA2000.GetN2kSource();
        if (sourceAddress != nodeAddress)
        {
            nodeAddress = sourceAddress;
            preferences.begin("nvs", false);
            preferences.putInt("LastNodeAddress", sourceAddress);
            preferences.end();
            syslog.printf("Address Change: New Address=%d\n", sourceAddress);
        }
    }
    void incrementSeq()
    {
        seq++;
        if (seq == 255)
            seq = 1;
    }
    void sendWind( double windAngle, float windSpeed)
    {
        tN2kMsg N2kMsg;
        SetN2kWindSpeed(N2kMsg, seq, windSpeed, windAngle, N2kWind_Apprent);
        if (NMEA2000.SendMsg(N2kMsg))
        {
            syslog.print("Bus ID: ");
            syslog.print(NMEA2000.GetN2kSource());
            syslog.println(" sent n2k message");
        }
        incrementSeq();
    }
};
#endif
