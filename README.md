# Mystical Grimoire 🧙‍♂️

[![License: MIT/CC BY 4.0](https://img.shields.io/badge/License-MIT%2FCC%20BY%204.0-lightgrey.svg)](LICENSE)
[![Version](https://img.shields.io/badge/Version-1.1.0-blue.svg)]()
[![Platform](https://img.shields.io/badge/Platform-ESP32_CYD_2.8%22_%26_Freenove_3.2%22-green.svg)]()
[![Status](https://img.shields.io/badge/Status-Stable_&_Optimized-brightgreen.svg)]()

[**English**](#english) | [**Español**](#español)

---

## English

### Overview

D&D 2024 companion application for the CYD ESP32 touchscreen: browse spells from the D&D 5.2.1 SRD organized by class, read from microSD card, plus an optional second spellbook with classes and subclasses from **Neon Odyssey** (playtest). The app interface is in English.

> **Estado v1.1.0:** ✅ **Dual-board support optimizado** - CYD 2.8" (ILI9341) + Freenove FNK0114 3.2" (ST7789) con soporte completo. Display rendering perfecto, ghosting 100% eliminado en 3.2", touch responsivo, performance fluida. Firmware: 535 KB. Testeado y estable en ambas placas.
>
> **v1.0.0:** ✅ Rotación dual (0°/180°), UI alineado, touch perfecto, firmware optimizado (531 KB).
>
> **Historial:** v0.8 con soft sleep (Options > Screen off), v0.7 con batería, v0.6 con iconos, v0.5 con hechizos, v0.4 con Options y persistencia en NVS.

## Instalación rápida

1. **microSD (FAT32):** copia `SD_CARD/grimoire` a la raíz de la tarjeta (queda `/grimoire/index/...` y `/grimoire/spells/...`). Para Neon Odyssey copia también `SD_CARD/neon` (queda `/neon/...`); es opcional: sin esa carpeta la app funciona solo con el SRD.
2. **Proyecto:** copia esta carpeta a `C:\Users\edmax\Projects\Mystical_Grimoire` y ábrela en VSCode (PlatformIO IDE).
3. Si PlanarNexus usa una línea `platform = ...` específica (core Arduino 3.x), cópiala a `platformio.ini`.
4. Selecciona el entorno **`cyd_touchtest`** y haz Upload. Toca las esquinas: debe dibujar puntos donde tocas. Ajusta `TOUCH_RAW_*`, `TOUCH_SWAP_XY` y `TOUCH_INVERT_*` en `src/config.h` hasta que coincida (puedes copiar los valores de PlanarNexus).
5. Cambia al entorno **`cyd`**, inserta la microSD y haz Upload.

## Lista de verificación en la placa

- [ ] Compila sin errores con PlatformIO.
- [ ] La pantalla se ve bien (si no: `-DINVERT_DISPLAY` o `-DTFT_RGB_ORDER=TFT_BGR` en `platformio.ini`).
- [ ] La SD se monta. Si sale "Could not mount": revisa los pines `SD_*` de `src/config.h` contra el PINS.md del repo de la CYD.
- [ ] El touch responde y coincide con lo que dibuja la prueba.
- [ ] Home -> clase -> pestañas de nivel -> lista -> detalle -> volver.
- [ ] Scroll por arrastre y con las flechas.
- [ ] Home -> Options -> elegir Neon Odyssey: el Home pasa a 13 clases (colores cian/magenta). Reiniciar: debe recordar la elección.
- [ ] Clase Neon con hechizos -> botón Sub -> lista de subclases -> lectura. Clases sin hechizos (Disciple, Juggernaut, Scoundrel, Soldier) abren directo sus subclases.
- [ ] Un reskin (p. ej. Enforcer > Bolster) muestra `Reskin of: Bless`.
- [ ] Options -> Brightness `-`/`+` cambia el brillo (5 niveles) y se recuerda al reiniciar.
- [ ] Options -> Sleep now apaga pantalla y LED; BOOT (toque corto) despierta la placa. Mantener BOOT 2 s desde cualquier pantalla también entra en sleep.
- [ ] Home -> Spellbooks -> crear un personaje (ambos libros), reiniciar y comprobar que sigue en la lista con sus slots usados.
- [ ] Batería: icono en el Home, voltaje en Options coincide con el multímetro tras calibrar, el rayo aparece al conectar USB.
- [ ] Iconos: tiles del Home (SRD y Neon), franja de clases sin hechizos, fondo atenuado en la pantalla de clase, scroll fluido con el fondo.
- [x] **Options > Rotate:** ✅ COMPLETADO - gira la pantalla entre 0° (landscape) y 180° (landscape invertido), el touch sigue coincidiendo perfectamente y se recuerda al reiniciar. Implementado con mapeo correcto de TFT_eSPI (rotaciones 1 y 3) y transformación de coordenadas táctiles para ambas orientaciones.
- [ ] Neon: las clases sin hechizos salen en la franja de abajo del Home.
- [ ] Teclado en pantalla: mayúscula tras espacio, Del, Cancel, OK.
- [ ] Hoja: casillas de slots, Long/Short rest, pestaña All (casilla preparar / nombre abrir), Always, Cast con upcast.
- [ ] En el SRD, los hechizos en gris (p. ej. Wizard > Blade Ward) son del PHB y muestran el aviso de que su texto no está incluido.

## Rotación Dual v1.0.0 ✨

La versión 1.0.0 implementa rotación completamente funcional en **dos orientaciones** (landscape):

- **0° (Normal):** Landscape horizontal estándar (320×240)
- **180° (Invertido):** Landscape girado 180 grados 

### Características:
- ✅ **Botón "Rotate"** en Options que alterna entre 0° y 180°
- ✅ **Touch perfecto** - Coordenadas transformadas automáticamente en ambas rotaciones
- ✅ **UI completamente alineado** - Todos los botones, tabs y elementos se posicionan correctamente
- ✅ **Persistencia** - La rotación elegida se guarda en NVS y se recuerda al reiniciar
- ✅ **Firmware optimizado** - 531 KB, funciona perfectamente en la placa

### Implementación Técnica:
- **Mapeo TFT_eSPI:** Rotaciones 1 (0°) y 3 (180°) para landscape correcto
- **Coordenadas táctiles:** Switch statement que transforma coordenadas según la rotación actual
- **NVS Storage:** Clave `"rot"` con valores 0 ó 2 (mapeados a 1 ó 3 en TFT_eSPI)
- **Layout responsivo:** Funciones dinámicas para reposicionar elementos

> **Nota:** Rotación vertical (90°/270° portrait) no está implementada; requeriría rediseñar completamente los layouts. La versión actual optimiza landscape en ambas direcciones.

Para más detalles sobre la implementación, ver [DEVELOPMENT.md](DEVELOPMENT.md) y [CHANGELOG.md](CHANGELOG.md).

---

## Dual-Board Support v1.1.0 ✨

Mystical Grimoire ahora soporta **dos variantes de hardware** con optimizaciones específicas para cada display:

### Supported Boards

| Board | Display | Driver | Status | Key Features |
|-------|---------|--------|--------|--------------|
| **CYD 2.8"** | 240×320 ILI9341 | ILI9341_2 | ✅ Tested | Software SPI touch, stable, proven |
| **Freenove FNK0114 3.2"** | 240×320 ST7789 | ST7789 | ✅ New! | Integrated touch, optimized rendering |

### v1.1.0 Optimizations (FNK0114 3.2")

- ✅ **Display Rendering:** 100% ghosting elimination via gamma curves (ST7789 commands 0xE0/0xE1)
- ✅ **SPI Optimization:** 80 MHz display SPI (5.3x faster than initial 15 MHz)
- ✅ **Touch Performance:** 2.5 MHz optimized XPT2046 polling, highly responsive
- ✅ **DMA Enabled:** Frame buffer acceleration for smooth transitions
- ✅ **Color Accuracy:** Reduced brightness levels prevent over-saturation
- ✅ **Backward Compatible:** All changes maintain CYD 2.8" functionality

### Building for Your Board

**For CYD 2.8":**
```bash
pio run -e cyd -t upload
```

**For Freenove FNK0114 3.2":**
```bash
pio run -e cyd32 -t upload
```

### Configuration

Each board has its own `platformio.ini` environment with optimal settings:

- `[env:cyd]` - CYD 2.8" with ILI9341 and software SPI touch
- `[env:cyd32]` - Freenove FNK0114 3.2" with ST7789 and integrated touch
- `[env:cyd_touchtest]` - Touch calibration for 2.8"
- `[env:cyd32_touchtest]` - Touch calibration for 3.2"

For detailed optimization documentation, see **[Mystical Grimoire - FNK0114 Optimizations](docs/OPTIMIZATIONS.md)** in Obsidian vault or project docs.

---

## Spellbooks (v0.4)

Home tiene dos pestañas: **Spells** (listas por clase) y **Spellbooks** (personajes). Se guardan en la SD como `/characters/cNN.txt` (máx. 12, texto plano editable).

- **Crear**: `+ New spellbook` -> libro (D&D o Neon Odyssey) -> nombre (teclado en pantalla) -> especie -> clase (solo lanzadores) -> subclase (o None) -> si la subclase tiene opciones (p. ej. Circle of the Land: Arid/Polar/Temperate/Tropical) se elige una -> nivel y modificador. DC = 8 + bono de competencia + mod, Attack = bono + mod; se pueden ajustar a mano (`Auto DC/Atk` vuelve al cálculo).
- **Hoja**: pestañas `Slots | Prepared | All | Always`. La cabecera muestra DC, Atk, `Cantrips x/y` y `Prepared x/y` (en rojo si te pasas; es un aviso, no un bloqueo).
- **Slots**: casillas según la tabla de la clase y nivel. Tocar una casilla marca usadas hasta esa; tocar la última usada la libera. `Long rest` restaura todo; Warlock/Cryptist (pact) tienen `Short rest`.
- **All**: lista de la clase por nivel (solo niveles que tienes en slots). Casilla = preparado/conocido; el nombre abre el hechizo.
- **Always**: hechizos que da la clase o la subclase (por nivel y opción elegida). No cuentan para el límite de preparados.
- **Cast**: al abrir un hechizo desde la hoja (nivel 1+) aparece el botón `Cast`. Muestra los slots desde el nivel base hacia arriba (upcast) con los restantes; también `No slot (free/ritual)`. Con pact solo ofrece el nivel del pact.
- **Edit**: nombre, nivel, mod, DC, Atk, subclase y borrar (se pide tocar dos veces).

Límites conocidos: sin multiclase; los máximos de slots salen de la tabla (no se editan); en subclases Neon con varias columnas se tomó la primera como siempre-concedida y las demás como "elige una"; los hechizos solo-PHB no tienen texto.

## Texto de los 52 hechizos solo-PHB (opcional, tu propio libro)

El repositorio **no** incluye ese texto (derechos de autor). Si tienes el PDF del Manual del Jugador 2024 en español, puedes cargarlo en tu tarjeta:

```
cp -r SD_CARD SD_PERSONAL          (Windows: xcopy /E /I SD_CARD SD_PERSONAL)
python tools/import_phb_es.py "Manual_del_Jugador.pdf" SD_PERSONAL
```

Copia luego `SD_PERSONAL/grimoire` (y `neon`) a la microSD. El texto queda en español (no se traduce) y los hechizos dejan de verse atenuados. Con `--restore` se vuelven los avisos originales. Necesita `pdftotext` (poppler) o `pip install pdfplumber`. Algunos hechizos en páginas a dos columnas salen mezclados; el script no los escribe y te dice cuáles rellenar a mano (en la prueba con el PDF: Staggering Smite, Swift Quiver y Thunderous Smite). `SD_PERSONAL/` está en `.gitignore`: no subas esos archivos.

## Iconos de clase (v0.6)

- Home: cada tile muestra su icono (D&D: imagen plana de color; Neon: siluetas teñidas con el color de la clase). En Neon, los tiles compactos usan el icono como marca de agua tenue y las clases sin hechizos van en la franja de abajo.
- Pantalla de clase: arte detallado atenuado y desaturado como fondo; las filas pierden el panel y quedan con una línea fina para que se lea el texto.
- Los iconos se generan desde `assets/src/` con `python3 tools/build_icons.py SD_CARD` (necesita pillow, numpy, scipy). Salen en `SD_CARD/<libro>/icons/<clase>_{t,w,b}.bin` (RGB565 crudo: `t` 36x36, `w` 34x34, `b` 160x160). Si falta un archivo, ese tile se dibuja como antes.
- Para cambiar un icono: reemplaza la imagen fuente (mismo orden de cuadrícula, ver el script) y vuelve a correr el script. La intensidad del fondo está en `composite(..., 0.30, 0.55)` dentro de `write()`.
- El fondo no se usa con fuentes VLW (el texto transparente necesita las fuentes de mapa de bits).

## Batería (v0.7)

La placa (CYD de 2,8" con el conector LiPo de 1,25 mm) trae cargador TP4054 y mide la batería en **IO34**. La app lee el voltaje cada 2 s (promedio de 16 lecturas, suavizado) y lo convierte a porcentaje con la curva típica de una LiPo.

- **Home:** icono de batería arriba a la derecha con el porcentaje debajo. Verde con rayo al cargar, ámbar por debajo de 30% y rojo por debajo de 15%.
- **Barra superior:** una franja de 2 px en el borde de arriba, visible en todas las pantallas (largo = carga; verde al cargar, ámbar y rojo cuando baja). Esas 2 filas están reservadas.
- **Options:** junto a "Spell book" se ve `3.92V 78%` entre dos botones `-` `+` que calibran la lectura (0,5% por toque, se guarda). Compara con un multímetro en la batería.
- **Divisor:** `BAT_DIV` en `config.h` (2.0 = divisor 100k/100k). Si el voltaje se ve muy lejos del real, ajústalo ahí o con los botones.
- **Carga:** el TP4054 no saca su pin de estado, así que la app **estima** que carga cuando el voltaje sube o está en ~4,2 V (puede equivocarse unos segundos). Para una señal exacta, suelda un cable del pin CHRG del TP4054 a IO35 (solo entrada, sin pull-up interno: agrega 10k a 3,3 V) y pon `PIN_CHG_STATUS 35` en `config.h`.

## Cómo funciona

- **Touch por software:** en la CYD el slot de SD usa el bus VSPI, que en PlanarNexus ocupa el touch. Aquí el XPT2046 se lee con SPI por software (`src/touch_soft.h`) y la SD usa VSPI.
- **Datos en la SD:** `index/<clase>.idx` (`id|nivel|nombre|escuela|flags[|base]`), `index/classes.txt`, `spells/NNN.txt` y `ATTRIBUTION.txt`. Nada del texto se carga completo a RAM: se lee un hechizo a la vez.
- **Libro Neon Odyssey (`/neon`):** mismo formato, pero los ids llevan prefijo: `gNNN` = hechizo del SRD (se lee de `/grimoire/spells`, así no se duplica texto) y `nNNN` = hechizo nuevo (`/neon/spells`). Un *reskin* aparece con su nombre Neon (p. ej. Bolster) y el 6.º campo guarda el hechizo base (Bless); el texto mostrado es el del hechizo original. `/neon/sub/<clase>.idx` + `/neon/sub/NNN.txt` guardan las subclases (resumen y hechizos que otorgan). `classes.txt` de Neon añade `subclases|basada_en`.
- **Options:** en el Home, botón Options: libro (SRD / Neon), brillo y sleep. Se guarda en la memoria NVS (`Preferences`, espacio `grimoire`, claves `pack` y `bri`). Brillo: PWM en el backlight (GPIO 21) con 5 niveles (duty 51-255) y sleep igual que PlanarNexus: deep sleep con wake por BOOT (GPIO 0, `rtc_gpio_pullup_en`), y BOOT mantenido 2 s también lo activa (solo después de haberlo soltado una vez, para no volver a dormir al despertar). Al despertar la app reinicia en el Home. Requiere el core Arduino-ESP32 3.x (`ledcAttach`); con core 2.x usa `ledcSetup`/`ledcAttachPin` automáticamente.
- **Hechizos solo del PHB 2024:** el SRD 5.2.1 tiene 339 de los 391 hechizos del Manual del Jugador 2024. Los 52 restantes (Armor of Agathys, Summon Beast, Thunderous Smite...) están en `data/phb_only.json` y se agregan a las listas de clase como entradas **sin texto** (nombre, nivel, escuela, clases y marca Conc./Ritual; flag `P` en los `.idx`). Se ven en gris y al abrirlos dicen que el texto está en el libro. El texto de esos hechizos no es SRD, así que no se incluye ni se traduce.
- **Tipografía:** por defecto usa las fuentes integradas de TFT_eSPI y convierte los caracteres especiales a ASCII. Si existe `/grimoire/fonts/body.vlw` en la SD, la app lo carga como fuente suave (Unicode). Usa una fuente de ~12-14 pt e incluye `' " - * x ...` además de ASCII: `’ — “ ” • – − × …`.
- **Pantallas:** Home (8 o 13 clases), Options, Clase (pestañas de nivel y lista, botón Sub en Neon), Subclases, Lector (hechizo, subclase y About).

## Datos (se ejecutan en PC)

1. `tools/1_extract_pages.py` lee el PDF del SRD (páginas 107-175), columna por columna. Necesita `pdftotext` (poppler) y el PDF junto al script.
2. `tools/2_parse_spells.py` genera `data/spells.json` (339 hechizos).
3. `tools/3_build_sd.py` genera `SD_CARD/`.

Comprobación contra el PHB en español (`tools/phb_1_parse_es.py`, `phb_2_match.py`, `phb_3_make_only.py`): se extraen los 391 hechizos del PDF, se emparejan con los 339 del SRD por nivel, escuela, clases, alcance, tiempo, duración y números del texto, y se genera `tools/phb_crosscheck.csv` (nombre en español, nombre en inglés, estado). Resultado: los 339 hechizos del SRD están en el libro, con las mismas clases y tiempo de lanzamiento, y 52 son exclusivos del PHB. Los nombres en inglés de esos 52 los puse yo a mano (el PDF solo está en español): revísalos en el CSV. Después, `python3 tools/3_build_sd.py data/spells.json SD_CARD` regenera `SD_CARD/grimoire` sin tocar `/neon`.

Neon Odyssey (necesita los dos PDF junto a los scripts): `tools/parse_neon_spells.py` (95 hechizos nuevos, desde el texto extraído con pdftotext), `tools/parse_neon_lists.py` (listas de hechizos por clase), `tools/parse_neon_subclasses.py` (52 subclases: 26 del Handbook y 26 del Overdrive) y `tools/build_neon_pack.py` (escribe `SD_CARD/neon`).

Límites conocidos:
- Las tablas dentro de algunos hechizos (Confusion, Prismatic Spray...) quedan como texto corrido.
- Del material Neon solo se incluyen hechizos, listas por clase y de cada subclase: nombre, frase, resumen (máx. ~900 caracteres) y hechizos que otorga. **No** incluye las características de clase/subclase ni los stat blocks.
- El playtest tiene incoherencias entre sus listas y los encabezados de hechizo (p. ej. nivel de Life Cycle); `build_neon_pack.py` las resuelve usando el nivel del propio hechizo y las avisa al ejecutarse.

## Prueba en PC (opcional)

```
cd hosttest
g++ -std=c++17 -Wall -Wextra -Istubs -I../src test.cpp -o hosttest
./hosttest
```

Compila `main.cpp` contra versiones falsas de las librerías de Arduino y abre todas las entradas de las listas SRD y Neon, las subclases, y prueba el cambio de libro, el brillo, el sleep y la lógica del botón BOOT. Debe terminar con `RESULT: PASS`.

## Licencia y atribución

El contenido del SRD 5.2.1 está bajo CC BY 4.0. El aviso de atribución está en `SD_CARD/grimoire/ATTRIBUTION.txt` y se muestra en la pantalla About. Consérvalo si redistribuyes la tarjeta.

**`SD_CARD/neon` es de uso personal.** Neon Odyssey (Playtest 1) es (c) Avantris, todos los derechos reservados, y es material de patrocinadores: no subas esa carpeta, ni los JSON `data/neon_*.json`, a un repositorio público ni la compartas. El `.gitignore` ya la excluye.

## Documentación Adicional 📚

- **[CHANGELOG.md](CHANGELOG.md)** - Historial completo de versiones y cambios
- **[DEVELOPMENT.md](DEVELOPMENT.md)** - Guía de desarrollo, debugging y modificaciones
- **[GITHUB_SETUP.md](GITHUB_SETUP.md)** - Instrucciones para subir/clonar del repositorio de GitHub

## Siguiente

- Multiclase y edición manual de slots; notas por personaje.
- Fuente VLW con acentos y símbolos.
- Búsqueda por nombre, favoritos, espacios de conjuro, sleep y LED como en PlanarNexus.
- Mejorar las tablas dentro de los hechizos.
- Rotación vertical (90°/270°) - requiere rediseño completo de layouts

## Soft sleep (v0.8)

- **Options > Screen off**: un botón que cicla *Never / 1 / 2 / 5 / 10 min* (por defecto 5). Se guarda en NVS (`soff`).
- Tras el tiempo sin tocar la pantalla, la luz de fondo baja al mínimo; 10 s después se apaga. El micro sigue despierto, así que **un toque (o pulsar BOOT) la enciende al instante** y estás justo donde lo dejaste. El toque que despierta no se cuenta como pulsación de botón.
- Es independiente del **Deep sleep** (botón en Options o mantener BOOT 2 s), que sí reinicia el programa al despertar.
- Ahorro: la luz de fondo es lo que más consume; con la pantalla apagada el consumo baja mucho, pero no tanto como el deep sleep.
