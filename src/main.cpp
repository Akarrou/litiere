#include "Arduino.h"
#include "ESP8266WiFi.h"
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <Adafruit_VL53L0X.h>
#include <RBD_Capacitance.h>
#include <RBD_Threshold.h>
#include <RBD_WaterSensor.h>
#include "ArduinoOTA.h"
#include <NTPClient.h>
#include <Ticker.h>
#include <ArduinoJson.h>
#include <index.h>

// Gestion des événements du WiFi
const char *SSID = "Livebox-5576";
const char *PASSWORD = "RaLvkqAaWUrMcAWmSm";
void onConnected(const WiFiEventStationModeConnected &event);
void onGotIP(const WiFiEventStationModeGotIP &event);
ESP8266WebServer webServer(80);

RBD::WaterSensor water_sensor(13, 15, 15); // send, receive pin, levels

Ticker timer;
Ticker timer2;

WebSocketsServer webSocket = WebSocketsServer(81);

// Relais
#define RELAY_NETOYER 0
#define RELAY_VIDANGE 2
// Lidar
// address we will assign if dual sensor is present
#define LOX1_ADDRESS 0x30
#define LOX2_ADDRESS 0x31
int sensor1, sensor2 = 1;
// set the pins to shutdown
#define SHT_LOX1 16
#define SHT_LOX2 12
// Led Red and Green
#define led_red 14
#define led_green 10

const int TEMPO = 60;
int manuel = 0;
long TOP_CHRONO = 0;
long TEMPCHASSE = 0;
int sensible;
int status;
int presence;
int heure;
int heureLightFin;
int heureLightDebut;
int lidarDistanceMax = 900;
int cleanSensorMax;
int dirtySensorMax;
int duringWaterOn = 30;

// Lidar
// objects for the vl53l0x
Adafruit_VL53L0X lox1 = Adafruit_VL53L0X();
Adafruit_VL53L0X lox2 = Adafruit_VL53L0X();
// this holds the measurement
VL53L0X_RangingMeasurementData_t measure1;
VL53L0X_RangingMeasurementData_t measure2;

// Consrtuctor
void handleRoot();
void setID();
void read_dual_sensors();
void detect();
void waterSensor();
void onStopChange();
void onVidangeChange();
void onNettoyage();
void setDuringWaterOn();
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
void getData();

// NTPClient horloge
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

