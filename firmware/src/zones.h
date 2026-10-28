#pragma once
#include "settings.h"

enum ZoneStatus : uint8_t {
  ZS_IDLE,        // Cerrada, sin necesidad de riego
  ZS_WATERING,    // Válvula abierta
  ZS_WAITING,     // Necesita riego pero espera (otra zona o fuera de horario)
  ZS_COOLDOWN,    // Pausa de seguridad tras alcanzar el tiempo máximo
  ZS_SENSOR_FAIL, // Sensor desconectado o fuera de rango
  ZS_DISABLED
};

struct ZoneState {
  bool on;
  int moisture;            // %, -1 si el sensor falla
  uint16_t rawMv;
  ZoneStatus status;
  unsigned long onSince;
  bool inCooldown;
  unsigned long cooldownSince;
};

extern ZoneState zones[NUM_ZONES];

void zonesBegin();
void zonesLoop();

void setMode(Mode m);
void setRelay(int zone, bool on);
void allRelaysOff();
bool allRelaysOn();
void toggleRelay(int zone);
void setAllRelays(bool on);

bool sensorOk(int zone);
const char *statusName(ZoneStatus s);
bool timeIsValid();
