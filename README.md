# **Ngen-ko 1.0.1 - Sistema de Riego Automático**
Este es un programa de **control de riego automático y manual** diseñado para sistemas de irrigación con **sensores de humedad** y **relés de control**, compatible con una pantalla **LCD 16x2** y botones analógicos.

---

## **Características principales**
### **Modos de operación**:
- **Apagado**: Todo el sistema se encuentra inactivo.
- **Manual**: El usuario puede activar o desactivar zonas de riego manualmente.
- **Automático**: El sistema controla automáticamente el riego de las zonas según niveles de humedad predeterminados.

### **Hardware utilizado**:
- **Pantalla LCD 16x2** para mostrar el estado del sistema.
- **Sensores de humedad** analógicos conectados a entradas **A1-A4**.
- **Relés controladores** para activar/desactivar zonas de riego.
- **Botones analógicos** para navegación y control del sistema.

### **Histeresis y umbrales configurables**:
- Permite ajustar **umbrales de humedad** para evitar activaciones innecesarias.
- Implementa un sistema de **"banda muerta"** para estabilidad.

---

## **Diagrama del sistema**

```plaintext
        +----------------------------------+
        |       Ngen-ko Sistema Riego      |
        |          Version 1.0.1           |
        +----------------------------------+
                      |
    +-----------------+-----------------+
    |                                   |
    v                                   v
 +---------+                      +------------+     
 |  Sensor |  A1-A4               |   Relay    |  D2-D3, D10-D11
 | Humedad |----------------------|  Shield    |------------------
 | Zonas 1-4                      |  Control   |   VALVULAS ON/OFF
 +---------+                      +------------+
       |                                 |
       v                                 v
+----------------+               +----------------+
|  Lectura %H2O  |               |  Activar/Desac.|
|  Calibración   |               |  Relé Manual   |
|  Umbral/Hister.|               |   (Modo Manual)|
+----------------+               +----------------+
       |                                 |
       |    Retroalimentación            |
       |      (Modo Automático)          |
       +-----------------+---------------+
                         |
          +----------------------------+
          |      Pantalla LCD 16x2     |
          | Muestra modo y estado de   |
          | cada zona (Manual/Auto)    |
          +----------------------------+
                         ^
                         |
   +---------------------+---------------------+
   |                                           |
   v                                           v
+--------------------+             +---------------------+
|   **Modo Manual**   |             |  **Modo Automático** |
| - Control directo  |             | - Lee sensores      |
|   de relés.        |             | - Evalúa humedad %  |
| - Activa/Desactiva |             | - Umbrales ajustados|
|   cada zona.       |             | - Activa relés solo |
+--------------------+             |   si humedad < TH.  |
                                   +---------------------+
```
## **Estructura del código**
El programa tiene una estructura bien organizada, dividiendo las funcionalidades principales:

### **Configuración inicial (`setup`)**:
- Configura los pines de los **relés**, **sensores de humedad** y la **pantalla LCD**.
- Inicializa el sistema apagado.

### **Bucle principal (`loop`)**:
- Gestiona los modos **Apagado**, **Manual** y **Automático**.
- Implementa un sistema de **standby** tras 3 segundos de inactividad.
- Actualiza las lecturas de humedad y toma decisiones.

### **Modo Manual**:
- Permite al usuario activar/desactivar **zonas individuales**.
- Opciones rápidas: activar/desactivar **todas las zonas** al mismo tiempo.

### **Modo Automático**:
- Lee periódicamente los valores de humedad (cada **1 segundo**).
- Activa/desactiva relés según el **umbral** y la **histeresis** configurados para cada zona.

### **Gestión de botones (`readButton`)**:
- Implementa **anti-rebote** para evitar lecturas erróneas.
- Detecta botones **RIGHT**, **LEFT**, **UP**, **DOWN**, **SELECT** para navegación y configuración del sistema.

---

## **Configuración del hardware**
### **Conexiones principales**:

| **Componente**            | **Pin en Arduino** |
|---------------------------|--------------------|
| Relé Zona 1               | D3                |
| Relé Zona 2               | D2                |
| Relé Zona 3               | D10               |
| Relé Zona 4               | D11               |
| Sensor Humedad Zona 1     | A1                |
| Sensor Humedad Zona 2     | A2                |
| Sensor Humedad Zona 3     | A3                |
| Sensor Humedad Zona 4     | A4                |
| Botones analógicos        | A0                |
| Pantalla LCD 16x2         | 8, 9, 4, 5, 6, 7  |

---

## **Instalación y uso**

