#include "settings.h"
#include <Preferences.h>

Settings settings;

static Preferences prefs;
static bool dirty = false;
static unsigned long lastChange = 0;

// Incrementar si cambia la estructura Settings, para descartar datos viejos.
static const uint8_t SETTINGS_VERSION = 1;

static void setDefaults() {
  memset(&settings, 0, sizeof(settings));
  settings.mode = MODE_OFF;
  for (int i = 0; i < NUM_ZONES; i++) {
    settings.zones[i].threshold = 40;
    settings.zones[i].airMv = 2400;
    settings.zones[i].waterMv = 1100;
    settings.zones[i].enabled = true;
  }
  settings.hysteresis = 10;
  settings.maxRunMin = 30;
  settings.cooldownMin = 60;
  settings.oneZoneAtATime = false;
  settings.windowEnabled = false;
  settings.windowStartHour = 6;
  settings.windowEndHour = 9;
  // Chile continental. Otras: https://github.com/nayarsystems/posix_tz_db
  strlcpy(settings.tz, "<-04>4<-03>,M9.1.6/24,M4.1.6/24", sizeof(settings.tz));
}

void settingsLoad() {
  setDefaults();
  prefs.begin("ngenko", false);
  if (prefs.getUChar("ver", 0) == SETTINGS_VERSION &&
      prefs.getBytesLength("cfg") == sizeof(Settings)) {
    prefs.getBytes("cfg", &settings, sizeof(Settings));
  }
  Serial.printf("Configuracion cargada (modo %d)\n", settings.mode);
}

void settingsChanged() {
  dirty = true;
  lastChange = millis();
}

void settingsSaveNow() {
  prefs.putUChar("ver", SETTINGS_VERSION);
  prefs.putBytes("cfg", &settings, sizeof(Settings));
  dirty = false;
  Serial.println("Configuracion guardada");
}

void settingsLoop() {
  if (dirty && millis() - lastChange >= SETTINGS_SAVE_MS) settingsSaveNow();
}
