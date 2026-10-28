# Ngen-ko 2.0 — Hardware

Placa única que reemplaza el montaje de la v1 (Arduino Uno + LCD Keypad Shield + módulo de 4 relés).

| Archivo | Contenido |
|---|---|
| `ngen-ko.kicad_pro` | Proyecto KiCad 10 |
| `ngen-ko.kicad_sch` | Esquemático |
| `ngen-ko.pdf` | Esquemático en PDF, para verlo sin KiCad |
| `ngen-ko.kicad_sym` | Símbolo propio: ESP32 DevKit V1 de 30 pines |
| `ngen-ko.pretty/` | Huella propia: zócalo del DevKit |

## Abrir el proyecto

KiCad necesita sus librerías oficiales para la fase 3 (PCB y vista 3D):

```bash
sudo pacman -S kicad-library kicad-library-3d
```

El esquemático trae los símbolos embebidos, así que se ve completo aunque no tengas las librerías. Sin ellas, el ERC muestra ~230 avisos de "biblioteca no incluida", y desaparecen al instalarlas. Fuera de esos avisos, **el ERC no tiene errores ni advertencias**.

## Bloques

| Bloque | Qué hace |
|---|---|
| **Alimentación** | Entrada 12V DC por jack (J1) o bornera (J2). Fusible rearmable de 2 A (F1), diodo contra polaridad invertida (D1) y regulador conmutado TSR 1-2450 de 5V / 1 A (U1). El DevKit genera los 3.3V con su propio regulador. |
| **ESP32** | DevKit V1 de 30 pines en zócalo (2 tiras hembra 1x15). Mismo pinout que la fase 1 (`firmware/PINOUT.md`). |
| **Sensores** | 4 conectores JST XH de 3 pines (3.3V, señal, GND). Cada entrada tiene filtro 1k + 100nF y resistencia de 100k a GND para detectar un sensor desconectado. |
| **Relés** | 4 × SRD-05VDC-SL-C manejados por transistor BC337, con diodo de rueda libre, 10k a GND (relé apagado mientras arranca el ESP32) y LED rojo por zona. **Se activan con HIGH**: compilar el firmware con `pio run -e pcb`. |
| **Bornera de válvulas** | J4: ZONA1–4 + COM. Un TVS bidireccional P6KE47CA en cada salida absorbe el pico de la bobina de la válvula y sirve tanto para 12V DC como para 24V AC. |
| **Fuente de válvulas** | Con **JP1 y JP2 puestos**, las válvulas usan la misma entrada de 12V. Para **válvulas de 24V AC** (riego profesional) se quitan ambos puentes y el transformador se conecta en J3. |
| **LCD** | Conector J9 con el orden del adaptador PCF8574 (GND, VCC, SDA, SCL) y conversor de nivel I²C 3.3V ↔ 5V con 2 BSS138. Resuelve lo que en la protoboard era un "normalmente funciona". |
| **Botones** | SW1–SW5 en la placa, y J10 para un panel frontal en paralelo. |
| **Expansión** | J11: GPIO13, 14, 16 y 17 libres, más 3.3V y GND (sensor de lluvia, caudalímetro…). |

## Consumo estimado (5V)

| Carga | Corriente |
|---|---|
| 4 bobinas de relé (72 mA c/u) | 290 mA |
| ESP32 con WiFi (picos de transmisión) | hasta 500 mA |
| LCD con retroiluminación | ~40 mA |
| LEDs | ~20 mA |
| **Peor caso** | **~850 mA**: cabe en el TSR 1-2450 (1 A) |

En la entrada de 12V eso equivale a ~0.45 A. Si las válvulas también se alimentan de los 12V (JP1/JP2 puestos), súmales su consumo: típicamente 0.2–0.4 A por válvula. El fusible F1 de 2 A está calculado para 1 o 2 válvulas a la vez. Para más, usa la opción "una zona a la vez" del firmware o una fuente aparte en J3.

## Lista de materiales

Todo es de **agujero pasante (THT)** para soldar a mano. La única excepción son Q5 y Q6 (BSS138, SOT-23), que son pequeños pero se sueldan con cautín de punta fina.

| Cant. | Referencias | Componente | Huella |
|---:|---|---|---|
| 1 | U2 | ESP32 DevKit V1 30 pines (DOIT) + 2 tiras hembra 1x15 | propia |
| 1 | U1 | TRACO TSR 1-2450 (o clon K7805-1000R3) | TO-220 vertical |
| 4 | K1–K4 | Relé Songle SRD-05VDC-SL-C | SRD |
| 4 | Q1–Q4 | BC337 | TO-92 |
| 2 | Q5, Q6 | BSS138 | SOT-23 |
| 1 | F1 | PTC Bourns MF-RHT200 (2 A) | radial |
| 1 | D1 | 1N5819 | DO-41 |
| 4 | D3–D6 | 1N4148 | DO-35 |
| 4 | D11–D14 | P6KE47CA (TVS bidireccional) | DO-15 |
| 4 | D7–D10 | LED rojo 3 mm | |
| 1 | D2 | LED verde 3 mm | |
| 1 | D15 | LED azul 3 mm | |
| 14 | R1–R5, R10–R17, R26 | 1 kΩ ¼ W | axial 7.62 mm |
| 8 | R6–R9, R22–R25 | 10 kΩ ¼ W | axial 7.62 mm |
| 4 | R18–R21 | 100 kΩ ¼ W | axial 7.62 mm |
| 6 | C2, C4, C6–C9 | 100 nF cerámico | disco 5 mm |
| 1 | C1 | 100 µF 25 V electrolítico | radial 6.3 mm |
| 1 | C3 | 47 µF 10 V electrolítico | radial 5 mm |
| 1 | J1 | Jack DC 5.5/2.1 mm | |
| 2 | J2, J3 | Bornera 2 polos, paso 5.08 | |
| 1 | J4 | Bornera 5 polos, paso 5.08 | |
| 4 | J5–J8 | JST XH 3 pines (B3B-XH-A) | |
| 1 | J9 | Tira macho 1x4 | |
| 2 | J10, J11 | Tira macho 1x6 | |
| 2 | JP1, JP2 | Tira macho 1x2 + jumper | |
| 5 | SW1–SW5 | Pulsador 6x6 mm | |

## Antes de la fase 3 (PCB): verificar

1. **Separación entre filas del DevKit.** La huella usa **25.4 mm** (1"), lo habitual en el DOIT de 30 pines. Algunas variantes usan 22.86 mm. Mide la tuya de centro a centro de pin. Si es distinta, se cambia en `ngen-ko.pretty/ESP32_DevKit_V1_30P.kicad_mod`.
2. **Orden de pines del DevKit.** El símbolo asume el pinout DOIT. Columna izquierda, desde la antena: EN, VP, VN, 34, 35, 32, 33, 25, 26, 27, 14, 12, 13, GND, VIN. Columna derecha: 23, 22, TX0, RX0, 21, 19, 18, 5, TX2, RX2, 4, 2, 15, GND, 3V3. Compáralo con lo serigrafiado en tu placa.
3. **Tipo de válvulas.** Define si usarás JP1/JP2 o J3.