### **Clona el repositorio**:
```bash
git clone https://github.com/0kamir/ngen-ko
```
### **Carga el código en Arduino**:
1. Abre el **IDE de Arduino**.
2. Selecciona la **placa Arduino** y el **puerto** correspondiente.
3. Abre el archivo principal del proyecto (**Ngen-ko.ino**) y verifica que no existan errores de compilación.
4. Carga el código en el microcontrolador mediante el botón **"Subir"** del IDE.

### **Conecta el hardware**:
Sigue el esquema de conexiones proporcionado en la sección de **Configuración del hardware**. Asegúrate de:
- Conectar correctamente los **relés** y las **válvulas** al **Relay Shield**.
- Insertar los **sensores de humedad** en las entradas analógicas **A1-A4**.
- Conectar la **pantalla LCD 16x2** a los pines digitales especificados.
- Asegurarte de que los **botones analógicos** funcionen en el pin **A0**.

---

## **Funcionamiento**

### **Modo Manual**
En el **Modo Manual**, el usuario puede controlar las zonas de riego directamente con los botones.

- **RIGHT**: Selecciona la siguiente zona de riego (Z1 → Z2 → Z3 → Z4 → Z1...).
- **UP**: Activa/Desactiva **todas las zonas** al mismo tiempo.  
   - Si todas las zonas están apagadas, se encenderán.  
   - Si todas las zonas están encendidas, se apagarán.
- **DOWN**: Cambia el estado de la zona seleccionada individualmente (ON ↔ OFF).
- **SELECT**: Muestra un **resumen del estado** de todas las zonas en la pantalla LCD durante **1 segundo**.

**Pantalla LCD en Modo Manual:**
```plaintext
Zona X Est: ON/OFF
```
Donde **X** es la zona seleccionada y el estado indica si la válvula está activada (ON) o desactivada (OFF).

**Ejemplo en pantalla LCD (Modo Manual):**
```plaintext
Zona 1 Est: ON
```
En este ejemplo:
- La zona 1 está seleccionada.
- La válvula de la zona 1 está activada (ON).

---

### **Modo Automático**
En el **Modo Automático**, el sistema utiliza la retroalimentación de los **sensores de humedad** para tomar decisiones en función del **umbral configurado** y la **histeresis**.

- **RIGHT**: Selecciona la zona de riego para ajustar su umbral de humedad.
- **UP**: Incrementa el umbral de humedad de la zona seleccionada en 5% (máximo 100%).
- **DOWN**: Reduce el umbral de humedad de la zona seleccionada en 5% (mínimo 0%).
- **SELECT**: Muestra las lecturas de humedad (%) de todas las zonas durante 1 segundo.

**Control automático**:
- Si la humedad leída (%) cae por debajo del umbral configurado, la válvula se activa (ON).
- Si la humedad supera el umbral + histeresis, la válvula se desactiva (OFF).

**Ejemplo en pantalla LCD (Modo Automático):**
```plaintext
Modo: Automático
Z1 T:40% H:35%
```
- **T:40%** → Umbral configurado para la zona 1.
- **H:35%** → Nivel de humedad actual para la zona 1.

En este caso, la humedad actual (35%) está por debajo del umbral (40%), por lo que la válvula de la zona 1 se activa (ON).

---

## **Resumen de botones por modo**

| Botón  | Modo Manual                                 | Modo Automático                       |
|--------|----------------------------------------------|---------------------------------------|
| RIGHT  | Seleccionar la siguiente zona                | Seleccionar zona para ajuste de umbral|
| UP     | Activar/Desactivar todas las zonas           | Aumentar umbral (+5%)                 |
| DOWN   | ON/OFF zona seleccionada                     | Disminuir umbral (-5%)                |
| SELECT | Resumen de estados (1 seg)                   | Mostrar humedad actual (1 seg)        |

---

## **Ejemplo práctico**
1. Inicia el sistema (se encuentra apagado).
2. Presiona **LEFT** para cambiar a Modo Manual.
3. En Modo Manual, usa **RIGHT** para seleccionar la zona, **DOWN** para encenderla/apagarla.  
   Presiona **UP** para activar/desactivar todas las zonas a la vez.  
   Presiona **SELECT** para ver un resumen de todas las zonas.
4. Presiona **LEFT** nuevamente para cambiar a Modo Automático.
5. En Modo Automático, usa **RIGHT** para elegir la zona y **UP/DOWN** para ajustar el umbral de humedad.  
   Presiona **SELECT** para ver las lecturas de humedad actuales de todas las zonas.

---

## **Licencia**
Este proyecto se distribuye bajo la licencia **GNU Affero General Public License v3.0**.  
Más información: [https://www.gnu.org/licenses/agpl-3.0.html](https://www.gnu.org/licenses/agpl-3.0.html)

---
