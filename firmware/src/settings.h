#pragma once
#include "config.h"

enum Mode : uint8_t {
  MODE_OFF = 0,
  MODE_MANUAL = 1,
  MODE_AUTO = 2
};

struct ZoneSettings {
  uint8_t threshold;   // % de humedad bajo el cual se riega
  uint16_t airMv;      // Lectura del sensor en el aire (0%)
  uint16_t waterMv;    // Lectura del sensor en agua (100%)
  bool enabled;        // Zona en uso (si no, se ignora en automático)
};

struct Settings {
  Mode mode;
  ZoneSettings zones[NUM_ZONES];
  uint8_t hysteresis;       // Banda muerta en %
  uint16_t maxRunMin;       // Tiempo máximo de riego continuo por zona
  uint16_t cooldownMin;     // Pausa obligatoria tras alcanzar el máximo
  bool oneZoneAtATime;      // En automático, regar una zona a la vez

  // Ventana horaria: en automático solo se riega dentro de ella.
  // Requiere hora por internet (NTP); sin hora, se ignora.
  bool windowEnabled;
  uint8_t windowStartHour;
  uint8_t windowEndHour;

  char tz[48];              // Zona horaria POSIX
  char wifiSsid[33];
  char wifiPass[65];
};

extern Settings settings;

void settingsLoad();
// Marca la configuración como modificada; se escribe en flash unos
// segundos después para no gastar la memoria con cada pulsación.
void settingsChanged();
void settingsLoop();
// Escribe inmediatamente (antes de reiniciar, por ejemplo).
void settingsSaveNow();
