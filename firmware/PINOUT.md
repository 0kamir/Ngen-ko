# Ngen-ko 2.0 — Conexiones en protoboard (fase 1)

Placa: **ESP32 DevKit V1 (ESP32-WROOM-32, 30 o 38 pines)**.
Reutiliza el **módulo de 4 relés** y los **sensores capacitivos** de la versión 1.

## Tabla de pines

| Función            | GPIO ESP32 | Antes (Arduino) | Notas |
|--------------------|-----------:|----------------:|-------|
| Relé zona 1        | 26 | D3  | |
| Relé zona 2        | 27 | D2  | |
| Relé zona 3        | 25 | D10 | En la v1 chocaba con la luz del LCD Keypad Shield |
| Relé zona 4        | 33 | D11 | |
| Sensor zona 1      | 36 (VP) | A1 | Solo entrada, ADC1 |
| Sensor zona 2      | 39 (VN) | A2 | Solo entrada, ADC1 |
| Sensor zona 3      | 34 | A3 | Solo entrada, ADC1 |
| Sensor zona 4      | 35 | A4 | Solo entrada, ADC1 |
| Botón RIGHT        | 4  | A0 (escalera) | Botón entre pin y GND |
| Botón UP           | 18 | A0 | |
| Botón DOWN         | 19 | A0 | |
| Botón LEFT         | 23 | A0 | |
| Botón SELECT       | 32 | A0 | |
| LCD I²C SDA        | 21 | — | Adaptador PCF8574 |
| LCD I²C SCL        | 22 | — | |
| LED de estado      | 2  | — | LED azul del DevKit |

Pines que **no** se usan a propósito:
- **0, 2, 5, 12, 15**: definen el modo de arranque. Si se conecta un relé en ellos, puede activarse solo al encender (el 2 solo maneja el LED).
- **ADC2 (0, 2, 4, 12–15, 25–27)**: no sirve para leer sensores con el WiFi encendido. Por eso los sensores van en 34–39.
- **6–11**: los usa la memoria flash interna.

## Diagrama

```
                    ESP32 DevKit
                 ┌───────────────┐
   Sensor Z1 ────┤ 36 (VP)    23 ├──── Botón LEFT ──── GND
   Sensor Z2 ────┤ 39 (VN)    22 ├──── LCD SCL
   Sensor Z3 ────┤ 34         21 ├──── LCD SDA
   Sensor Z4 ────┤ 35         19 ├──── Botón DOWN ──── GND
 Botón SELECT ───┤ 32         18 ├──── Botón UP ────── GND
   Relé IN4 ─────┤ 33          4 ├──── Botón RIGHT ─── GND
   Relé IN3 ─────┤ 25            │
   Relé IN1 ─────┤ 26            │
   Relé IN2 ─────┤ 27            │
                 │           3V3 ├──── VCC sensores, VCC lógica relés
                 │ VIN (5V)  GND ├──── GND común
                 └───────────────┘
```

## Detalles importantes

### 1. Módulo de relés (ESP32 a 3.3 V, módulo a 5 V)
El ESP32 saca **3.3 V**, y el módulo de relés fue hecho para 5 V. Si lo conectas directo, puede que el relé **no se apague bien** (queda el LED tenue o el relé "zumbando").

**Si tu módulo tiene el puente `JD-VCC`** (casi todos los de 4 relés con optoacoplador):
1. Quita el puente (jumper) entre `JD-VCC` y `VCC`.
2. `VCC` del conector de entrada → **3V3** del ESP32 (alimenta los LEDs de los optoacopladores).
3. `JD-VCC` → **5 V** (alimenta las bobinas de los relés).
4. `GND` → GND común.

Así además queda aislado eléctricamente el lado de las bobinas.

Si tu módulo no tiene ese puente y no apaga bien, avísame y lo resolvemos con un transistor por canal. Así quedará en la PCB de la fase 2 de todas formas.

Si algún día usas un módulo que se activa con HIGH, cambia `RELAY_ACTIVE_LOW` a `false` en `src/config.h`.

### 2. Sensores capacitivos
- Aliméntalos a **3.3 V**, no a 5 V. A 5 V su salida puede superar lo que acepta el ESP32.
- Pon una **resistencia de 100 kΩ entre cada entrada (36, 39, 34, 35) y GND**. Con eso, un sensor desconectado lee ~0 mV y el sistema muestra **ERR** en vez de inventar una humedad. En la v1, un sensor suelto podía marcar 0% y regar sin parar.
- Las lecturas ahora son en **milivolts**, así que la calibración de la v1 (500/230) ya no sirve. Calibra desde la página web (ver README).

### 3. LCD 16x2 con adaptador I²C
- Alimenta el adaptador con **5 V** (con 3.3 V el texto casi no se ve).
- Estos adaptadores traen resistencias pull-up a 5 V en SDA/SCL, y el ESP32 no está garantizado para 5 V. En protoboard normalmente funciona, pero lo correcto es una de estas dos:
  - un **conversor de nivel I²C** (módulo BSS138 de 4 canales, ~1 USD), o
  - retirar las 2 resistencias pull-up del adaptador y poner 4.7 kΩ desde SDA y SCL a **3V3**.
- El firmware detecta solo la dirección 0x27 o 0x3F. Si se enciende la luz pero no hay texto, ajusta el potenciómetro azul del adaptador (contraste).

### 4. Botones
Cada botón va entre su GPIO y **GND**; el ESP32 usa su pull-up interno, sin resistencias externas.
El LCD Keypad Shield viejo no sirve tal cual: sus 5 botones comparten un solo pin con una escalera de resistencias, y eso en el ESP32 es poco confiable. Usa 5 pulsadores comunes.

### 5. Alimentación
- Protoboard: el ESP32 por USB y los relés y el LCD desde el pin **VIN/5V** sirven para pruebas sin válvulas.
- **Con válvulas reales** usa una fuente aparte: 12 V DC para válvulas de 12 V, o un transformador de 24 V AC para válvulas de riego estándar. Esa fuente pasa **solo** por los contactos de los relés (COM → NO → válvula).
- Si el ESP32 se reinicia al activar un relé, la fuente de 5 V es débil. Usa un cargador de 5 V / 2 A como mínimo.
- Pon un **diodo 1N4007 en paralelo con cada válvula de 12 V DC** (cátodo al +) para proteger los contactos del relé. Con 24 V AC no hace falta.
