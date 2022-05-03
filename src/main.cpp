#include "Arduino.h"
#include "ESP8266WiFi.h"
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <Adafruit_VL53L0X.h>
#include "ArduinoOTA.h"
#include <NTPClient.h>
#include <Ticker.h>
#include <ArduinoJson.h>
#include "MedianFilterLib.h"
#include <index.h>
#include <ESP_EEPROM.h>
#include <ESP_Mail_Client.h>

MedianFilter<int> medianFilter(5);

// Gestion des événements du WiFi
const char *SSID = "Livebox-5576";
const char *PASSWORD = "RaLvkqAaWUrMcAWmSm";
void onConnected(const WiFiEventStationModeConnected &event);
void onGotIP(const WiFiEventStationModeGotIP &event);
ESP8266WebServer webServer(80);

#define WATER_SENSOR_SIGNAL_PIN A0

WebSocketsServer webSocket = WebSocketsServer(81);
// Memory
#define EEPROM_SIZE 16
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

// identification du compte email utilisé pour l'envoi
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
#define AUTHOR_EMAIL "valettejerome31@gmail.com"
#define AUTHOR_PASSWORD "Hyna.321"

// identification du destinataire
#define RECIPIENT_NAME "jerome valette"
#define RECIPIENT_EMAIL "jeromevalette@orange.fr"
#define EMAIL_TITLE "Alerte eau"

SMTPSession smtp;

const int TEMPO = 60;
int manuel = 0;
boolean alert = false;
long TOP_CHRONO = 0;
unsigned long TEMPCHASSE = 0;
int sensible;
int status;
int presence;
int heure;
int heureLightFin;
int heureLightDebut;
int lidarDistanceMaxSensor1 = 900;
int lidarDistanceMaxSensor2 = 900;
int cleanSensorMax;
int dirtySensorMax;
unsigned long duringWaterOn = 30;
boolean hasWater = false;
boolean waterSensorOn = false;
boolean emailSender = false;
String myArray[0][0];

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
void send_email();

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

  pinMode(RELAY_NETOYER, OUTPUT);
  pinMode(RELAY_VIDANGE, OUTPUT);
  digitalWrite(RELAY_NETOYER, HIGH);
  digitalWrite(RELAY_VIDANGE, HIGH);

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

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // Init EEPROM

  EEPROM.begin(32);

  if (EEPROM.percentUsed() >= 0)
  {
    EEPROM.get(0, duringWaterOn);
    EEPROM.get(4, lidarDistanceMaxSensor1);
    EEPROM.get(8, lidarDistanceMaxSensor2);
  }
  else
  {
    // put some data into eeprom
    EEPROM.put(0, duringWaterOn);
    EEPROM.put(4, lidarDistanceMaxSensor1);
    EEPROM.put(8, lidarDistanceMaxSensor2);
    EEPROM.commit();
  }
  waterSensor();
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
  if (waterSensorOn){
    waterSensor();
  }
    getData();
  webSocket.loop();

  if (heure == 1)
  {
    emailSender = true;
  }

  if (manuel == 0 && !alert)
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
      if (((millis() / 1000UL) - TEMPCHASSE) < duringWaterOn)
      {
        Serial.println("Chasse ON");
        digitalWrite(RELAY_NETOYER, LOW);
        waterSensorOn= true;
        delay(10);
      }
      else
      {
        Serial.println("Chasse OFF");
        digitalWrite(RELAY_NETOYER, HIGH);
        TEMPCHASSE = 0;
        status = 0;
        waterSensorOn = false;
        delay(10);
        setID();
      }
    }
  }

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
    if (webServer.arg("onOffNettoyage").toInt() == false)
    {
      webServer.send(200, "text/html", "On");
      manuel = 1;
      waterSensorOn = true;
    }
    else
    {
      webServer.send(200, "text/html", "Off");
      manuel = 0;
      waterSensorOn = false;
      setID();
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
  digitalWrite(RELAY_NETOYER, HIGH);
  digitalWrite(RELAY_VIDANGE, HIGH);
  delay(50);
  setID();
  webServer.send(200, "text/plain", "Stop");
}

