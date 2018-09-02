#include <SPI.h>
#include "SX1276.h"

////////////////////////////////////////////////////////////////////
#define RADIO_RFM92_95
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// please uncomment only 1 choice
//#define BAND868
#define BAND900
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// CHANGE HERE THE LORA MODE, NODE ADDRESS
#define LORAMODE  4
#define node_addr 0x10
#define gateway_addr 0x01
#define NETWORK_KEY_0 0x01
#define NETWORK_KEY_1 0x01
//////////////////////////////////////////////////////////////////

#define PRINTLN                   Serial.println("")
#define PRINT_CSTSTR(fmt,param)   Serial.print(F(param))
#define PRINT_STR(fmt,param)      Serial.print(param)
#define PRINT_VALUE(fmt,param)    Serial.print(param)
#define FLUSHOUTPUT               Serial.flush();

unsigned long lastTransmissionTime = 0;
unsigned long delayBeforeTransmit = 10000;
uint8_t message[150];

void setup() {
  int e;

  delay(3000);
  // Open serial communications and wait for port to open:
  Serial.begin(9600);

  // Power ON the module
  sx1276.ON();

  // Set transmission mode and print the result
  e = sx1276.setMode(LORAMODE);
  PRINT_CSTSTR("%s", "Setting Mode: state ");
  PRINT_VALUE("%d", e);
  PRINTLN;

  // Setting the network key
  sx1276.setNetworkKey(NETWORK_KEY_0, NETWORK_KEY_1);

#ifdef BAND868
  // Select frequency channel
  e = sx1276.setChannel(CH_10_868);
#else // assuming #defined BAND900
  // Select frequency channel
  e = sx1276.setChannel(CH_05_900);
#endif

  PRINT_CSTSTR("%s", "Setting Channel: state ");
  PRINT_VALUE("%d", e);
  PRINTLN;

  // Select output power (eXtreme, Max, High or Low)
  e = sx1276.setPower('X');

  PRINT_CSTSTR("%s", "Setting Power: state ");
  PRINT_VALUE("%d", e);
  PRINTLN;

  // Set the node address and print the result
  e = sx1276.setNodeAddress(node_addr);
  PRINT_CSTSTR("%s", "Setting node addr: state ");
  PRINT_VALUE("%d", e);
  PRINTLN;

  // Print a success message
  PRINT_CSTSTR("%s", "SX1276 successfully configured\n");

  delay(500);
}

void loop(void) {
  long startSend;
  long endSend;
  int e;

  if (millis() - lastTransmissionTime > delayBeforeTransmit) {

    lastTransmissionTime = millis();

    uint8_t r_size;
    r_size = sprintf((char*)message, "Connected to Medium One");
    //r_size=sprintf((char*)message,"Hello");

    PRINT_CSTSTR("%s", "Sending ");
    PRINT_STR("%s", (char*)(message));
    PRINTLN;

    PRINT_CSTSTR("%s", "Real payload size is ");
    PRINT_VALUE("%d", r_size);
    PRINTLN;

    int pl = r_size;

    startSend = millis();

    e = sx1276.sendPacketTimeout(gateway_addr, message, pl);
    endSend = millis();

    PRINT_CSTSTR("%s", "LoRa pkt size ");
    PRINT_VALUE("%d", pl);
    PRINTLN;

    PRINT_CSTSTR("%s", "LoRa pkt seq ");
    PRINT_VALUE("%d", sx1276.packet_sent.packnum);
    PRINTLN;

    PRINT_CSTSTR("%s", "LoRa Sent in ");
    PRINT_VALUE("%ld", endSend - startSend);
    PRINTLN;

    PRINT_CSTSTR("%s", "Packet sent, state ");
    PRINT_VALUE("%d", e);
    PRINTLN;

    PRINT_VALUE("%ld", delayBeforeTransmit);
    PRINTLN;
  }
  delay(50);
}

