# Mystical Grimoire 🧙‍♂️

[![License: MIT/CC BY 4.0](https://img.shields.io/badge/License-MIT%2FCC%20BY%204.0-lightgrey.svg)](LICENSE)
[![Version](https://img.shields.io/badge/Version-1.0.0-blue.svg)]()
[![Platform](https://img.shields.io/badge/Platform-ESP32-green.svg)]()
[![Status](https://img.shields.io/badge/Status-Stable-brightgreen.svg)]()

[**English**](#english) | [**Español**](#español)

---

## English

### Overview

D&D 2024 companion application for the CYD ESP32 touchscreen. Features:

- **Spell Browser**: Browse 300+ spells from D&D 5.2.1 SRD organized by class
- **MicroSD Storage**: All spells stored on microSD card for offline access
- **Neon Odyssey Support**: Optional playtest content with additional classes and subclasses
- **Spellbooks**: Create and manage personal character spellbooks
- **Touch Interface**: Full capacitive touch screen support with rotation
- **Battery Indicator**: Real-time battery level and charging status
- **Adjustable Display**: Brightness control and sleep modes

**Current Status v1.0.0:** ✅ Dual rotation fully functional (0° and 180°), perfect UI alignment, correct touch response in both orientations, optimized firmware (531 KB). Tested and stable on hardware.

### Quick Installation

1. **MicroSD (FAT32):** Copy `SD_CARD/grimoire` to the root of your card (creates `/grimoire/index/...` and `/grimoire/spells/...`). For Neon Odyssey, also copy `SD_CARD/neon` (optional).
2. **Project:** Copy this folder to your PlatformIO workspace and open in VSCode
3. **Calibration:** Upload with `cyd_touchtest` environment and follow on-screen instructions to calibrate touch
4. **Build & Upload:** Switch to `cyd` environment, insert microSD, and upload

### Feature Checklist

- [x] **Dual Rotation v1.0.0** - Landscape 0° and 180° with perfect touch alignment
- [x] **Brightness Control** - 5 levels with persistent storage
- [x] **Smart Sleep** - Screen off and deep sleep modes
- [x] **Battery Indicator** - Real-time voltage display and charging icon
- [x] **D&D 5.2.1 SRD** - 300+ spells with complete descriptions
- [x] **Neon Odyssey Support** - Optional playtest content
- [x] **Spellbook Management** - Create, save, and manage character spellbooks
- [ ] Multi-class support and manual slot editing
- [ ] on-screen keyboard with accent support
- [ ] Spell search and favorites system
- [ ] Full D&D spell mechanics display
- [ ] Vertical orientation (90°/270°) - requires complete layout redesign

### Technical Details

**Architecture:**
- Language: C++ (Arduino framework)
- Platform: ESP32 (CYD - Cheap Yellow Display)
- Display: ILI9341 320×240 pixel touchscreen
- Storage: MicroSD card (FAT32)
- Data Format: Binary index + text spell files
- Persistence: NVS (Non-Volatile Storage)

**Build Requirements:**
- PlatformIO Core
- Python 3.7+
- Git

**License Notes:**
- Application Code: MIT License
- D&D 5.2.1 SRD Content: CC BY 4.0 (Wizards of the Coast)
- Neon Odyssey Content: Personal use only (playtest materials)

### Documentation

- **[DEVELOPMENT.md](DEVELOPMENT.md)** - Development guide, code structure, and modification guide
- **[CHANGELOG.md](CHANGELOG.md)** - Complete version history
- **[GITHUB_SETUP.md](GITHUB_SETUP.md)** - Repository setup and workflow instructions
- **[LICENSE](LICENSE)** - Complete licensing information (MIT, CC BY 4.0, Neon Odyssey)

---

## Español

### Descripción General

Aplicación compañera de D&D 2024 para la pantalla táctil CYD ESP32. Características:

- **Navegador de Hechizos**: Explora 300+ hechizos del SRD 5.2.1 de D&D organizados por clase
- **Almacenamiento en MicroSD**: Todos los hechizos guardados en tarjeta microSD para acceso sin conexión
- **Soporte Neon Odyssey**: Contenido de playtest opcional con clases y subclases adicionales
- **Libros de Hechizos**: Crea y gestiona libros de hechizos personales de personajes
- **Interfaz Táctil**: Soporte completo de pantalla táctil capacitiva con rotación
- **Indicador de Batería**: Nivel de batería en tiempo real e indicador de carga
- **Pantalla Ajustable**: Control de brillo y modos de ahorro de energía

**Estado Actual v1.0.0:** ✅ Rotación dual completamente funcional (0° y 180°), UI perfectamente alineado, respuesta táctil correcta en ambas orientaciones, firmware optimizado (531 KB). Testeado y estable en placa.

### Instalación Rápida

1. **MicroSD (FAT32):** Copia `SD_CARD/grimoire` a la raíz de la tarjeta (crea `/grimoire/index/...` y `/grimoire/spells/...`). Para Neon Odyssey, también copia `SD_CARD/neon` (opcional).
2. **Proyecto:** Copia esta carpeta a tu espacio de trabajo de PlatformIO y abre en VSCode
3. **Calibración:** Sube con entorno `cyd_touchtest` y sigue las instrucciones en pantalla para calibrar el táctil
4. **Compilación y Subida:** Cambia a entorno `cyd`, inserta la microSD y sube

### Lista de Verificación

- [x] **Rotación Dual v1.0.0** - Landscape 0° y 180° con alineación táctil perfecta
- [x] **Control de Brillo** - 5 niveles con almacenamiento persistente
- [x] **Sleep Inteligente** - Modos de apagar pantalla y deep sleep
- [x] **Indicador de Batería** - Visualización de voltaje en tiempo real e icono de carga
- [x] **D&D 5.2.1 SRD** - 300+ hechizos con descripciones completas
- [x] **Soporte Neon Odyssey** - Contenido de playtest opcional
- [x] **Gestión de Libros de Hechizos** - Crear, guardar y gestionar libros de personajes
- [ ] Soporte multiclase y edición manual de slots
- [ ] Teclado en pantalla con soporte de acentos
- [ ] Búsqueda de hechizos y sistema de favoritos
- [ ] Visualización completa de mecánicas de hechizos D&D
- [ ] Orientación vertical (90°/270°) - requiere rediseño completo de layouts

### Detalles Técnicos

**Arquitectura:**
- Lenguaje: C++ (framework Arduino)
- Plataforma: ESP32 (CYD - Cheap Yellow Display)
- Pantalla: ILI9341 320×240 píxeles con táctil
- Almacenamiento: Tarjeta microSD (FAT32)
- Formato de Datos: Índice binario + archivos de texto
- Persistencia: NVS (Non-Volatile Storage)

**Requisitos de Compilación:**
- PlatformIO Core
- Python 3.7+
- Git

**Notas de Licencia:**
- Código de Aplicación: Licencia MIT
- Contenido D&D 5.2.1 SRD: CC BY 4.0 (Wizards of the Coast)
- Contenido Neon Odyssey: Solo uso personal (materiales de playtest)

### Documentación

- **[DEVELOPMENT.md](DEVELOPMENT.md)** - Guía de desarrollo, estructura de código y guía de modificaciones
- **[CHANGELOG.md](CHANGELOG.md)** - Historial completo de versiones
- **[GITHUB_SETUP.md](GITHUB_SETUP.md)** - Instrucciones de configuración del repositorio y workflow
- **[LICENSE](LICENSE)** - Información completa de licencias (MIT, CC BY 4.0, Neon Odyssey)

---

## Important Legal Notices / Avisos Legales Importantes

### English

**D&D Content Attribution:** This project uses content from Dungeons & Dragons 5th Edition System Reference Document, which is copyright © 2016 Wizards of the Coast LLC. This content is used under the Creative Commons Attribution 4.0 International License.

**Neon Odyssey Content Notice:** Neon Odyssey playtest materials included in this project are used for personal, non-commercial purposes only. Redistribution or commercial use is prohibited.

**Disclaimer:** Mystical Grimoire is an unofficial fan project and is not affiliated with Wizards of the Coast LLC or Neon Odyssey creators. All D&D references are used for fan purposes only.

### Español

**Atribución Contenido D&D:** Este proyecto utiliza contenido del Documento de Referencia del Sistema de Dungeons & Dragons 5ª Edición, que es propiedad intelectual © 2016 Wizards of the Coast LLC. Este contenido se utiliza bajo la Licencia Internacional de Atribución de Creative Commons 4.0.

**Aviso Contenido Neon Odyssey:** Los materiales de playtest de Neon Odyssey incluidos en este proyecto se utilizan únicamente con fines personales y no comerciales. La redistribución o uso comercial está prohibido.

**Descargo de Responsabilidad:** Mystical Grimoire es un proyecto de fans no oficial y no está afiliado con Wizards of the Coast LLC ni con los creadores de Neon Odyssey. Todas las referencias a D&D se utilizan únicamente con fines de fans.

---

## Support / Soporte

For issues, questions, or contributions, please open an issue on GitHub.
Para problemas, preguntas o contribuciones, abre un issue en GitHub.

**Repository:** https://github.com/EdMaxt/MysticalGrimoire

---

*Última actualización / Last updated: 2026-11-02*
