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

#include "config.h"
#include "settings.h"
#include "zones.h"
#include "ui.h"
#include "net.h"

void setup() {
  // Primero los relés, para que las válvulas queden cerradas desde el arranque
  zonesBegin();

  Serial.begin(115200);
  Serial.println("\nNgen-ko " NGENKO_VERSION);

  settingsLoad();  // Recupera el modo y los umbrales tras un corte de luz
  uiBegin();
  netBegin();
}

void loop() {
  uiLoop();
  zonesLoop();
  netLoop();
  settingsLoop();
}
