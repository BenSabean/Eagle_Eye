/*
   Eagle-Eye Energy Monitor
   Version:     1.0
   Date:        Dec 12, 2017
   LastUpdate:  Dec 13, 2017

   Reading current data from DS2438 and
   transmiting over MQTT
*/

#include <OneWire.h>                // Library for One-Wire interface
#include <DS2438.h>
#include <ESP8266WiFi.h>            // ESP WiFi Libarary
#include <PubSubClient.h>           // MQTT publisher/subscriber client 
#include <stdio.h>

#define DEBUG false

//  Chip Calibration  //
//--------- CT1---------//
#define S1_COEFFICIENT_B    0.027   //0.018
#define S1_OFFSET_B         -0.15    //-0.11
//--------- CT2---------//
#define S1_COEFFICIENT_A    5.55    //3.5
#define S1_OFFSET_A         -0.15   //-0.1
//--------- CT3---------//
#define S2_COEFFICIENT_B    0.027
#define S2_OFFSET_B         -0.15 // -0.1
//--------- CT4---------//
#define S2_COEFFICIENT_A    5.55
#define S2_OFFSET_A         -0.15 // -0.07
//--------- CT5---------//
#define S3_COEFFICIENT_B    0.027
#define S3_OFFSET_B         -10.45  // -6.93
//--------- CT6---------//
#define S3_COEFFICIENT_A    5.55
#define S3_OFFSET_A         -0.15 //-0.07
//--------- CT7---------//
#define S4_COEFFICIENT_B    0.027
#define S4_OFFSET_B         1.4     //17.17
//--------- CT8---------//
#define S4_COEFFICIENT_A    5.5
#define S4_OFFSET_A         -0.15 //-0.07

//  Threshold for active CT //
#define THRESHOLD           0.1

// define the Arduino digital I/O pin to be used for the 1-Wire network here
const uint8_t ONE_WIRE_PIN = 12;

// define the 1-Wire address of the DS2438 battery monitor here (lsb first)
uint8_t chip1_address[] = { 0x26, 0xED, 0x21, 0xCF, 0x01, 0x00, 0x00, 0x3F };
uint8_t chip2_address[] = { 0x26, 0x0F, 0x51, 0xCF, 0x01, 0x00, 0x00, 0x15 };
uint8_t chip3_address[] = { 0x26, 0x1B, 0x34, 0xCF, 0x01, 0x00, 0x00, 0x51 };
uint8_t chip4_address[] = { 0x26, 0xE4, 0x43, 0xCF, 0x01, 0x00, 0x00, 0x3B };

OneWire ow(ONE_WIRE_PIN);

DS2438 chip1(&ow, chip1_address);
DS2438 chip2(&ow, chip2_address);
DS2438 chip3(&ow, chip3_address);
DS2438 chip4(&ow, chip4_address);

//------------------MQTT-----------------------//
#define MQTT_PORT
#define DEVICE_ID 1
/* Wifi setup */
const char* ssid =        "Wireless_AP082574";     //  AER172-2
const char* password =    "8146001239";   //  80mawiomi*
/* MQTT connection setup */
const char* mqtt_server = "aerlab.ddns.net";
const char* mqtt_user =   "aerlab";
const char* mqtt_pass =   "server";
/* Topic Setup */
const char* location =        "Home";
const char* systemName =      "EnergyMonitor";
const char* subSystem =       "EagleEye";
const char* transducerType =  "Current";
char gTopic[64];                             // Stores the gTopic being sent

WiFiClient espClient;
PubSubClient client(mqtt_server, 1883, espClient);
String clientName;

