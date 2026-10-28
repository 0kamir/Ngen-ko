# Ngen-ko 2.0 — Firmware ESP32

Migración del sistema de riego Ngen-ko 1.0.1 (Arduino Uno) a ESP32.
Mantiene los mismos modos y el mismo manejo con botones, y agrega lo necesario para dejarlo instalado y funcionando sin supervisión.

Conexiones: ver [PINOUT.md](PINOUT.md).

## Qué cambia respecto a la v1

| | v1 (Arduino) | v2 (ESP32) |
|---|---|---|
| Modos Apagado / Manual / Automático | ✔ | ✔ igual |
| Botones y pantallas del LCD | ✔ | ✔ igual |
| Umbral y histéresis por zona | ✔ | ✔ |
| Configuración tras un corte de luz | Se perdía | **Se guarda** (modo, umbrales, calibración, WiFi) |
| Sensor desconectado | Leía 0% → regaba | **Muestra ERR y cierra la válvula** |
| Tiempo máximo de riego | — | **Corta tras N min** (por defecto 30) y hace una pausa (60) |
| Horario de riego | — | Opcional: solo regar entre ciertas horas |
| Una zona a la vez | — | Opcional, para no perder presión de agua |
| Control remoto | — | **Página web** en `http://ngen-ko.local` |
| Luz del LCD | Siempre encendida | Se apaga tras 5 min; el primer toque solo la enciende |

## Compilar y subir

Requiere [PlatformIO](https://platformio.org/) (`sudo pacman -S platformio-core` o la extensión de VS Code).

```bash
cd firmware
pio run -t upload        # protoboard con módulo de relés
pio run -e pcb -t upload # placa Ngen-ko 2.0 (ver ../hardware)
pio device monitor       # ver las lecturas por consola (115200)
```

## Primer encendido

1. El equipo arranca **apagado** y sin WiFi configurado. Crea su propia red **`Ngen-ko`** (clave `ngenko123`) y el LED azul parpadea.
2. Conéctate a esa red desde el celular y entra a **http://192.168.4.1**.
3. En la sección **WiFi** ingresa tu red y su clave, y pulsa *Guardar y reiniciar*.
4. Al reconectar, el LED queda fijo y el LCD (en reposo) alterna entre la versión y la **IP**. Entra a esa IP o a **http://ngen-ko.local**.

Si la red guardada no responde en 20 s, el equipo vuelve a abrir la red `Ngen-ko` para que puedas corregirla, y mientras tanto sigue intentando conectarse.

## Calibrar sensores

Las lecturas son en milivolts. Hay que calibrar cada sensor una vez:

1. Pon el equipo en cualquier modo y abre la página web.
2. Con el sensor **seco, al aire**, pulsa **Seco** en su zona.
3. Sumérgelo en un vaso de agua **hasta la línea blanca** (no más) y pulsa **Mojado**.

## Funcionamiento

Botones (igual que la v1):

| Botón  | Manual | Automático |
|--------|--------|------------|
| LEFT   | Cambiar modo (Apagado → Manual → Automático) | ← |
| RIGHT  | Siguiente zona | Siguiente zona |
| UP     | Todas ON / OFF | Umbral +5% (mantener para repetir) |
| DOWN   | ON / OFF zona seleccionada | Umbral −5% |
| SELECT | Resumen de zonas | Humedad de todas las zonas |

En automático, la línea inferior muestra `Z1 T:40% H:35%*`. El `*` indica que esa zona está regando.

**Lógica automática** por zona, cada 1 s:
1. Si el sensor falla (`ERR`) → válvula cerrada.
2. Si lleva más del **tiempo máximo** regando → se cierra y queda en **pausa**.
3. Humedad < umbral − histéresis → abrir. Humedad > umbral + histéresis → cerrar.
4. Fuera del **horario** (si está activo) o con otra zona regando (si está activo *una zona a la vez*) → **esperando**.

El horario necesita la hora de internet. Sin conexión no hay hora, y entonces se riega solo por humedad. La zona horaria viene configurada para Chile continental; se cambia en *Ajustes* con un texto POSIX (lista en [posix_tz_db](https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv)).

## Seguridad

La página web **no tiene contraseña**: cualquiera conectado a tu WiFi puede manejar el riego. Para uso doméstico es razonable. Si el equipo va a estar en una red compartida, avísame y agregamos usuario y clave.

## API (para integraciones, por ejemplo Home Assistant)

| Método | Ruta | Cuerpo |
|---|---|---|
| GET  | `/api/state` | — |
| POST | `/api/mode` | `{"mode": 0\|1\|2}` |
| POST | `/api/relay` | `{"zone": 0-3 o -1 (todas), "on": true}` (solo en manual) |
| POST | `/api/config` | `{"hysteresis", "maxRunMin", "cooldownMin", "oneZoneAtATime", "windowEnabled", "windowStartHour", "windowEndHour", "tz", "zones": [{"threshold", "enabled"}]}` |
| POST | `/api/calibrate` | `{"zone": 0-3, "point": "air"\|"water"}` |
| POST | `/api/wifi` | `{"ssid", "pass"}` (reinicia el equipo) |

## Estructura

```
src/
  config.h      Pines, tiempos y constantes
  settings.*    Configuración guardada en flash (NVS)
  zones.*       Relés, sensores y lógica de riego con protecciones
  ui.*          LCD y botones
  net.*         WiFi, hora NTP, servidor web y API
  web_page.h    La página web (HTML/CSS/JS en un solo archivo)
  main.cpp
```
