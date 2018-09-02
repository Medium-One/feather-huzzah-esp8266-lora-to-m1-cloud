
// Include the SX1276 and SPI library:
#include <SX1276.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h> 

#define RADIO_RFM92_95
#define BAND900

#define gateway_addr 0x01
#define LORAMODE  4
#define NETWORK_KEY_0 0x01
#define NETWORK_KEY_1 0x01

int e;
char my_packet[150];

String my_packet_str;

WiFiClientSecure wifiClient; // Use for secure TLS MQTT

// MQTT Connection info
char server[] = "mqtt.mediumone.com";
int port = 61618;
char pub_topic[]="0/7T3kxm_Rdf4/SjNkEPKgycg";
char sub_topic[]="1/7T3kxm_Rdf4/SjNkEPKgycg/esp8266/event";
char mqtt_username[]="7T3kxm_Rdf4/SjNkEPKgycg";
char mqtt_password[]="FEBTXN77HHV2PJPKJRC52PZQG5SGGMRSGM2TEOBYGQ4DAMBQ/Device1@";


// Wifi credentials
char WIFI_SSID[] = "mediumone";
char WIFI_PASSWORD[] = "cccccccc";

// set pin for LED
const int LED_PIN = 2;

// ongoing timer counter for heartbeat
static int heartbeat_timer = 0;

// ongoing timer counter for sensor
static int sensor_timer = 0;

// set heartbeat period in milliseconds
static int heartbeat_period = 60000;

// set sensor transmit period in milliseconds
static int sensor_period = 5000;

// track time when last connection error occurs
long lastReconnectAttempt = 0;


void setup()
{
  int nCnt = 0;
   // Open serial communications and wait for port to open:
  Serial.begin(9600);

 // wifi setup
  WiFi.mode(WIFI_STA);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  //not sure this is needed
  delay(5000);
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("Failed to connect, resetting"));
    ESP.reset();
  }
  
  // if you get here you have connected to the WiFi
  Serial.println(F("Connected to Wifi!"));

  Serial.println(F("Init hardware LED"));
  pinMode(LED_PIN, OUTPUT);
 
  // connect to MQTT broker to send board reset msg
  connectMQTT();

  Serial.println("Complete Setup");
  
  // Print a start message
  Serial.println(F("SX1276 module and Arduino: receive packets without ACK"));

  // Power ON the module
  e = sx1276.ON();
  Serial.print(F("Setting power ON: state "));
  Serial.println(e, DEC);

  // Setting the network key
  sx1276.setNetworkKey(NETWORK_KEY_0,NETWORK_KEY_1);
  
  // Set transmission mode and print the result
  e = sx1276.setMode(LORAMODE);
  Serial.print(F("Setting Mode: state "));
  Serial.println(e, DEC);
  
  // Select frequency channel
  e = sx1276.setChannel(CH_05_900);
  Serial.print(F("Setting Channel: state "));
  Serial.println(e, DEC);
  
  // Select output power (Xtreme, Max, High or Low)
  e = sx1276.setPower('X');
  Serial.print(F("Setting Power: state "));
  Serial.println(e, DEC);
  
  // Set the node address and print the result
  e = sx1276.setNodeAddress(gateway_addr);
  Serial.print(F("Setting node address: state "));
  Serial.println(e, DEC);
    
  // Print a success message
  Serial.println(F("SX1276 successfully configured"));
  Serial.println();
}

void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
  int i = 0;
  char message_buff[length + 1];
  for(i=0; i < length; i++) {
    message_buff[i] = payload[i];
  }
  message_buff[i] = '\0';
  
  Serial.print(F("Received some data: "));
  Serial.println(String(message_buff));
  
  // Process message to turn LED on and off
  if (String(message_buff[0]) == "0") { 
    // Turn off LED
    digitalWrite(LED_PIN, HIGH);
  } else if (String(message_buff[0]) == "1") { 
    // Turn on LED
    digitalWrite(LED_PIN, LOW);
  }
}

PubSubClient client(server, port, callback, wifiClient);

boolean connectMQTT()
{    
  // Important Note: MQTT requires a unique id (UUID), we are using the mqtt_username as the unique ID
  // Besure to create a new device ID if you are deploying multiple devices.
  // Learn more about Medium One's self regisration option on docs.mediumone.com
  if (client.connect((char*) mqtt_username,(char*) mqtt_username, (char*) mqtt_password)) {
    Serial.println(F("Connected to MQTT broker"));

    // send a connect message
    if (client.publish((char*) pub_topic, "{\"event_data\":{\"mqtt_connected\":true}, \"add_client_ip\":true}")) {
      Serial.println("Publish connected message ok");
    } else {
      Serial.print(F("Publish connected message failed: "));
      Serial.println(String(client.state()));
    }

    // subscrive to MQTT topic
    if (client.subscribe((char *)sub_topic,1)){
      Serial.println(F("Successfully subscribed"));
    } else {
      Serial.print(F("Subscribed failed: "));
      Serial.println(String(client.state()));
    }
  } else {
    Serial.println(F("MQTT connect failed"));
    Serial.println(F("Will reset and try again..."));
    abort();
  }
  return client.connected();
}


void loop(void)
{

if (!client.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 1000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (connectMQTT()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // Client connected
    client.loop();
  }
  
  // Receive message
  e = sx1276.receivePacketTimeout(10000);
  if ( e == 0 )
  {
    
    Serial.print(F("Receive packet, state "));
    Serial.println(e, DEC);

    for (unsigned int i = 0; i < sx1276.packet_received.length; i++)
    {
      my_packet[i] = (char)sx1276.packet_received.data[i];
    }
    my_packet_str = String(my_packet);
    int Index1 = my_packet_str.indexOf('#');
    int Index2 = my_packet_str.indexOf('#', Index1+1);
    int Index3 = my_packet_str.indexOf('#', Index2+1);

    String secondValue = my_packet_str.substring(Index1+1, Index2);
    String thirdValue = my_packet_str.substring(Index2+1, Index3);
    Serial.print(F("Source: "));
    Serial.println(sx1276.packet_received.src);
    Serial.print(F("Message: "));
    Serial.println(my_packet);
    
    sx1276.getRSSIpacket();
    sx1276.getSNR();
    
    Serial.print("Console: RSSI is ");
    Serial.print(sx1276._RSSIpacket,DEC); 
    Serial.print(" dBm");
    Serial.println(" ");    
    Serial.print("Console: SNR is ");
    Serial.print(sx1276._SNR,DEC); 
    Serial.println(" dBm");

    if (client.publish((char *) pub_topic, my_packet) ) {
        Serial.println(F("Publish ok"));
      } else {
        Serial.print(F("Failed to publish heartbeat: "));
        Serial.println(String(client.state()));
      }
   }
  else {
    Serial.print(F("Receive packet, state "));
    Serial.println(e, DEC);
  }
}