void setup()
{
  // Mise en place d'une liaison série
  Serial.begin(115200);
  delay(100);

  // Nom de l'objet OTA
  ArduinoOTA.setHostname("Litiere");
  ArduinoOTA.setPassword("test");

  // Mode de connexion
  WiFi.mode(WIFI_STA);
  WiFi.softAP("Litiere");

  // Démarrage de la connexion
  WiFi.begin(SSID, PASSWORD);

  static WiFiEventHandler onConnectedHandler = WiFi.onStationModeConnected(onConnected);
  static WiFiEventHandler onGotIPHandler = WiFi.onStationModeGotIP(onGotIP);

  webServer.on("/", handleRoot);
  webServer.on("/nettoyage", onNettoyage);
  webServer.on("/stop", onStopChange);
  webServer.on("/vidange", onVidangeChange);
  webServer.on("/duringWater", setDuringWaterOn);

  webServer.begin();

  // water sensor
  water_sensor.setLevel(1, 120);
  water_sensor.setLevel(2, 154);
  water_sensor.setLevel(3, 187);
  water_sensor.setMaxLevel(220);

  pinMode(RELAY_NETOYER, OUTPUT);
  pinMode(RELAY_VIDANGE, OUTPUT);
  digitalWrite(RELAY_NETOYER, LOW);
  digitalWrite(RELAY_VIDANGE, LOW);

  // wait until serial port opens for native USB devices
  while (!Serial)
  {
    delay(1);
  }

  pinMode(SHT_LOX1, OUTPUT);
  pinMode(SHT_LOX2, OUTPUT);

  Serial.println("Shutdown pins inited...");

  digitalWrite(SHT_LOX1, LOW);
  digitalWrite(SHT_LOX2, LOW);

  Serial.println("Both in reset mode...(pins are low)");
  Serial.println("Starting...");
  setID();
  // read data toute les 5 seconde

  timer.attach(1, getData);
  timer2.attach(5, waterSensor);
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop()
{

  // Si l'objet est connecté au réseau, on effectue les tâches qui doivent l'être dans ce cas
  if (WiFi.isConnected())
  {
    webServer.handleClient();
    ArduinoOTA.begin();
  }
  timeClient.update();
  delay(10);
  heure = timeClient.getHours(); // heure
  delay(10);
  read_dual_sensors();

  webSocket.loop();

  if (heure == heureLightDebut)
  {
    // digitalWrite(Light, HIGH);
  }
  if (heure == heureLightFin)
  {
    // digitalWrite(Light, LOW);
  }

  if (manuel == 0)
  {
    if (status == 1)
    {
      TOP_CHRONO = (millis() / 1000);
      status = 2;
      delay(10);
    }

    if (status == 2)
    {
      if (((millis() / 1000) - TOP_CHRONO) > TEMPO)
      {
        status = 3;
        TEMPCHASSE = (millis() / 1000);
        TOP_CHRONO = 0;
        delay(10);
      }
    }

    if (status == 3)
    {
      if (((millis() / 1000) - TEMPCHASSE) < duringWaterOn)
      {
        Serial.println("Chasse ON");
        digitalWrite(RELAY_NETOYER, HIGH);
        delay(10);
      }
      else
      {
        Serial.println("Chasse OFF");
        digitalWrite(RELAY_NETOYER, LOW);
        TEMPCHASSE = 0;
        status = 0;
        delay(10);
      }
    }
    else
    {
      digitalWrite(RELAY_NETOYER, LOW);
      TEMPCHASSE = 0;
      delay(50);
    }
  }
  delay(300);

  // ArduinoOTA...
  ArduinoOTA.handle();
}

void onConnected(const WiFiEventStationModeConnected &event)
{
  Serial.println("WiFi connecté");
}

void onGotIP(const WiFiEventStationModeGotIP &event)
{
  Serial.println("Adresse IP : " + WiFi.localIP().toString());
  Serial.println("Passerelle IP : " + WiFi.gatewayIP().toString());
  Serial.println("DNS IP : " + WiFi.dnsIP().toString());
  Serial.print("Puissance de réception : ");
  Serial.println(WiFi.RSSI());
}

void handleRoot()
{
  webServer.send(200, "text/html", index_html);
}

void onNettoyage()
{
  if (webServer.hasArg("onOffNettoyage") == true)
  {
    digitalWrite(RELAY_NETOYER, webServer.arg("onOffNettoyage").toInt());
    delay(50);
    if (webServer.arg("onOffNettoyage").toInt() == true)
    {
      webServer.send(200, "text/html", "On");
      manuel = 1;
    }
    else
    {
      webServer.send(200, "text/html", "Off");
      manuel = 0;
    }
  }
}

void onStopChange()
{
  status = 0;
  TEMPCHASSE = 0;
  TOP_CHRONO = 0;
  manuel = 0;
  presence = 0;
  digitalWrite(RELAY_NETOYER, LOW);
  digitalWrite(RELAY_VIDANGE, LOW);
  delay(50);
  webServer.send(200, "text/plain", "Stop");
}

void onVidangeChange()
{
  if (webServer.hasArg("onOffVidange") == true)
    digitalWrite(RELAY_VIDANGE, webServer.arg("onOffVidange").toInt());
  delay(50);
  {
    if (webServer.arg("onOffVidange").toInt() == true)
    {
      webServer.send(200, "text/html", "On");
      manuel = 1;
    }
    else
    {
      webServer.send(200, "text/html", "Off");
      manuel = 0;
    }
  }
}

void setDuringWaterOn()
{
  if (webServer.hasArg("setDuringWaterOn") == true)
  {
    duringWaterOn = webServer.arg("setDuringWaterOn").toInt();
  }
}

void detect()
{
  if (sensor1 < lidarDistanceMax || sensor2 < lidarDistanceMax)
  {
    status = 1;
    presence = 1;
    Serial.println("Y a quelque chose");
  }
  else
  {
    presence = 0;
  }
}

void setID()
{
  // all reset
  digitalWrite(SHT_LOX1, LOW);
  digitalWrite(SHT_LOX2, LOW);
  delay(10);
  // all unreset
  digitalWrite(SHT_LOX1, HIGH);
  digitalWrite(SHT_LOX2, HIGH);
  delay(10);

  // activating LOX1 and reseting LOX2
  digitalWrite(SHT_LOX1, HIGH);
  digitalWrite(SHT_LOX2, LOW);

  // initing LOX1
  if (!lox1.begin(LOX1_ADDRESS))
  {
    Serial.println(F("Failed to boot first VL53L0X"));
    while (1)
      ;
  }
  delay(10);

  // activating LOX2
  digitalWrite(SHT_LOX2, HIGH);
  delay(10);

  // initing LOX2
  if (!lox2.begin(LOX2_ADDRESS))
  {
    Serial.println(F("Failed to boot second VL53L0X"));
    while (1)
      ;
  }
}

void read_dual_sensors()
{
  lox1.rangingTest(&measure1, false); // pass in 'true' to get debug data printout!
  lox2.rangingTest(&measure2, false); // pass in 'true' to get debug data printout!
  // print sensor one reading
  if (measure1.RangeStatus != 4)
  { // if not out of range
    sensor1 = measure1.RangeMilliMeter;
  }
  else
  {
    Serial.print("Out of range sensor 1");
  }

  // print sensor two reading
  if (measure2.RangeStatus != 4)
  {
    sensor2 = measure2.RangeMilliMeter;
  }
  else
  {
    Serial.print("Out of range sensor 2");
  }

  detect();
}

void waterSensor()
{
  water_sensor.update();

  if (water_sensor.onRawValueChange())
  {
  }
  // Serial.print("Active Level: ");
  // Serial.print(water_sensor.getActiveLevel());
  // Serial.print("  ---  ");
  // Serial.print("Raw Value: ");
  // Serial.println(water_sensor.getRawValue());
}

void getData()
{
  const uint8_t size = JSON_OBJECT_SIZE(200);
  StaticJsonDocument<size> json;
  json["status"] = status;
  json["duringWaterOn"] = duringWaterOn;
  json["lidarDistanceMax"] = lidarDistanceMax;

  JsonArray data = json.createNestedArray("lidar");
  data.add(sensor1);
  data.add(sensor2);

  char buffer[200];
  size_t len = serializeJson(json, buffer);
  webSocket.broadcastTXT(buffer, len);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_DISCONNECTED:

    break;
  case WStype_CONNECTED:
  {
  }
  break;
  case WStype_TEXT:
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
    timer.detach();
    int tempMaxvalue = doc["tempMaxvalue"];
    int sensorMaxvalue = doc["sensorMaxvalue"];

    if (tempMaxvalue > 0)
    {
      duringWaterOn = tempMaxvalue;
    }
    if (sensorMaxvalue > 0)
    {
      lidarDistanceMax = sensorMaxvalue;
    }
    timer.attach(1, getData);
    break;
  }
}