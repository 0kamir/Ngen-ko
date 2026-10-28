#include "ui.h"
#include "zones.h"
#include "net.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

enum Button {
  NONE,
  RIGHT_BTN,
  UP_BTN,
  DOWN_BTN,
  LEFT_BTN,
  SELECT_BTN
};

static const uint8_t BUTTON_PINS[] = {0, BTN_RIGHT_PIN, BTN_UP_PIN, BTN_DOWN_PIN, BTN_LEFT_PIN, BTN_SELECT_PIN};
static const unsigned long DEBOUNCE_MS = 30;
static const unsigned long REPEAT_DELAY_MS = 600;  // UP/DOWN repiten al mantener
static const unsigned long REPEAT_RATE_MS = 200;

static LiquidCrystal_I2C *lcd = nullptr;
static char shown[2][17];  // Lo que hay en pantalla, para escribir solo cambios

static int selectedZone = 0;
static unsigned long lastInteraction = 0;
static bool inStandby = false;
static bool backlightOn = true;
static bool buttonIsRepeat = false;  // La pulsación viene de mantener apretado

enum Overlay { OV_NONE, OV_MANUAL_SUMMARY, OV_AUTO_SUMMARY };
static Overlay overlay = OV_NONE;
static unsigned long overlaySince = 0;

// -----------------------------------
// LCD
// -----------------------------------

static uint8_t findLcdAddress() {
  const uint8_t candidates[] = {0x27, 0x3F};
  for (uint8_t addr : candidates) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) return addr;
  }
  Serial.println("LCD no encontrado en 0x27 ni 0x3F");
  return 0x27;
}

// Escribe una línea completando con espacios; solo toca el LCD si cambió.
static void printLine(int row, const char *text) {
  char buf[17];
  snprintf(buf, sizeof(buf), "%-16s", text);
  if (strcmp(buf, shown[row]) == 0) return;
  lcd->setCursor(0, row);
  lcd->print(buf);
  memcpy(shown[row], buf, sizeof(buf));
}

static void printLines(const char *l0, const char *l1) {
  printLine(0, l0);
  printLine(1, l1);
}

static const char *modeLine() {
  switch (settings.mode) {
    case MODE_MANUAL: return "Modo: Manual";
    case MODE_AUTO:   return "Modo: Automatico";
    default:          return "Sistema Apagado";
  }
}

static const char *onOff(int zone) {
  return zones[zone].on ? "ON" : "OFF";
}

static void formatMoisture(char *out, size_t len, int zone) {
  if (sensorOk(zone)) snprintf(out, len, "%d%%", zones[zone].moisture);
  else snprintf(out, len, "ERR");
}

// -----------------------------------
// Botones
// -----------------------------------

static Button readButton() {
  static Button lastRaw = NONE;
  static Button stable = NONE;
  static unsigned long rawSince = 0;
  static unsigned long pressedSince = 0;
  static unsigned long lastRepeat = 0;

  Button raw = NONE;
  for (int b = RIGHT_BTN; b <= SELECT_BTN; b++) {
    if (digitalRead(BUTTON_PINS[b]) == LOW) {
      raw = (Button)b;
      break;
    }
  }

  unsigned long now = millis();
  if (raw != lastRaw) {
    lastRaw = raw;
    rawSince = now;
  }
  if (now - rawSince < DEBOUNCE_MS) return NONE;

  if (raw != stable) {
    stable = raw;
    pressedSince = now;
    lastRepeat = now;
    buttonIsRepeat = false;
    return stable;  // Nueva pulsación
  }

  bool repeats = stable == UP_BTN || stable == DOWN_BTN;
  if (repeats && now - pressedSince > REPEAT_DELAY_MS && now - lastRepeat > REPEAT_RATE_MS) {
    lastRepeat = now;
    buttonIsRepeat = true;
    return stable;
  }
  return NONE;
}

// -----------------------------------
// Pantallas
// -----------------------------------

static void drawOff() {
  printLines("Sistema Apagado", "Ngen-ko " NGENKO_VERSION);
}

static void drawStandby() {
  // Alterna entre la versión y el estado de la red
  bool showNet = (millis() / 3000) % 2;
  printLines(modeLine(), showNet ? networkLine() : "Ngen-ko " NGENKO_VERSION);
}

static void drawManual() {
  char l1[17];
  snprintf(l1, sizeof(l1), "Zona %d Est: %s", selectedZone + 1, onOff(selectedZone));
  printLines(modeLine(), l1);
}

static void drawManualSummary() {
  char l0[17], l1[17];
  snprintf(l0, sizeof(l0), "Z1:%-3s Z2:%s", onOff(0), onOff(1));
  snprintf(l1, sizeof(l1), "Z3:%-3s Z4:%s", onOff(2), onOff(3));
  printLines(l0, l1);
}

