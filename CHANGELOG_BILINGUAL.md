# Changelog / Registro de Cambios

[**English**](#english) | [**Español**](#español)

---

## English

All notable changes to this project are documented in this file.

### [1.0.0] - 2026-11-02

#### Added
- ✨ Dual screen rotation support (0° and 180°)
- ✨ Rotate button in Options screen
- ✨ Touch coordinate transformation for both rotations
- ✨ Rotation persistence in NVS (Non-Volatile Storage)
- ✨ Dynamic helper functions for orientation detection
- ✨ Complete layout refactoring for adaptive rendering
- ✨ Comprehensive technical documentation
- ✨ Bilingual documentation (English/Spanish)
- ✨ Complete licensing information (MIT, CC BY 4.0, Neon Odyssey)

#### Improved
- 🔧 Firmware optimization (531 KB)
- 🔧 Better button alignment on home screen
- 🔧 Dynamic spell/spellbook tab adaptation
- 🔧 Perfect UI alignment in both orientations
- 🔧 Touch response correct at all screen positions

#### Fixed
- 🐛 Fixed TFT_eSPI rotation mapping (using 1,3 instead of 0,2)
- 🐛 Touch coordinates now work correctly in both rotations
- 🐛 Button positioning no longer overlapped

#### Technical Changes
- Modified `rot180` variable to `currentRot` (supports extended states)
- Refactored `setRotation()` to handle landscape-only modes
- Updated touch transformation switch statement for all rotations
- Refactored UI draw functions for dynamic positioning
- Updated NVS initialization to constrain to 0 or 2

### [0.8.0] - 2026-10-30

#### Initial Release
- Initial project version
- 300+ spells from D&D 5.2.1 SRD
- Neon Odyssey support (optional)
- Adjustable brightness (5 levels)
- Smart sleep and deep sleep modes
- Custom spellbooks

#### Features
- Capacitive touch interface
- Dark mode optimized
- Spell search by class
- Complete spell details with full description
- Spell upcasting
- Subclass management

---

## Español

Todos los cambios notables en este proyecto están documentados en este archivo.

### [1.0.0] - 2026-11-02

#### Agregado
- ✨ Soporte para rotación de pantalla dual (0° y 180°)
- ✨ Botón de rotación en pantalla de opciones
- ✨ Transformación de coordenadas táctiles para ambas rotaciones
- ✨ Persistencia de rotación en NVS (Non-Volatile Storage)
- ✨ Funciones auxiliares dinámicas para detección de orientación
- ✨ Refactorización completa del layout para renderizado adaptativo
- ✨ Documentación técnica exhaustiva
- ✨ Documentación bilingüe (Inglés/Español)
- ✨ Información completa de licencias (MIT, CC BY 4.0, Neon Odyssey)

#### Mejorado
- 🔧 Optimización de firmware (531 KB)
- 🔧 Mejor alineación de botones en pantalla principal
- 🔧 Adaptación dinámica de pestañas de hechizos/libros
- 🔧 Alineación de UI perfecta en ambas orientaciones
- 🔧 Respuesta táctil correcta en todas las posiciones

#### Corregido
- 🐛 Mapeo correcto de rotaciones TFT_eSPI (usando 1,3 en lugar de 0,2)
- 🐛 Las coordenadas táctiles ahora funcionan correctamente en ambas rotaciones
- 🐛 Posicionamiento de botones ya no superpuesto

#### Cambios Técnicos
- Variable `rot180` modificada a `currentRot` (soporta estados extendidos)
- `setRotation()` refactorizado para manejar modos solo landscape
- Declaración switch de transformación táctil actualizada para todas las rotaciones
- Funciones de dibujo de UI refactorizadas para posicionamiento dinámico
- Inicialización de NVS actualizada para limitar a 0 o 2

### [0.8.0] - 2026-10-30

#### Versión Inicial
- Versión inicial del proyecto
- 300+ hechizos del SRD 5.2.1 de D&D
- Soporte Neon Odyssey (opcional)
- Brillo ajustable (5 niveles)
- Modos de sleep inteligente y deep sleep
- Libros de hechizos personalizados

#### Características
- Interfaz de táctil capacitivo
- Modo oscuro optimizado
- Búsqueda de hechizos por clase
- Detalles completos de hechizos con descripción completa
- Amplificación de hechizos
- Gestión de subclases

---

## Version Format / Formato de Versión

This project follows [Semantic Versioning](https://semver.org/) conventions:
`MAJOR.MINOR.PATCH`

Este proyecto sigue las convenciones de [Versionado Semántico](https://semver.org/):
`MAJOR.MINOR.PATCH`

## Commit Format / Formato de Commits

Commits use emoji prefixes:
- `✨ feat:` New features / Nuevas características
- `🔧 improve:` Improvements / Mejoras
- `🐛 fix:` Bug fixes / Correcciones
- `📚 docs:` Documentation changes / Cambios en documentación
- `🧪 test:` Tests / Tests
- `♻️ refactor:` Code refactoring / Refactorización de código

---

*Last updated / Última actualización: 2026-11-02*
