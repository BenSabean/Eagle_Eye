/*
   Eagle-Eye Energy Monitor
   Version:     1.0
   Date:        Dec 12, 2017
   LastUpdate:  Dec 13, 2017

   Reading current data from DS2438 and
   transmiting over MQTT
*/

#include <OneWire.h>                // Library for One-Wire interface
#include <ESP8266WiFi.h>            // ESP WiFi Libarary
#include <PubSubClient.h>           // MQTT publisher/subscriber client 
#include <EEPROM.h>                 // Wrie/Read EEPROM
#include <stdio.h>
#include "DS2438.h"                 // Modified library to communicate with DS2438 
// OneWire ICs

//  Threshold for active CT //
#define THRESHOLD    0.1

//  Chip Calibration constants //
float slope[8]  = {1, 1, 1, 1, 1, 1, 1, 1};
float offset[8] = {0, 0, 0, 0, 0, 0, 0, 0};

// define the Arduino digital I/O pin to be used for the 1-Wire network here
const uint8_t ONE_WIRE_PIN = 12;
const uint8_t numCT = 8;

// define the 1-Wire address of the DS2438 battery monitor here (lsb first)
uint8_t chip1_address[] = { 0x26, 0xED, 0x21, 0xCF, 0x01, 0x00, 0x00, 0x3F };
uint8_t chip2_address[] = { 0x26, 0x0F, 0x51, 0xCF, 0x01, 0x00, 0x00, 0x15 };
uint8_t chip3_address[] = { 0x26, 0x1B, 0x34, 0xCF, 0x01, 0x00, 0x00, 0x51 };
uint8_t chip4_address[] = { 0x26, 0xE4, 0x43, 0xCF, 0x01, 0x00, 0x00, 0x3B };

OneWire ow(ONE_WIRE_PIN);

DS2438 chip[4] = {
  DS2438(&ow, chip1_address),
  DS2438(&ow, chip2_address),
  DS2438(&ow, chip3_address),
  DS2438(&ow, chip4_address)
};

//------------------MQTT-----------------------//
#define MQTT_PORT
#define DEVICE_ID 4
/* Wifi setup */
const char* ssid =        "";
const char* password =    "";
/* MQTT connection setup */
const char* mqtt_server = "192.168.2.21";
const char* mqtt_user =   "aerlab";
const char* mqtt_pass =   "server";
/* Topic Setup */
const char* ID = "0";
char gTopic[64];          // Stores the gTopic being sent

WiFiClient espClient;
PubSubClient client(mqtt_server, 1883, espClient);
String clientName;

// ------------------ SETUP --------------------//
void setup()
{
  Serial.begin(115200);
  EEPROM.begin(512);
  Serial.println("START");

  //setEEPROM();    //Only for reseting EEPROM to default value.
  readEEPROM();
  printConstants();

  //  MQTT Setup  //
  client.setServer(mqtt_server, 1883);
  clientName += "esp8266-";
  uint8_t mac[6];
  WiFi.macAddress(mac);
  clientName += macToStr(mac);
  clientName += "-";
  clientName += String(DEVICE_ID);

  // -----------DEBUG----------------
  Serial.println("Module Name: " + clientName);
  Serial.print("Connecting to ");
  Serial.println(ssid);

  //  Wifi Setup  //
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  // -----------DEBUG----------------
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Setting the callback function
  client.setCallback(callback);
  // check for MQTT Connection
  if (!client.connected()) reconnect();
  client.loop();          // Listening loop
  start_listen();

}

// ------------------ LOOP --------------------//
void loop()
{
  bool published;            // Indicates succesful publish
  float data = 0;           // Temprary stored message to publish
  uint8_t deviceCount = 0;  // Tracks number of active CT's
  String Temperature;       // Stores temperature

  Serial.println("LOOP");   // DEBUG
  print_parameters();       // DEBUG

  if (WiFi.status() == WL_CONNECTED)
  {
    // check for MQTT Connection
    if (!client.connected()) reconnect();
    client.loop();          // Listening loop

    /* Reconfigure Slope and Offset */
    for (uint8_t i = 0; i < 4; i++)
      chip[i].begin(slope[i * 2 + 1], offset[i * 2 + 1], slope[i * 2], offset[i * 2]);

    /*  Update Current Temperature  */
    for (uint8_t i = 0; i < 4; i++)  chip[i].update();

    /*  Publish CT data  */
    for (uint8_t i = 0; i < 4; i++)
    { /* Channel 1 Data */
      data = chip[i].getCurrent(DS2438_CHB);
      sprintf(gTopic, "%s/%s/%s%i", ID, "Data" , "CT", i * 2 + 1);
      published = client.publish(gTopic, String(data).c_str());
      if (data > THRESHOLD)  deviceCount ++;   // Counting active sensors
      print_message(String(gTopic), String(data), published);
      /* Channel 2 Data */
      data = chip[i].getCurrent(DS2438_CHA);
      sprintf(gTopic, "%s/%s/%s%i", ID, "Data" , "CT", i * 2 + 2);
      published = client.publish(gTopic, String(data).c_str());
      if (data > THRESHOLD)  deviceCount ++;   // Counting active sensors
      print_message(String(gTopic), String(data), published);
    }

    /*  Get & Publish average temperature  */
    Temperature = avgTemperature();
    sprintf(gTopic, "%s/%s/%s", ID, "Temperature" , "Average");
    published = client.publish(gTopic, Temperature.c_str());
    //print_message(String(gTopic), String(Temperature), published);

    /*  Publish number of active CT's */
    sprintf(gTopic, "%s/%s/%s", ID, "Status" , "DeviceCount");
    published = client.publish(gTopic, String(deviceCount).c_str());
    if (data > THRESHOLD)  deviceCount ++;   // Counting active sensors
    //print_message(String(gTopic), String(deviceCount), published);
  }
  else {
    /* If wifi disconnected try again */
    WiFi.begin(ssid, password);
  }
  delay(2000);
}