static void drawAuto() {
  char l1[17], h[5];
  ZoneStatus st = zones[selectedZone].status;
  if (st == ZS_COOLDOWN) {
    snprintf(l1, sizeof(l1), "Z%d en pausa", selectedZone + 1);
  } else if (st == ZS_DISABLED) {
    snprintf(l1, sizeof(l1), "Z%d deshabilitada", selectedZone + 1);
  } else {
    formatMoisture(h, sizeof(h), selectedZone);
    snprintf(l1, sizeof(l1), "Z%d T:%d%% H:%s%s", selectedZone + 1,
             settings.zones[selectedZone].threshold, h, zones[selectedZone].on ? "*" : "");
  }
  printLines(modeLine(), l1);
}

static void drawAutoSummary() {
  char l0[17], l1[17], a[5], b[5];
  formatMoisture(a, sizeof(a), 0);
  formatMoisture(b, sizeof(b), 1);
  snprintf(l0, sizeof(l0), "Z1:%-4s Z2:%s", a, b);
  formatMoisture(a, sizeof(a), 2);
  formatMoisture(b, sizeof(b), 3);
  snprintf(l1, sizeof(l1), "Z3:%-4s Z4:%s", a, b);
  printLines(l0, l1);
}

static void showTemporary(const char *l0, const char *l1) {
  printLines(l0, l1);
}

// -----------------------------------
// Manejo de botones por modo (igual que la versión 1)
// -----------------------------------

static void handleManual(Button button) {
  if (buttonIsRepeat) return;  // Mantener apretado no debe abrir/cerrar válvulas en ráfaga
  switch (button) {
    case RIGHT_BTN:
      selectedZone = (selectedZone + 1) % NUM_ZONES;
      break;
    case UP_BTN: {
      bool turnOn = !allRelaysOn();
      setAllRelays(turnOn);
      break;
    }
    case DOWN_BTN:
      toggleRelay(selectedZone);
      break;
    case SELECT_BTN:
      overlay = OV_MANUAL_SUMMARY;
      overlaySince = millis();
      break;
    default:
      break;
  }
}

static void handleAuto(Button button) {
  uint8_t &th = settings.zones[selectedZone].threshold;
  switch (button) {
    case RIGHT_BTN:
      selectedZone = (selectedZone + 1) % NUM_ZONES;
      break;
    case UP_BTN:
      th = min(100, th + 5);
      settingsChanged();
      break;
    case DOWN_BTN:
      th = max(0, th - 5);
      settingsChanged();
      break;
    case SELECT_BTN:
      overlay = OV_AUTO_SUMMARY;
      overlaySince = millis();
      break;
    default:
      break;
  }
}

static void wake() {
  lastInteraction = millis();
  inStandby = false;
  if (!backlightOn) {
    lcd->backlight();
    backlightOn = true;
  }
}

void uiBegin() {
  for (int b = RIGHT_BTN; b <= SELECT_BTN; b++) pinMode(BUTTON_PINS[b], INPUT_PULLUP);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  lcd = new LiquidCrystal_I2C(findLcdAddress(), 16, 2);
  lcd->init();
  lcd->backlight();
  memset(shown, 0, sizeof(shown));

  showTemporary("Sistema de Riego", "Ngen-ko " NGENKO_VERSION);
  delay(2000);
  lastInteraction = millis();
}

void uiLoop() {
  Button button = readButton();
  unsigned long now = millis();

  if (button != NONE) {
    bool wasDark = !backlightOn;
    wake();
    // Con la luz del LCD apagada, el primer toque solo la enciende
    if (wasDark) button = NONE;
  }

  if (button == LEFT_BTN) {
    setMode((Mode)((settings.mode + 1) % 3));
    overlay = OV_NONE;
    button = NONE;
  }

  if (settings.mode == MODE_MANUAL) handleManual(button);
  else if (settings.mode == MODE_AUTO) handleAuto(button);

  if (overlay != OV_NONE && now - overlaySince > OVERLAY_MS) overlay = OV_NONE;

  if (!inStandby && settings.mode != MODE_OFF && now - lastInteraction > STANDBY_MS) {
    inStandby = true;
  }
  if (backlightOn && now - lastInteraction > BACKLIGHT_OFF_MS) {
    lcd->noBacklight();
    backlightOn = false;
  }

  if (settings.mode == MODE_OFF) drawOff();
  else if (overlay == OV_MANUAL_SUMMARY) drawManualSummary();
  else if (overlay == OV_AUTO_SUMMARY) drawAutoSummary();
  else if (inStandby) drawStandby();
  else if (settings.mode == MODE_MANUAL) drawManual();
  else drawAuto();
}
