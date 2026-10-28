// Ngen-ko
// Copyright (C) 2024
//
// Este programa es software libre: puedes redistribuirlo y/o modificarlo
// bajo los términos de la Licencia Pública General Affero de GNU
// publicada por la Free Software Foundation, ya sea la versión 3 de la
// Licencia, o cualquier versión posterior.
//
// Consulta <https://www.gnu.org/licenses/>.

#pragma once
#include <Arduino.h>

#define NGENKO_VERSION "2.0.0"

#define NUM_ZONES 4

// -----------------------------------
// Pines (ver PINOUT.md)
// -----------------------------------

// Relés. Evitamos pines de arranque (0, 2, 5, 12, 15).
const uint8_t RELAY_PINS[NUM_ZONES] = {26, 27, 25, 33};

// Módulo de relés (protoboard): se activa con LOW.
// PCB Ngen-ko 2.0 (transistor BC337): se activa con HIGH; el entorno
// "pcb" de platformio.ini lo define como false.
#ifndef RELAY_ACTIVE_LOW
#define RELAY_ACTIVE_LOW true
#endif

// Sensores de humedad: SOLO pines ADC1 (ADC2 no funciona con WiFi activo).
const uint8_t SENSOR_PINS[NUM_ZONES] = {36, 39, 34, 35};

// Botones: cada uno entre el pin y GND, usamos pull-up interno.
#define BTN_RIGHT_PIN  4
#define BTN_UP_PIN     18
#define BTN_DOWN_PIN   19
#define BTN_LEFT_PIN   23
#define BTN_SELECT_PIN 32

// LCD 16x2 con adaptador I2C (PCF8574)
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22

// LED integrado del DevKit (indica WiFi)
#define STATUS_LED_PIN 2

// -----------------------------------
// Tiempos
// -----------------------------------
const unsigned long SENSOR_INTERVAL_MS  = 1000;    // Lectura de sensores
const unsigned long STANDBY_MS          = 3000;    // Pantalla de reposo
const unsigned long BACKLIGHT_OFF_MS    = 300000;  // Apagar luz LCD tras 5 min
const unsigned long OVERLAY_MS          = 1500;    // Resúmenes (botón SELECT)
const unsigned long SETTINGS_SAVE_MS    = 2000;    // Espera antes de guardar en flash

// -----------------------------------
// Detección de fallas del sensor (milivolts)
// Con una resistencia de 100k a GND en cada entrada, un sensor
// desconectado lee ~0 mV. Un sensor capacitivo sano nunca baja de ~800 mV.
// -----------------------------------
const uint16_t SENSOR_MIN_VALID_MV = 300;
const uint16_t SENSOR_MAX_VALID_MV = 3100;

// -----------------------------------
// WiFi
// -----------------------------------
#define AP_SSID      "Ngen-ko"
#define AP_PASSWORD  "ngenko123"   // Mínimo 8 caracteres
#define MDNS_NAME    "ngen-ko"     // http://ngen-ko.local
const unsigned long WIFI_CONNECT_TIMEOUT_MS = 20000;