// ------------------ SETUP --------------------//
void setup()
{
  bool published;             // published succesfully flag
  char sensor[10];            // sensor gTopic variable

#if DEBUG // -----------DEBUG----------------
  Serial.begin(115200);
  Serial.println("START");
#endif    // -----------DEBUG----------------

  chip1.begin(S1_COEFFICIENT_A, S1_OFFSET_A, S1_COEFFICIENT_B, S1_OFFSET_B);
  chip2.begin(S2_COEFFICIENT_A, S2_OFFSET_A, S2_COEFFICIENT_B, S2_OFFSET_B);
  chip3.begin(S3_COEFFICIENT_A, S3_OFFSET_A, S3_COEFFICIENT_B, S3_OFFSET_B);
  chip4.begin(S4_COEFFICIENT_A, S4_OFFSET_A, S4_COEFFICIENT_B, S4_OFFSET_B);

  //  MQTT Setup  //
  client.setServer(mqtt_server, 1883);
  clientName += "esp8266-";
  uint8_t mac[6];
  WiFi.macAddress(mac);
  clientName += macToStr(mac);
  clientName += "-";
  clientName += String(DEVICE_ID);

#if DEBUG // -----------DEBUG----------------
  Serial.println("Module Name: " + clientName);
  Serial.print("Connecting to ");
  Serial.println(ssid);
#endif    // -----------DEBUG----------------

  //  Wifi Setup  //
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
    delay(500);

#if DEBUG // -----------DEBUG----------------
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
#endif    // -----------DEBUG----------------

  //  check for MQTT Connection
  if (!client.connected())
    reconnect();
}