void onVidangeChange()
{
  if (webServer.hasArg("onOffVidange") == true)
  {
    digitalWrite(RELAY_VIDANGE, webServer.arg("onOffVidange").toInt());
    delay(50);

    if (webServer.arg("onOffVidange").toInt() == true)
    {
      webServer.send(200, "text/html", "On");
      manuel = 1;
      setID();
    }
    else
    {
      webServer.send(200, "text/html", "Off");
      manuel = 0;
      setID();
    }
  }
}

void setDuringWaterOn()
{
  if (webServer.hasArg("setDuringWaterOn") == true)
  {
    duringWaterOn = webServer.arg("setDuringWaterOn").toInt();
    EEPROM.put(0, duringWaterOn);
    EEPROM.commit();
  }
}

void detect()
{
  if (sensor1 < lidarDistanceMaxSensor1 || sensor2 < lidarDistanceMaxSensor2)
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

  // activating LOX1 and resetting LOX2
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
  lox1.startRangeContinuous();
  lox2.startRangeContinuous();
}

void read_dual_sensors()
{
  if (lox1.isRangeComplete() || lox2.isRangeComplete())
  {
    sensor1 = medianFilter.AddValue(lox1.readRange());
    sensor2 = medianFilter.AddValue(lox2.readRange());
    detect();
  }
}

void waterSensor()
{
  int valWaterSensor = analogRead(WATER_SENSOR_SIGNAL_PIN);
  if (valWaterSensor < 10)
  {
    hasWater = false;
    alert = true;
    // of pompe
    digitalWrite(RELAY_NETOYER, HIGH);
    delay(10);
    send_email();
  }
  else
  {
    alert = false;
    hasWater = true;
  }
}

void getData()
{
  const uint8_t size = JSON_OBJECT_SIZE(11);
  StaticJsonDocument<size> json;
  json["status"] = status;
  json["duringWaterOn"] = duringWaterOn;
  json["lidarDistanceMaxSensor1"] = lidarDistanceMaxSensor1;
  json["lidarDistanceMaxSensor2"] = lidarDistanceMaxSensor2;
  json["waterSensor"] = hasWater;
  json["senderMail"] = emailSender;
  json["alarme"] = alert;

  JsonArray data = json.createNestedArray("lidar");
  data.add(sensor1);
  data.add(sensor2);

  char buffer[300];
  size_t len = serializeJson(json, buffer);
  webSocket.broadcastTXT(buffer, len);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, payload);
  switch (type)
  {
  case WStype_DISCONNECTED:
    // statements
    break;
  case WStype_CONNECTED:
    // statements
    break;
  case WStype_ERROR:
    // statements
    break;
  case WStype_BIN:
    // statements
    break;
  case WStype_TEXT:
    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
    // timer.detach();
    int tempMaxvalue = doc["tempMaxvalue"];
    int sensor1Maxvalue = doc["sensor1Maxvalue"];
    int sensor2Maxvalue = doc["sensor2Maxvalue"];

    if (tempMaxvalue > 0)
    {
      duringWaterOn = tempMaxvalue;
    }
    if (sensor1Maxvalue > 0)
    {
      lidarDistanceMaxSensor1 = sensor1Maxvalue;
    }
    if (sensor2Maxvalue > 0)
    {
      lidarDistanceMaxSensor2 = sensor2Maxvalue;
    }
    delay(10);
    EEPROM.put(0, duringWaterOn);
    EEPROM.put(4, lidarDistanceMaxSensor1);
    EEPROM.put(8, lidarDistanceMaxSensor2);
    EEPROM.commit();
    // timer.attach(1, getData);
    break;
  }
}

void send_email()
{
  ESP_Mail_Session session;
  char contenu_message[100]; // ici ajusté à 100 caractères maximum
  SMTP_Message message;

  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;

  message.sender.name = "ESP32";
  message.sender.email = AUTHOR_EMAIL;
  message.subject = EMAIL_TITLE;
  message.addRecipient(RECIPIENT_NAME, RECIPIENT_EMAIL);

  // construction du corps du message (inclusion d'un nombre aléatoire)
  sprintf(contenu_message, "Le niveau de l'eau est bas", random(10000) / 100.0);
  message.text.content = contenu_message;

  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_normal;

  if (!smtp.connect(&session))
  {
    emailSender = false;
    return;
  }

  if (!MailClient.sendMail(&smtp, &message))
  {
    emailSender = true;
    Serial.println("Erreur lors de l'envoi du email, " + smtp.errorReason());
  }
}