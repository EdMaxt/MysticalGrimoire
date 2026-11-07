# Setup para CYD ESP32-32E 3.2"

**Estado:** ✅ Compatible con firmware actual  
**Fecha:** 2026-11-06

---

## 📋 Verificación de Compatibilidad

La pantalla 3.2" es **idéntica a nivel de firmware** a la 2.8". No requiere cambios de código.

### Comparativa de Pines
```
PANTALLA 2.8" vs PANTALLA 3.2"

Display (LCD):
  IO15 - LCD_CS      (IGUAL)
  IO2  - LCD_DC      (IGUAL)
  IO14 - LCD_CLK     (IGUAL)
  IO13 - LCD_MOSI    (IGUAL)
  IO12 - LCD_MISO    (IGUAL)
  IO27 - LCD_BL      (IGUAL)

Touch (XPT2046):
  IO33 - TOUCH_CS    (IGUAL)
  IO36 - TOUCH_INT   (IGUAL)
  IO14 - TOUCH_CLK   (IGUAL - compartido)
  IO13 - TOUCH_MOSI  (IGUAL - compartido)
  IO12 - TOUCH_MISO  (IGUAL - compartido)

Otros:
  IO34 - BAT_ADC     (IGUAL)
  IO22, IO16, IO17 - RGB LED (IGUAL)
```

### ⚠️ NOTA IMPORTANTE: Backlight Pin

En `platformio.ini` línea 28 está configurado:
```ini
-DTFT_BL=21
```

Pero según la documentación oficial de 3.2", el backlight está en **IO27**.

**Solución:** Vamos a verificar y actualizar si es necesario.

---

## 🔧 Pasos de Setup

### 1. Verificar Configuración Actual

```bash
cd C:\Users\edmax\Projects\Mystical_Grimoire\Mystical_Grimoire

# Ver la configuración actual
cat platformio.ini | grep -A 30 "\[env:cyd\]"
```

### 2. Actualizar platformio.ini si es necesario

**Si el backlight está en IO21 en tu CYD 2.8":** Mantener igual  
**Si el backlight está en IO27 en la 3.2":** Actualizar

```ini
; ANTES (si usaste 2.8" original)
-DTFT_BL=21

; DESPUÉS (para 3.2" estándar)
-DTFT_BL=27
```

### 3. Verificar config.h

Abrir `src/config.h` y revisar:

```cpp
#define SCREEN_W 320      // Ancho
#define SCREEN_H 240      // Alto (en landscape)
```

Estos valores son **correctos para ambas** (240×320 de resolución, pero mostrados como landscape).

### 4. Compilación

```bash
# Compilar firmware
platformio run -e cyd

# Si hay errores, revisar log completo
platformio run -v -e cyd
```

### 5. Prueba de Calibración Táctil

**Muy importante:** Aunque los pines son iguales, la pantalla es ligeramente más grande (3.2" vs 2.8").

```bash
# Compilar con calibración táctil
platformio run -e cyd_touchtest --target upload

# El dispositivo mostrará un punto de calibración
# Presionar en las esquinas cuando se indique
```

### 6. Upload Normal

```bash
# Después de calibrar, compilar versión normal
platformio run -e cyd --target upload
```

---

## 🔌 Conexión Física

### Paso 1: Preparación
```
1. Desconectar USB del ESP32
2. Desconectar batería (si está conectada)
3. Esperar 10 segundos
```

### Paso 2: Cambio de Pantalla
```
ANTES de desconectar cables, FOTOGRAFIAR la conexión actual
para referencia.

Desconectar cables de pantalla 2.8" en el siguiente orden:
  1. VCC (rojo/+)
  2. GND (negro/-)
  3. SPI CLK (naranja)
  4. SPI MOSI (amarillo)
  5. SPI MISO (verde)
  6. CS (azul)
  7. DC (violeta)
  8. RST (gris) - si es necesario

Conectar cables de pantalla 3.2" en el MISMO orden.
```

### Paso 3: Verificación Visual
```
Revisar que:
  ✓ No hay cables sueltos
  ✓ Polaridad correcta (+ a VCC, - a GND)
  ✓ Todos los cables están asegurados
  ✓ No hay cortocircuitos visibles
```

### Paso 4: Test de Potencia
```
1. Conectar USB (sin batería primero)
2. El ESP32 debe encenderse
3. Si algo se calienta → DESCONECTAR inmediatamente
```

---

## 📊 Checklist de Verificación Post-Setup

- [ ] Pantalla muestra imagen (no negra)
- [ ] Home screen visible
- [ ] Botones responden al tocar
- [ ] Rotación funciona (0° y 180°)
- [ ] Battery indicator funciona
- [ ] Spells cargan correctamente
- [ ] Touch es preciso en todas las esquinas
- [ ] Deep sleep funciona
- [ ] SD card es detectada

---

## 🐛 Troubleshooting

### Pantalla Negra
```
1. Revisar voltaje USB (debe ser 5V)
2. Revisar que pin IO27 de backlight esté correctamente configurado
3. Revisar pines de CS y DC
4. Intentar reset (botón RESET en la placa)
```

### Touch No Responde
```
1. Ejecutar cyd_touchtest y recalibrar
2. Revisar pines TOUCH_CS (IO33) y TOUCH_INT (IO36)
3. Si aún falla, verificar voltaje SPI (3.3V)
```

### Firmware No Compila
```
1. Limpiar build: platformio run -t clean -e cyd
2. Actualizar librerías: platformio lib update
3. Revisar que platformio.ini sea válido (sin caracteres extra)
```

### USB No Detecta Carga
```
1. Conectar batería LiPo (3.7V, 2-3Ah)
2. Presionar + en Options para calibrar voltaje
3. Comparar con multímetro en batería
4. Si sigue fallando, considerar soldar CHRG pin (opcional)
```

---

## 📚 Documentación de Referencia

- **Pantalla 3.2" oficial:** https://www.lcdwiki.com/3.2inch_ESP32-32E_Display
- **Especificación técnica:** E32R32P Specification V1.0.pdf
- **Driver LCD:** ST7789P3 (igual que 2.8")
- **Touch IC:** XPT2046 (igual que 2.8")

---

## ✅ Confirmación de Setup Completo

Una vez que todo funcione:

```bash
# 1. Compilar versión final
platformio run -e cyd

# 2. Subir a dispositivo
platformio run -e cyd --target upload

# 3. Verificar que inicia correctamente
platformio device monitor -e cyd -b 115200

# 4. Documentar éxito
# Crear archivo de log con:
#   - Fecha y hora
#   - Versión de firmware
#   - Resolución de pantalla (verificar en serial output)
#   - Toma de pantalla del home screen
```

---

*Setup preparado: 2026-11-06*
*Compatible con Mystical Grimoire v1.0.1*