// ------------------ LOOP --------------------//
void loop()
{
  bool published;           // Indicates succesful publish
  float data = 0;           // Temprary stored message to publish
  uint8_t deviceCount = 0;  // Tracks number of active CT's
  String Temperature;       // Stores temperature

  //  Update Current $ Temperature  //
  chip1.update();
  chip2.update();
  chip3.update();
  chip4.update();

  //  Get & Publish average temperature  //
  Temperature = avgTemperature();
  sprintf(gTopic, "%s/%s/%s/%s/%s/%s", location, systemName, subSystem, "Temperature", "Data" , "Average");
  published = client.publish(gTopic, Temperature.c_str());

  //  Publish CT1 data  //
  data = chip1.getCurrent(DS2438_CHB);
  sprintf(gTopic, "%s/%s/%s/%s/%s/%s", location, systemName, subSystem, transducerType, "Data" , "CT1");
  published = client.publish(gTopic, String(data).c_str());
  if (data > THRESHOLD)  deviceCount ++;   // Counting active sensors
#if DEBUG // --------------------------DEBUG---------------------------------------
  if (published) Serial.println("Topic: " + String(gTopic) + " Data: " + String(data));
  else           Serial.println("Topic: " + String(gTopic) + " FAILED");
#endif  // ----------------------------DEBUG---------------------------------------

  //  Publish CT2 data  //
  data = chip1.getCurrent(DS2438_CHA);
  sprintf(gTopic, "%s/%s/%s/%s/%s/%s", location, systemName, subSystem, transducerType, "Data" , "CT2");
  published = client.publish(gTopic, String(data).c_str());
  if (data > THRESHOLD)  deviceCount ++;   // Counting active sensors
#if DEBUG // --------------------------DEBUG---------------------------------------
  if (published) Serial.println("Topic: " + String(gTopic) + " Data: " + String(data));
  else           Serial.println("Topic: " + String(gTopic) + " FAILED");
#endif  // ----------------------------DEBUG---------------------------------------

  //  Publish CT3 data  //
  data = chip2.getCurrent(DS2438_CHB);
  sprintf(gTopic, "%s/%s/%s/%s/%s/%s", location, systemName, subSystem, transducerType, "Data" , "CT3");
  published = client.publish(gTopic, String(data).c_str());
  if (data > THRESHOLD)  deviceCount ++;   // Counting active sensors
#if DEBUG // --------------------------DEBUG---------------------------------------
  if (published) Serial.println("Topic: " + String(gTopic) + " Data: " + String(data));
  else           Serial.println("Topic: " + String(gTopic) + " FAILED");
#endif  // ----------------------------DEBUG---------------------------------------

  //  Publish CT4 data  //
  data = chip2.getCurrent(DS2438_CHA);
  sprintf(gTopic, "%s/%s/%s/%s/%s/%s", location, systemName, subSystem, transducerType, "Data" , "CT4");
  published = client.publish(gTopic, String(data).c_str());
  if (data > THRESHOLD)  deviceCount ++;   // Counting active sensors
#if DEBUG // --------------------------DEBUG---------------------------------------
  if (published) Serial.println("Topic: " + String(gTopic) + " Data: " + String(data));
  else           Serial.println("Topic: " + String(gTopic) + " FAILED");
#endif  // ----------------------------DEBUG---------------------------------------

  //  Publish CT5 data  //
  data = chip3.getCurrent(DS2438_CHB);
  sprintf(gTopic, "%s/%s/%s/%s/%s/%s", location, systemName, subSystem, transducerType, "Data" , "CT5");
  published = client.publish(gTopic, String(data).c_str());
  if (data > THRESHOLD)  deviceCount ++;   // Counting active sensors
#if DEBUG // --------------------------DEBUG---------------------------------------
  if (published) Serial.println("Topic: " + String(gTopic) + " Data: " + String(data));
  else           Serial.println("Topic: " + String(gTopic) + " FAILED");
#endif  // ----------------------------DEBUG---------------------------------------

  //  Publish CT6 data  //
  data = chip3.getCurrent(DS2438_CHA);
  sprintf(gTopic, "%s/%s/%s/%s/%s/%s", location, systemName, subSystem, transducerType, "Data" , "CT6");
  published = client.publish(gTopic, String(data).c_str());
  if (data > THRESHOLD)  deviceCount ++;   // Counting active sensors
#if DEBUG // --------------------------DEBUG---------------------------------------
  if (published) Serial.println("Topic: " + String(gTopic) + " Data: " + String(data));
  else           Serial.println("Topic: " + String(gTopic) + " FAILED");
#endif  // ----------------------------DEBUG---------------------------------------

  //  Publish CT7 data  //
  data = chip4.getCurrent(DS2438_CHB);
  /*
     This is a "special" Chip that gives reading of 17.17 at rest
     when CT supplies current the reading current
     to adjust for the following behaviour the range is limited to 15A
  */
  if (data > 15.0) data = 0;  // resets the value for rest condition
  sprintf(gTopic, "%s/%s/%s/%s/%s/%s", location, systemName, subSystem, transducerType, "Data" , "CT7");
  published = client.publish(gTopic, String(data).c_str());
  if (data > THRESHOLD)  deviceCount ++;   // Counting active sensors
#if DEBUG // --------------------------DEBUG---------------------------------------
  if (published) Serial.println("Topic: " + String(gTopic) + " Data: " + String(data));
  else           Serial.println("Topic: " + String(gTopic) + " FAILED");
#endif  // ----------------------------DEBUG---------------------------------------

  //  Publish CT8 data  //
  data = chip4.getCurrent(DS2438_CHA);
  sprintf(gTopic, "%s/%s/%s/%s/%s/%s", location, systemName, subSystem, transducerType, "Data" , "CT8");
  published = client.publish(gTopic, String(data).c_str());
  if (data > THRESHOLD)  deviceCount ++;   // Counting active sensors
#if DEBUG // --------------------------DEBUG---------------------------------------
  if (published) Serial.println("Topic: " + String(gTopic) + " Data: " + String(data));
  else           Serial.println("Topic: " + String(gTopic) + " FAILED");
#endif  // ----------------------------DEBUG---------------------------------------

  //  Publish number of active CT's //;
  sprintf(gTopic, "%s/%s/%s/%s/%s/%s", location, systemName, subSystem, transducerType, "Status" , "DeviceCount");
  published = client.publish(gTopic, String(deviceCount).c_str());
  if (data > THRESHOLD)  deviceCount ++;   // Counting active sensors
#if DEBUG // --------------------------DEBUG---------------------------------------
  if (published) Serial.println("Topic: " + String(gTopic) + " Data: " + String(deviceCount));
  else           Serial.println("Topic: " + String(gTopic) + " FAILED");
#endif  // ----------------------------DEBUG---------------------------------------

  delay(500);
}


/*
   Gets temperatures form all DS2438
   Calculates the average
*/
String avgTemperature ()
{
  double avgTemp = 0.0;
  //  Accumulate Temperature  //
  avgTemp += chip1.getTemperature();
  avgTemp += chip2.getTemperature();
  avgTemp += chip3.getTemperature();
  avgTemp += chip4.getTemperature();
  //  return Average as String  //
  return String(avgTemp / 4);
}

/*
   If MQTT connection failed
*/
void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected()) {
#if DEBUG
    Serial.print("Attempting MQTT connection...");
#endif
    // Attempt to connect
    if (client.connect(clientName.c_str(), mqtt_user, mqtt_pass))
    {
#if DEBUG
      Serial.println("connected");
#endif
    }
    else
    {
#if DEBUG
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
#endif
      // Wait 30 seconds before retrying
      delay(30000);
    }
  }
}

/*
   Internal Mac address to string conversion
*/
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


