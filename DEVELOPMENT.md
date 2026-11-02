# Guía de Desarrollo

## Ambiente de Desarrollo

### Requisitos
- Python 3.7+
- PlatformIO Core
- Git
- MicroSD card (para datos)

### Setup

```bash
# Clonar repositorio
git clone https://github.com/EdMaxt/MysticalGrimoire.git
cd Mystical_Grimoire

# Instalar dependencias
pip install platformio

# Verificar instalación
platformio --version
```

## Estructura del Código

### Archivo Principal: `src/main.cpp`
- **~1600 líneas** de código C++ / Arduino
- Contiene toda la lógica de UI, input y control
- Organizado en secciones:
  - Tipos y constantes (línea 20-60)
  - Variables de estado (línea 65-120)
  - Prototipos de funciones (línea 125-185)
  - Funciones de fuente y color (línea 190-250)
  - Funciones de montaje SD (línea 260-400)
  - Parsing de datos (línea 410-500)
  - Funciones de dibujo (línea 790-1150)
  - Manejo de input (línea 1350-1550)
  - Setup y loop (línea 1546-1600)

### Headers
- `config.h` - Configuración de pines y constantes
- `spellbook.h` - Lógica de libros de hechizos
- `battery.h` - Gestión de batería e icono
- `touch_soft.h` - Driver táctil por software

### Data Files
- `data/spells.json` - 339 hechizos SRD 5.2.1
- `data/neon_spells.json` - 95 hechizos Neon Odyssey
- `data/neon_subclasses.json` - 52 subclases Neon

## Compilación

### Build Debug
```bash
platformio run -v -e cyd
```

### Build Release
```bash
platformio run -e cyd
```

### Limpiar Build
```bash
platformio run -t clean -e cyd
```

## Upload a Dispositivo

### Upload Automático (Detección de Puerto)
```bash
platformio run -e cyd --target upload
```

### Upload Manual (Puerto Específico)
```bash
platformio run -e cyd --target upload --upload-port COM3
```

### Monitor Serial
```bash
# Monitor con baud rate automático
platformio device monitor -e cyd

# Monitor con baud rate específico
platformio device monitor -e cyd -b 115200
```

## Modificaciones Comunes

### Cambiar Nivel de Brillo Máximo
En `src/main.cpp`, línea 79:
```cpp
static const uint8_t BRI_DUTY[5] = { 51, 102, 153, 204, 255 };
```

### Agregar Hechizo Nuevo
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
3. Recompilar y re-upload de datos SD

### Cambiar Tiempo de Sleep
En `src/main.cpp`, línea 82:
```cpp
static const uint8_t SOFT_MIN[5] = {0, 1, 2, 5, 10};
```

## Funciones Principales

### Dibujo
- `drawHome()` - Pantalla principal
- `drawOptions()` - Pantalla de opciones
- `drawClassScreen()` - Vista de clase
- `drawReaderScreen()` - Lectura de hechizo
- `drawTabs()` - Tabs de nivel
- `drawList()` - Lista de hechizos

### Input
- `handleTap(int x, int y)` - Procesa toques
- `touch.read()` - Lee coordenadas táctiles

### Utilidades
- `isPortrait()` - Detecta orientación
- `getLogicalWidth()` - Ancho dinámico
- `setRotation(uint8_t r)` - Cambia rotación
- `cycleRotation()` - Cicla rotaciones

## NVS (Persistencia)

### Variables Guardadas
```cpp
prefs.begin("grimoire", false);
prefs.getUChar("pack", 0)      // Paquete activo
prefs.getUChar("bri", 5)       // Brillo
prefs.getUChar("soff", 3)      // Sleep index
prefs.getUChar("rot", 0)       // Rotación
```

## Debugging

### Print Statements
```cpp
Serial.begin(115200);
Serial.println("Mensaje de debug");
```

### Monitorear Memoria
```cpp
Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
```

## Testing

### Test del Touch
Compilar con:
```cpp
#define TOUCH_TEST
```

Esto abre el test de calibración táctil en startup.

### Verificar Rotación
1. Ir a Options
2. Presionar "Rotate" múltiples veces
3. Verificar que alterna entre "Rotate: 0" y "Rotate: 180"
4. Verificar que UI se ve bien en ambas

## Build Targets

### `platformio.ini` Ambientes

```ini
[env:cyd]
platform = espressif32
board = esp32dev
framework = arduino
```

Dos ambientes disponibles:
- `cyd` - Build normal para CYD
- `cyd_touchtest` - Build con test de calibración táctil

## Cambios Recientes (v1.0.0)

### Modificaciones en main.cpp
1. **Línea 88:** Variable `rot180` → `currentRot`
2. **Línea 1161-1200:** Funciones de rotación refactorizadas
3. **Línea 1412-1445:** HandleTap adaptable
4. **Línea 1559:** Inicialización de rotación
5. **Múltiples líneas:** Funciones drawXXX refactorizadas

### Nuevas Funciones
- `cycleRotation()` - Cicla entre rotaciones
- `isPortrait()` - Detecta portrait
- `getLogicalWidth()` - Ancho dinámico
- `getLogicalHeight()` - Alto dinámico
- `getRotationLabel()` - Label de rotación

## Problemas Comunes

### "Pantalla rotada 90°"
**Solución:** Verificar que `tft.setRotation()` usa valores 1 y 3, no 0 y 2.

### "Touch no responde en rotación"
**Solución:** Asegurar que las coordenadas se transforman en el switch statement de handleTap.

### "Firmware demasiado grande"
**Solución:** 
1. Remover fuentes suavizadas innecesarias
2. Comprimir imágenes/iconos
3. Optimizar con `platformio run -t release`

## Recursos

- [PlatformIO Documentation](https://docs.platformio.org/)
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32)
- [TFT_eSPI Library](https://github.com/Bodmer/TFT_eSPI)
- [D&D 5.2.1 SRD](https://www.dndbeyond.com/sources)

## Contacto

Para preguntas de desarrollo, abrir un issue en GitHub.

---

*Última actualización: 2026-11-02*
