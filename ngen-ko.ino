// Ngen-ko
// Copyright (C) 2024
//
// Este programa es software libre: puedes redistribuirlo y/o modificarlo
// bajo los términos de la Licencia Pública General Affero de GNU
// publicada por la Free Software Foundation, ya sea la versión 3 de la
// Licencia, o cualquier versión posterior.
//
// Este programa se distribuye con la esperanza de que sea útil,
// pero SIN NINGUNA GARANTÍA; ni siquiera la garantía implícita
// de COMERCIABILIDAD o IDONEIDAD PARA UN PROPÓSITO PARTICULAR.
// Consulta la Licencia Pública General Affero de GNU para más detalles.
//
// Deberías haber recibido una copia de la Licencia Pública General
// Affero de GNU junto con este programa.
// Si no, consulta <https://www.gnu.org/licenses/>.

#include <LiquidCrystal.h>

// Pines del LCD Shield
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Pines del Relay Shield
#define RELAY1 3
#define RELAY2 2
#define RELAY3 10
#define RELAY4 11

int relayPins[4] = {RELAY1, RELAY2, RELAY3, RELAY4};

// Sensores de humedad
#define MOISTURE_SENSOR_A1 A1
#define MOISTURE_SENSOR_A2 A2
#define MOISTURE_SENSOR_A3 A3
#define MOISTURE_SENSOR_A4 A4

int moistureSensors[4] = {MOISTURE_SENSOR_A1, MOISTURE_SENSOR_A2, MOISTURE_SENSOR_A3, MOISTURE_SENSOR_A4};

// Calibración individual por zona
int airValues[4] = {500, 500, 500, 500};    
int waterValues[4] = {230, 230, 230, 230};  

// Variables de estado
int mode = 0; // 0=Apagado, 1=Manual, 2=Automático
int relayState[4] = {HIGH, HIGH, HIGH, HIGH};
int selectedRelay = 0; 
int moistureValuesArr[4] = {0, 0, 0, 0}; 

// Umbrales e histeresis
int thresholds[4] = {40, 40, 40, 40};  
int hysteresis = 10;

unsigned long lastInteraction = 0; 
bool inStandby = false;

// Control de rebotes y botones
unsigned long lastButtonPress = 0;
const unsigned long debounceTime = 200;
int lastButton = -1;

enum Button {
  NONE,
  RIGHT_BTN,
  UP_BTN,
  DOWN_BTN,
  LEFT_BTN,
  SELECT_BTN
};

// Actualización automática cada 1 segundo
unsigned long lastMoistureUpdate = 0;
const unsigned long updateInterval = 1000; // 1 seg

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Sistema de Riego");
  lcd.setCursor(0, 1);
  lcd.print("Ngen-ko 1.0.1  ");
  delay(2000);

  for (int i = 0; i < 4; i++) {
    pinMode(relayPins[i], OUTPUT);
  }
  apagarRelays();

  for (int i = 0; i < 4; i++) {
    pinMode(moistureSensors[i], INPUT);
  }

  lcd.clear();
  mostrarModo();
  lastInteraction = millis();
}

void loop() {
  Button button = readButton();

  // Modo apagado
  if (mode == 0) {
    lcd.setCursor(0, 0);
    lcd.print("Sistema Apagado ");
    lcd.setCursor(0, 1);
    lcd.print("Ngen-ko 1.0.1  ");
    if (button == LEFT_BTN) {
      mode = 1; 
      lcd.clear();
      mostrarModo();
      lastInteraction = millis();
      inStandby = false;
      return;
    }
    return; 
  }

  if (inStandby && button != NONE) {
    inStandby = false;
    lcd.clear();
    mostrarModo();
    lastInteraction = millis();
  }

  if (!inStandby && button != NONE) {
    lastInteraction = millis();
    if (button == LEFT_BTN) {
      mode = (mode + 1) % 3; 
      lcd.clear();
      if (mode == 0) {
        apagarRelays();
        lcd.setCursor(0, 0);
        lcd.print("Sistema Apagado ");
        lcd.setCursor(0, 1);
        lcd.print("Ngen-ko 1.0.1  ");
      } else {
        mostrarModo();
      }
    }
  }

  if (!inStandby && mode != 0) {
    if (mode == 1) {
      controlarManual(button);
    } else if (mode == 2) {
      controlarAutomatico(button);
    }
  }

  // Standby tras 3 seg sin interacción
  if (mode != 0 && !inStandby && (millis() - lastInteraction > 3000)) {
    inStandby = true;
    lcd.clear();
    mostrarModo();
    lcd.setCursor(0, 1);
    lcd.print("Ngen-ko 1.0.1  ");
  }

  // Actualización cada 1 seg en modo automático (incluso en standby)
  if (mode == 2) {
    if (millis() - lastMoistureUpdate >= updateInterval) {
      lastMoistureUpdate = millis();
      actualizarZonasAutomatico(); // Se actualiza siempre, sin necesidad de apretar nada
    }
  }
}

void apagarRelays() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(relayPins[i], HIGH);
    relayState[i] = HIGH;
  }
}

void mostrarModo() {
  lcd.setCursor(0, 0);
  if (mode == 0) {
    lcd.print("Sistema Apagado ");
  } else if (mode == 1) {
    lcd.print("Modo: Manual    ");
  } else if (mode == 2) {
    lcd.print("Modo: Automatico");
  }
}

