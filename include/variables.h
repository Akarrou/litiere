#include "Arduino.h"

// Relais
#define RELAY 0
#define RELAY2 2

const int TEMPO = 60;
int manuel = 0;
long TOP_CHRONO = 0;
long TEMPCHASSE = 0;
int sensible;
int status;
bool relais;
int presence;
int heure;
int heureLightFin;
int heureLightDebut;
int lidarDistanceMax;
int cleanSensorMax;
int dirtySensorMax;
int duringWaterOn;

// Lidar
// address we will assign if dual sensor is present
#define LOX1_ADDRESS 0x30
#define LOX2_ADDRESS 0x31
int sensor1, sensor2 = 1;
// set the pins to shutdown
#define SHT_LOX1 16
#define SHT_LOX2 12