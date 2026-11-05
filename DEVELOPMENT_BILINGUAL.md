# Development Guide / Guía de Desarrollo

[**English**](#english) | [**Español**](#español)

---

## English

### Development Environment Setup

#### Requirements
- Python 3.7+
- PlatformIO Core
- Git
- VSCode with PlatformIO IDE extension
- MicroSD card (FAT32) for testing

#### Installation

```bash
# Clone repository
git clone https://github.com/EdMaxt/MysticalGrimoire.git
cd MysticalGrimoire

# Install PlatformIO
pip install platformio

# Verify installation
platformio --version
```

### Project Structure

**Main File: `src/main.cpp`**
- ~1600 lines of C++/Arduino code
- Contains all UI, input, and control logic
- Sections:
  - Types and constants (lines 20-60)
  - State variables (lines 65-120)
  - Function prototypes (lines 125-185)
  - Font and color functions (lines 190-250)
  - SD mounting (lines 260-400)
  - Data parsing (lines 410-500)
  - Drawing functions (lines 790-1150)
  - Input handling (lines 1350-1550)
  - Setup and loop (lines 1546-1600)

**Headers:**
- `config.h` - Pin configuration and constants
- `spellbook.h` - Spellbook logic
- `battery.h` - Battery management and icon
- `touch_soft.h` - Software touch driver

**Data Files:**
- `data/spells.json` - 300+ spells from D&D 5.2.1 SRD
- `data/neon_spells.json` - Neon Odyssey spells
- `data/neon_subclasses.json` - Neon Odyssey subclasses

### Building and Uploading

#### Debug Build
```bash
platformio run -v -e cyd
```

#### Release Build
```bash
platformio run -e cyd
```

#### Upload to Device
```bash
# Auto-detect port
platformio run -e cyd --target upload

# Specific port
platformio run -e cyd --target upload --upload-port COM3
```

#### Serial Monitor
```bash
# Auto baud rate
platformio device monitor -e cyd

# Specific baud rate
platformio device monitor -e cyd -b 115200
```

### Common Modifications

#### Change Maximum Brightness
File: `src/main.cpp`, line 79
```cpp
static const uint8_t BRI_DUTY[5] = { 51, 102, 153, 204, 255 };
```

#### Add New Spell
1. Edit `data/spells.json`
2. Add entry:
```json
{
  "id": 340,
  "level": 3,
  "school": 2,
  "name": "My Spell",
  "flags": 0
}
```
3. Recompile and upload SD card

#### Change Sleep Time
File: `src/main.cpp`, line 82
```cpp
static const uint8_t SOFT_MIN[5] = {0, 1, 2, 5, 10};
```

### Main Functions

#### Drawing
- `drawHome()` - Main screen
- `drawOptions()` - Options screen
- `drawClassScreen()` - Class view
- `drawReaderScreen()` - Spell reading
- `drawTabs()` - Level tabs
- `drawList()` - Spell list

#### Input
- `handleTap(int x, int y)` - Process touches
- `touch.read()` - Read touch coordinates

#### Utilities
- `isPortrait()` - Detect orientation
- `getLogicalWidth()` - Dynamic width
- `setRotation(uint8_t r)` - Change rotation
- `cycleRotation()` - Cycle rotations

### NVS (Persistence)

**Stored Variables:**
```cpp
prefs.begin("grimoire", false);
prefs.getUChar("pack", 0)      // Active spell pack
prefs.getUChar("bri", 5)       // Brightness
prefs.getUChar("soff", 3)      // Sleep index
prefs.getUChar("rot", 0)       // Rotation (0 or 2)
```

### Debugging

#### Print Statements
```cpp
Serial.begin(115200);
Serial.println("Debug message");
```

#### Monitor Memory
```cpp
Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
```

### Testing

#### Touch Calibration
Compile with `cyd_touchtest` environment to access calibration test on startup.

#### Verify Rotation
1. Go to Options
2. Press "Rotate" button multiple times
3. Verify it toggles between "Rotate: 0°" and "Rotate: 180°"
4. Verify UI renders correctly in both orientations

### Recent Changes (v1.0.0)

1. Line 88: Variable `rot180` → `currentRot`
2. Lines 1161-1200: Rotation functions refactored
3. Lines 1412-1445: Dynamic `handleTap()` adaptation
4. Line 1559: Rotation initialization
5. Multiple lines: Draw functions refactored

### Battery Charging Detection (v1.0.0 Update)

**Technical Note:** The CYD ESP32-32E does not officially expose the TP4054 charger's CHRG pin to any GPIO. Charging is estimated by monitoring voltage rise (≥0.013V in 60 seconds).

**v1.0.0 Improvement:** Sensitivity increased from 0.04V to 0.013V (3x more sensitive) for better USB detection.

**File:** `src/battery.h` line 61

**Optional Hardware Enhancement:** Solder CHRG pin from TP4054 to GPIO 35 + add 10k pull-up to 3.3V, then set `#define PIN_CHG_STATUS 35` in config.h.

### Common Issues

#### Screen Rotated 90°
**Solution:** Verify `tft.setRotation()` uses values 1 and 3, not 0 and 2.

#### Touch Unresponsive in Rotation
**Solution:** Ensure coordinates are transformed in `handleTap()` switch statement.

#### USB Not Detecting Charge
**Solution:** v1.0.0 improved sensitivity. If still not working:
1. Connect USB and press + button in Options to calibrate battery
2. Verify voltage reading matches multimeter
3. Consider optional CHRG pin solder to GPIO 35 (see "Battery Charging Detection")

#### Firmware Too Large
**Solution:**
1. Remove unnecessary smoothed fonts
2. Compress images/icons
3. Optimize with `platformio run -t release`

### Resources

- [PlatformIO Documentation](https://docs.platformio.org/)
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32)
- [TFT_eSPI Library](https://github.com/Bodmer/TFT_eSPI)
- [D&D 5.2.1 SRD](https://www.dndbeyond.com/sources)

---

## Español

### Configuración del Ambiente de Desarrollo

#### Requisitos
- Python 3.7+
- PlatformIO Core
- Git
- VSCode con extensión PlatformIO IDE
- Tarjeta microSD (FAT32) para pruebas

#### Instalación

```bash
# Clonar repositorio
git clone https://github.com/EdMaxt/MysticalGrimoire.git
cd MysticalGrimoire

# Instalar PlatformIO
pip install platformio

# Verificar instalación
platformio --version
```

### Estructura del Proyecto

**Archivo Principal: `src/main.cpp`**
- ~1600 líneas de código C++/Arduino
- Contiene toda la lógica de UI, input y control
- Secciones:
  - Tipos y constantes (líneas 20-60)
  - Variables de estado (líneas 65-120)
  - Prototipos de funciones (líneas 125-185)
  - Funciones de fuente y color (líneas 190-250)
  - Montaje de SD (líneas 260-400)
  - Parsing de datos (líneas 410-500)
  - Funciones de dibujo (líneas 790-1150)
  - Manejo de input (líneas 1350-1550)
  - Setup y loop (líneas 1546-1600)

**Headers:**
- `config.h` - Configuración de pines y constantes
- `spellbook.h` - Lógica de libros de hechizos
- `battery.h` - Gestión de batería e icono
- `touch_soft.h` - Driver táctil por software

**Archivos de Datos:**
- `data/spells.json` - 300+ hechizos del SRD 5.2.1 de D&D
- `data/neon_spells.json` - Hechizos de Neon Odyssey
- `data/neon_subclasses.json` - Subclases de Neon Odyssey

### Compilación y Subida

#### Build Debug
```bash
platformio run -v -e cyd
```

#### Build Release
```bash
platformio run -e cyd
```

#### Subir a Dispositivo
```bash
# Detección automática de puerto
platformio run -e cyd --target upload

# Puerto específico
platformio run -e cyd --target upload --upload-port COM3
```

#### Monitor Serial
```bash
# Baud rate automático
platformio device monitor -e cyd

# Baud rate específico
platformio device monitor -e cyd -b 115200
```

### Modificaciones Comunes

#### Cambiar Brillo Máximo
Archivo: `src/main.cpp`, línea 79
```cpp
static const uint8_t BRI_DUTY[5] = { 51, 102, 153, 204, 255 };
```

#### Agregar Nuevo Hechizo
1. Editar `data/spells.json`
2. Agregar entrada:
```json
{
  "id": 340,
  "level": 3,
  "school": 2,
  "name": "Mi Hechizo",
  "flags": 0
}
```
3. Recompilar y re-subir SD

#### Cambiar Tiempo de Sleep
Archivo: `src/main.cpp`, línea 82
```cpp
static const uint8_t SOFT_MIN[5] = {0, 1, 2, 5, 10};
```

### Funciones Principales

#### Dibujo
- `drawHome()` - Pantalla principal
- `drawOptions()` - Pantalla de opciones
- `drawClassScreen()` - Vista de clase
- `drawReaderScreen()` - Lectura de hechizo
- `drawTabs()` - Tabs de nivel
- `drawList()` - Lista de hechizos

#### Input
- `handleTap(int x, int y)` - Procesa toques
- `touch.read()` - Lee coordenadas táctiles

#### Utilidades
- `isPortrait()` - Detecta orientación
- `getLogicalWidth()` - Ancho dinámico
- `setRotation(uint8_t r)` - Cambia rotación
- `cycleRotation()` - Cicla rotaciones

### NVS (Persistencia)

**Variables Guardadas:**
```cpp
prefs.begin("grimoire", false);
prefs.getUChar("pack", 0)      // Pack de hechizos activo
prefs.getUChar("bri", 5)       // Brillo
prefs.getUChar("soff", 3)      // Sleep index
prefs.getUChar("rot", 0)       // Rotación (0 o 2)
```

### Debugging

#### Print Statements
```cpp
Serial.begin(115200);
Serial.println("Mensaje de debug");
```

#### Monitorear Memoria
```cpp
Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
```

### Testing

#### Calibración del Táctil
Compilar con entorno `cyd_touchtest` para acceder a prueba de calibración al iniciar.

#### Verificar Rotación
1. Ir a Opciones
2. Presionar botón "Rotate" múltiples veces
3. Verificar que alterna entre "Rotate: 0°" y "Rotate: 180°"
4. Verificar que UI se ve bien en ambas orientaciones

### Cambios Recientes (v1.0.0)

1. Línea 88: Variable `rot180` → `currentRot`
2. Líneas 1161-1200: Funciones de rotación refactorizadas
3. Líneas 1412-1445: `handleTap()` adaptable
4. Línea 1559: Inicialización de rotación
5. Múltiples líneas: Funciones drawXXX refactorizadas

### Problemas Comunes

#### Pantalla Rotada 90°
**Solución:** Verificar que `tft.setRotation()` usa valores 1 y 3, no 0 y 2.

#### Touch no Responde en Rotación
**Solución:** Asegurar que las coordenadas se transforman en switch de `handleTap()`.

#### Firmware Demasiado Grande
**Solución:**
1. Remover fuentes suavizadas innecesarias
2. Comprimir imágenes/iconos
3. Optimizar con `platformio run -t release`

### Recursos

- [Documentación PlatformIO](https://docs.platformio.org/)
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32)
- [Librería TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)
- [D&D 5.2.1 SRD](https://www.dndbeyond.com/sources)

---

*Last updated / Última actualización: 2026-11-02*