Button readButton() {
  int reading = analogRead(A0);
  int currentButton = NONE;
  if (reading <= 50) currentButton = RIGHT_BTN;
  else if (reading >= 140 && reading <= 210) currentButton = UP_BTN;
  else if (reading >= 290 && reading <= 360) currentButton = DOWN_BTN;
  else if (reading >= 490 && reading <= 510) currentButton = LEFT_BTN;
  else if (reading >= 730 && reading <= 750) currentButton = SELECT_BTN;
  else currentButton = NONE;

  if (currentButton == lastButton && (millis() - lastButtonPress < debounceTime)) {
    return NONE;
  }

  if (currentButton != NONE) {
    lastButton = currentButton;
    lastButtonPress = millis();
  }

  return (Button)currentButton;
}

// -----------------------------------
// MODO MANUAL
// -----------------------------------

void mostrarEstadoZonaManual() {
  lcd.setCursor(0, 1);
  lcd.print("Zona ");
  lcd.print(selectedRelay + 1);
  lcd.print(" Est: ");
  lcd.print(relayState[selectedRelay] == LOW ? "ON " : "OFF");
  lcd.print("   ");
}

void mostrarEstadoZonasManual() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Z1:");
  lcd.print(relayState[0] == LOW ? "ON " : "OFF");
  lcd.print(" Z2:");
  lcd.print(relayState[1] == LOW ? "ON" : "OFF");

  lcd.setCursor(0, 1);
  lcd.print("Z3:");
  lcd.print(relayState[2] == LOW ? "ON " : "OFF");
  lcd.print(" Z4:");
  lcd.print(relayState[3] == LOW ? "ON" : "OFF");
}

void controlarManual(Button button) {
  mostrarEstadoZonaManual();
  switch (button) {
    case RIGHT_BTN:
      selectedRelay = (selectedRelay + 1) % 4;
      break;

    case SELECT_BTN:
      mostrarEstadoZonasManual();
      delay(1000);
      lcd.clear();
      mostrarModo();
      mostrarEstadoZonaManual();
      break;

    case UP_BTN:
      {
        bool allOn = true;
        for (int i = 0; i < 4; i++) {
          if (relayState[i] == HIGH) {
            allOn = false;
            break;
          }
        }
        int newState = allOn ? HIGH : LOW;
        for (int i = 0; i < 4; i++) {
          relayState[i] = newState;
          digitalWrite(relayPins[i], newState);
        }
        lcd.setCursor(0, 1);
        lcd.print("Todas: ");
        lcd.print(newState == LOW ? "ON " : "OFF");
        lcd.print("       ");
      }
      break;

    case DOWN_BTN:
      relayState[selectedRelay] = (relayState[selectedRelay] == HIGH) ? LOW : HIGH;
      digitalWrite(relayPins[selectedRelay], relayState[selectedRelay]);
      mostrarEstadoZonaManual();
      break;

    default:
      break;
  }
}

// -----------------------------------
// MODO AUTOMATICO
// -----------------------------------

void actualizarZonasAutomatico() {
  for (int i = 0; i < 4; i++) {
    int raw = analogRead(moistureSensors[i]);
    int moisturePercent;
    Serial.print("Zona "); Serial.print(i+1);
    Serial.print(" raw: "); Serial.print(raw);

    if (raw > 1000) {
      moisturePercent = 0; // Sin sensor o error
    } else {
      moisturePercent = map(raw, airValues[i], waterValues[i], 0, 100);
      moisturePercent = constrain(moisturePercent, 0, 100);
    }

    Serial.print(" mapped: "); Serial.print(moisturePercent);
    Serial.println("%");

    moistureValuesArr[i] = moisturePercent;

    int lowerBound = thresholds[i] - hysteresis;
    int upperBound = thresholds[i] + hysteresis;

    // Control con banda muerta
    if (relayState[i] == HIGH && moisturePercent < lowerBound) {
      digitalWrite(relayPins[i], LOW);
      relayState[i] = LOW;
    } else if (relayState[i] == LOW && moisturePercent > upperBound) {
      digitalWrite(relayPins[i], HIGH);
      relayState[i] = HIGH;
    }
  }
}

void controlarAutomatico(Button button) {
  switch (button) {
    case UP_BTN:
      thresholds[selectedRelay] = min(100, thresholds[selectedRelay] + 5);
      break;

    case DOWN_BTN:
      thresholds[selectedRelay] = max(0, thresholds[selectedRelay] - 5);
      break;

    case RIGHT_BTN:
      selectedRelay = (selectedRelay + 1) % 4;
      break;

    case SELECT_BTN:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Z1:");
      lcd.print(moistureValuesArr[0]); lcd.print("% ");
      lcd.print("Z2:");
      lcd.print(moistureValuesArr[1]); lcd.print("%");

      lcd.setCursor(0, 1);
      lcd.print("Z3:");
      lcd.print(moistureValuesArr[2]); lcd.print("% ");
      lcd.print("Z4:");
      lcd.print(moistureValuesArr[3]); lcd.print("%");
      delay(1000);
      lcd.clear();
      break;

    default:
      break;
  }

  // Mostrar info en LCD
  lcd.setCursor(0, 0);
  lcd.print("Modo: Automatico");
  lcd.setCursor(0, 1);
  lcd.print("Z");
  lcd.print(selectedRelay + 1);
  lcd.print(" T:");
  lcd.print(thresholds[selectedRelay]);
  lcd.print("% H:");
  lcd.print(moistureValuesArr[selectedRelay]);
  lcd.print("%  ");
}
