#pragma once

// WiFi, hora por internet (NTP) y la página web de control.
//
// Sin red configurada, el equipo crea su propia red "Ngen-ko"; conéctate
// a ella y entra a http://192.168.4.1 para ingresar tu WiFi.
void netBegin();
void netLoop();

// Texto corto (16 caracteres) con el estado de la red, para el LCD.
const char *networkLine();
