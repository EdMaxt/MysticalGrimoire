# Changelog

Todos los cambios notables en este proyecto se documentan en este archivo.

## [1.0.0] - 2026-11-02

### Agregado
- ✨ Soporte para rotación de pantalla en 2 orientaciones (0° y 180°)
- ✨ Botón "Rotate" en pantalla de opciones
- ✨ Persistencia de rotación en NVS (Non-Volatile Storage)
- ✨ Transformación de coordenadas táctiles para ambas rotaciones
- ✨ Funciones auxiliares para detectar orientación (isPortrait, getLogicalWidth)
- ✨ Refactorización completa del layout para ser más adaptable
- ✨ Documentación técnica en Obsidian y GitHub

### Mejorado
- 🔧 Optimización del firmware (531 KB)
- 🔧 Mejor alineación de botones en pantalla principal
- 🔧 Tabs de Spells/Spellbooks ahora se adaptan dinámicamente
- 🔧 UI completamente alineado en ambas orientaciones
- 🔧 Touch responde correctamente en todas las posiciones

### Corregido
- 🐛 Arreglado mapeo de rotaciones TFT_eSPI
- 🐛 Coordenadas táctiles ahora funcionan correctamente en ambas rotaciones
- 🐛 Posición de botones ya no superpuesta

## [0.8.0] - 2026-10-30

### Agregado
- Versión inicial del proyecto
- 339 hechizos del SRD 5.2.1
- Soporte para Neon Odyssey (opcional)
- Control de brillo ajustable
- Sleep inteligente y deep sleep
- Libros de hechizos personalizados

### Características
- Interfaz táctil capacitiva
- Modo oscuro optimizado
- Búsqueda de hechizos por clase
- Visualización de detalles completos
- Upcast de hechizos
- Gestión de subclases

---

## Notas de Versión

### Versión 1.0.0
La versión 1.0.0 marca la implementación completa de rotación dual y refinamiento de UI. Todos los componentes están testeados y funcionando correctamente.

**Cambios principales:**
- Rotación funcionalmente completa en landscape (0° y 180°)
- Touch completamente funcional en ambas orientaciones
- UI perfectamente alineado
- Documentación exhaustiva

**Estado:** ✅ Stable - Listo para producción

---

## Formato de Commits

Este proyecto sigue estas convenciones:
- `✨ feat:` Nuevas características
- `🔧 improve:` Mejoras a características existentes
- `🐛 fix:` Correcciones de bugs
- `📚 docs:` Cambios en documentación
- `🧪 test:` Agregación o modificación de tests
- `♻️ refactor:` Refactorización de código

---

*Para más detalles ver [DEVELOPMENT.md](DEVELOPMENT.md)*