/*
   Gets temperatures form all DS2438
   Calculates the average
*/
String avgTemperature ()
{
  double avgTemp = 0.0;
  /*  Accumulate Temperature  */
  for (uint8_t i = 0; i < 4; i++)
    avgTemp += chip[i].getTemperature();
  /*  return Average as String  */
  return String(avgTemp / 4);
}

/* If MQTT connection failed */
void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected()) {

    Serial.print("Attempting MQTT connection...");

    // Attempt to connect
    if (client.connect(clientName.c_str(), mqtt_user, mqtt_pass))
    {
      Serial.println("connected");
      start_listen();
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 30 seconds before retrying
      delay(30000);
    }
  }
}

/* Internal Mac address to string conversion */
String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

/* Debug print */
void print_message(String topic, String data, bool published)
{
  if (published) Serial.println("Topic: " + topic + " Data: " + data);
  else           Serial.println("Topic: " + topic + " FAILED");
}

/* Called when new message arrives */
void callback(char* topic, byte* payload, unsigned int lenght)
{
  char temp[64];
  sprintf(temp, "%s", (char *)payload);
  Serial.println("PAYLOAD: " + String(String(temp).toFloat())); // DEBUG
  /* Check for matching topic */
  for (uint8_t i = 0; i < 8; i++)
  {
    sprintf(gTopic, "%s/%s/%s%i", ID, "Control", "slope", i + 1);
    if (strcmp(gTopic, topic) == 0)
      slope[i] = String(temp).toFloat();

  }
  for (uint8_t i = 0; i < 8; i++)
  {
    sprintf(gTopic, "%s/%s/%s%i", ID, "Control", "offset", i + 1);
    if (strcmp(gTopic, topic) == 0)
      offset[i] = String(temp).toFloat();

  }
}

/* Subscribes to specific topic from the server */
void start_listen()
{
  for (uint8_t i = 1; i < 9; i++)
  {
    sprintf(gTopic, "%s/%s/%s%i", ID, "Control", "slope", i);
    client.subscribe(gTopic, 1);
    client.loop();
  }
  for (uint8_t i = 1; i < 9; i++)
  {
    sprintf(gTopic, "%s/%s/%s%i", ID, "Control", "offset", i);
    client.subscribe(gTopic, 1);
    client.loop();
  }
}

/* DEBUG function to print all slope's and offsets */
void print_parameters()
{
  Serial.print("SLOPES: ");
  for (uint8_t i = 0; i < 8; i++) Serial.print(String(slope[i]) + " ");
  Serial.print("\nOFFSETS: ");
  for (uint8_t i = 0; i < 8; i++) Serial.print(String(offset[i]) + " ");
  Serial.println();
}

void setEEPROM() {
  //  Chip Calibration constants //
  const float newSlope[numCT]  = {0.00408f, 3.9726f, 0.00408f, 0.8178f, 1.0f, 0.7273f, 1.0f, 3.5359f};
  const float newOffset[numCT] = { -0.003852f, -0.0137f, -0.00027f, -0.035f, 0.0f, -0.0265f, 0.0f, -0.0102f};
  for (uint8_t i = 0; i < numCT; i++) {
    EEPROM.put(i * sizeof(float), newSlope[i]);
    EEPROM.put((numCT * sizeof(float)) + (i * sizeof(float)), newOffset[i]);
    //EEPROM.write(i*sizeof(float), newSlope[i]);
    //EEPROM.write((numCT*sizeof(float)) + (i*sizeof(float)), newOffset[i]);
  }
  EEPROM.commit();
}

void readEEPROM() {
  float f = 0.0;
  for (uint8_t i = 0; i < (numCT * 2); i++) {
    EEPROM.get(i * sizeof(float), f );
    if (i < numCT)     slope[i] = f;
    else               offset[i - numCT] = f;
  }
}

void printConstants() {
  Serial.println("Slope:");
  for (uint8_t i = 0; i < numCT; i ++) {
    Serial.print("Address " + String(i * sizeof(float)) + ": ");
    Serial.println(slope[i]);
  }
  Serial.println("Offset:");
  for (uint8_t i = 0; i < numCT; i ++) {
    Serial.print("Address " + String((numCT * sizeof(float)) + (i * sizeof(float))) + ": ");
    Serial.println(offset[i]);
  }
}


