# Guía Rápida: Subir Firmware a CYD 3.2"

**Versión del Firmware:** v1.0.1  
**Dispositivo:** CYD ESP32-32E 3.2"  
**Estado:** ✅ Compilado y Listo

---

## ⚡ Proceso Rápido (5 minutos)

### 1. Preparar Hardware

```
☐ Pantalla 3.2" conectada a ESP32 (mismos pines que 2.8")
☐ USB conectado a computadora
☐ Batería desconectada (inicialmente)
```

### 2. Detectar Puerto

```bash
# Windows - Listar puertos COM
wmic logicaldisk get name

# Ver dispositivos conectados
platformio device list
```

Busca el ESP32 en la lista (generalmente COM3, COM4, etc.)

### 3. Upload Manual

```bash
cd C:\Users\edmax\Projects\Mystical_Grimoire\Mystical_Grimoire

# Auto-detectar puerto
platformio run -e cyd --target upload

# O especificar puerto
platformio run -e cyd --target upload --upload-port COM3
```

**Salida esperada:**
```
Retrieving maximum program size...
Uploading .pio\build\cyd\firmware.bin
Upload complete
```

### 4. Verificar en Dispositivo

```
✓ Pantalla muestra logo/home screen
✓ Botones responden
✓ Batería muestra ícono verde (cuando se carga)
```

---

## 🔧 Si Algo Falla

### Pantalla Negra
```bash
# 1. Limpiar build
platformio run -t clean -e cyd

# 2. Recompilar
platformio run -e cyd

# 3. Re-upload
platformio run -e cyd --target upload
```

### Puerto No Detectado
```bash
# Instalar drivers USB (CH340 o equivalente)
# 1. Desconectar USB
# 2. Esperar 3 segundos
# 3. Reconectar USB
# 4. Intentar de nuevo
```

### Timeout en Upload
```bash
# Aumentar timeout
platformio run -e cyd --target upload --upload-speed 115200

# O presionar BOOT botón durante upload
```

---

## 📊 Monitor Serial

Después de upload, ver el terminal:

```bash
platformio device monitor -e cyd -b 115200
```

Salida esperada:
```
[BOOT] Loading spellbook from SD...
[OK] 339 spells loaded
[BATTERY] Initial voltage: 4.15V
[DISPLAY] Resolution: 240x320 (Landscape)
```

---

## ✅ Confirmación Post-Upload

Crear archivo `3.2_TESTED.txt` cuando funcione todo:

```
Fecha: 2026-11-06
Dispositivo: CYD ESP32-32E 3.2"
Firmware: v1.0.1
Status: ✅ FUNCIONANDO

Verificado:
✓ Home screen visible
✓ Touch responde
✓ Botones clickeables
✓ Spells cargan desde SD
✓ Batería detectada
✓ Rotación 0°/180° funciona
✓ Deep sleep responde
```

---

## 📚 Troubleshooting Avanzado

### Si necesitas recalibración táctil:

```bash
platformio run -e cyd_touchtest --target upload

# Después de calibrar, volver a compilar normal:
platformio run -e cyd --target upload
```

### Ver logs detallados:

```bash
platformio run -v -e cyd --target upload 2>&1 | tee upload.log
```

### Reset de fábrica (si es necesario):

```bash
# Conectar a puerto serial
platformio device monitor -e cyd -b 115200

# En el monitor, presionar ENTER para ver comandos
# Escribir: erase_all_nvs
# Esto borra todas las configuraciones guardadas
```

---

## 🎯 Verificación Final

```bash
# Comprobar que el firmware está en el dispositivo
platformio run -e cyd --target erase_flash

# Re-upload completo
platformio run -e cyd --target upload

# Ver serial output
platformio device monitor -e cyd -b 115200
```

---

*Guía preparada: 2026-11-06*
*Compatible con Mystical Grimoire v1.0.1*

¿Necesitas ayuda? Revisa CYD_3.2_SETUP.md para troubleshooting detallado.
