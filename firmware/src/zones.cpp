#include "zones.h"
#include <time.h>

ZoneState zones[NUM_ZONES];

static unsigned long lastSensorRead = 0;

static void writeRelay(int i, bool on) {
  bool level = RELAY_ACTIVE_LOW ? !on : on;
  digitalWrite(RELAY_PINS[i], level ? HIGH : LOW);
}

void setRelay(int i, bool on) {
  if (zones[i].on == on) return;
  zones[i].on = on;
  if (on) zones[i].onSince = millis();
  writeRelay(i, on);
  Serial.printf("Zona %d -> %s\n", i + 1, on ? "ON" : "OFF");
}

void allRelaysOff() {
  for (int i = 0; i < NUM_ZONES; i++) setRelay(i, false);
}

bool allRelaysOn() {
  for (int i = 0; i < NUM_ZONES; i++) {
    if (!zones[i].on) return false;
  }
  return true;
}

void toggleRelay(int i) {
  setRelay(i, !zones[i].on);
}

void setAllRelays(bool on) {
  for (int i = 0; i < NUM_ZONES; i++) setRelay(i, on);
}

void setMode(Mode m) {
  if (settings.mode == m) return;
  // Al cambiar de modo partimos siempre con las válvulas cerradas
  allRelaysOff();
  for (int i = 0; i < NUM_ZONES; i++) zones[i].inCooldown = false;
  settings.mode = m;
  settingsChanged();
}

bool sensorOk(int i) {
  return zones[i].moisture >= 0;
}

bool timeIsValid() {
  return time(nullptr) > 1700000000;  // Ya se sincronizó por NTP
}

const char *statusName(ZoneStatus s) {
  switch (s) {
    case ZS_IDLE:        return "reposo";
    case ZS_WATERING:    return "regando";
    case ZS_WAITING:     return "esperando";
    case ZS_COOLDOWN:    return "pausa";
    case ZS_SENSOR_FAIL: return "falla sensor";
    case ZS_DISABLED:    return "deshabilitada";
  }
  return "?";
}

void zonesBegin() {
  for (int i = 0; i < NUM_ZONES; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    writeRelay(i, false);
    zones[i] = ZoneState{};
    zones[i].moisture = -1;

    analogSetPinAttenuation(SENSOR_PINS[i], ADC_11db);  // Rango 0-3.1 V
  }
}

static uint16_t readSensorMv(int i) {
  uint32_t sum = 0;
  for (int s = 0; s < 8; s++) sum += analogReadMilliVolts(SENSOR_PINS[i]);
  return sum / 8;
}

static void readSensors() {
  for (int i = 0; i < NUM_ZONES; i++) {
    uint16_t mv = readSensorMv(i);
    zones[i].rawMv = mv;

    const ZoneSettings &z = settings.zones[i];
    // Calibración inválida (seco == mojado) se trata como falla, map() dividiría por cero
    if (mv < SENSOR_MIN_VALID_MV || mv > SENSOR_MAX_VALID_MV || z.airMv == z.waterMv) {
      zones[i].moisture = -1;
    } else {
      int pct = map(mv, z.airMv, z.waterMv, 0, 100);
      zones[i].moisture = constrain(pct, 0, 100);
    }

    Serial.printf("Zona %d mV: %u  humedad: %d%%\n", i + 1, mv, zones[i].moisture);
  }
}

static bool insideWindow() {
  if (!settings.windowEnabled || !timeIsValid()) return true;
  time_t now = time(nullptr);
  struct tm t;
  localtime_r(&now, &t);
  int h = t.tm_hour;
  int start = settings.windowStartHour;
  int end = settings.windowEndHour;
  if (start == end) return true;
  if (start < end) return h >= start && h < end;
  return h >= start || h < end;  // Cruza medianoche (ej: 22 a 6)
}

static bool anyOtherOn(int zone) {
  for (int i = 0; i < NUM_ZONES; i++) {
    if (i != zone && zones[i].on) return true;
  }
  return false;
}

// Corta el riego que lleva demasiado tiempo abierto.
// Es la protección principal contra sensores dañados o mal ubicados.
static bool checkMaxRun(int i) {
  if (!zones[i].on || settings.maxRunMin == 0) return false;
  if (millis() - zones[i].onSince < (unsigned long)settings.maxRunMin * 60000UL) return false;
  Serial.printf("Zona %d: tiempo maximo alcanzado\n", i + 1);
  setRelay(i, false);
  return true;
}

static void runAutomatic() {
  bool window = insideWindow();

  for (int i = 0; i < NUM_ZONES; i++) {
    ZoneState &zs = zones[i];

    if (!settings.zones[i].enabled) {
      setRelay(i, false);
      zs.status = ZS_DISABLED;
      continue;
    }

    if (!sensorOk(i)) {
      setRelay(i, false);
      zs.status = ZS_SENSOR_FAIL;
      continue;
    }

    if (checkMaxRun(i)) {
      zs.inCooldown = true;
      zs.cooldownSince = millis();
    }

    if (zs.inCooldown) {
      if (millis() - zs.cooldownSince < (unsigned long)settings.cooldownMin * 60000UL) {
        zs.status = ZS_COOLDOWN;
        continue;
      }
      zs.inCooldown = false;
    }

    // Control con banda muerta (histeresis), igual que la versión 1
    int lower = settings.zones[i].threshold - settings.hysteresis;
    int upper = settings.zones[i].threshold + settings.hysteresis;
    bool needsWater = zs.on ? zs.moisture <= upper : zs.moisture < lower;

    if (!needsWater) {
      setRelay(i, false);
      zs.status = ZS_IDLE;
    } else if (!window || (!zs.on && settings.oneZoneAtATime && anyOtherOn(i))) {
      setRelay(i, false);
      zs.status = ZS_WAITING;
    } else {
      setRelay(i, true);
      zs.status = ZS_WATERING;
    }
  }
}

static void runManual() {
  for (int i = 0; i < NUM_ZONES; i++) {
    checkMaxRun(i);
    zones[i].status = zones[i].on ? ZS_WATERING : ZS_IDLE;
  }
}

void zonesLoop() {
  // El tiempo máximo se vigila en cada vuelta, no solo al leer sensores
  if (settings.mode == MODE_MANUAL) runManual();

  if (millis() - lastSensorRead < SENSOR_INTERVAL_MS) return;
  lastSensorRead = millis();

  readSensors();

  if (settings.mode == MODE_AUTO) {
    runAutomatic();
  } else if (settings.mode == MODE_OFF) {
    allRelaysOff();
    for (int i = 0; i < NUM_ZONES; i++) zones[i].status = ZS_IDLE;
  }
}
