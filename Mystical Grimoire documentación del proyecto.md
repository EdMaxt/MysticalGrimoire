# Mystical Grimoire: documentación del proyecto

Oct 30, 2026 · @EdMaxt

## Resumen

Mystical Grimoire es un compañero de hechizos de D&D 2024 para la CYD ESP32 (pantalla táctil de 320×240) que lee todo desde una microSD. La versión actual es la **v0.6**, y el usuario la ha probado en la placa: funciona, incluidos Spellbooks, rotación e iconos.

- **Dos libros:** D&D 2024 (SRD 5.2.1, con licencia abierta) y Neon Odyssey (playtest de Avantris, solo uso personal). Se cambia en Options y se recuerda al reiniciar.
- **Listas de hechizos por clase**, pestañas por nivel, lectura con scroll por arrastre, subclases y hechizos extra del PHB (solo nombre y datos, sin texto).
- **Spellbooks:** un personaje por libro, con espacios de conjuro, Cast con upcast, preparados y siempre-preparados.
- **Options:** brillo (5 niveles), sleep, BOOT 2 s para dormir y despertar, rotación 0/180.
- **Iconos de clase** en los tiles del Home y arte tenue de fondo en la pantalla de cada clase.
- **Todo es datos en la SD:** el firmware no tiene hechizos dentro. Los scripts en PC generan la tarjeta.

El proyecto vive en `C:\Users\edmax\Projects` y se entrega como un zip único (`Mystical_Grimoire.zip`). Lo que no se ha probado en la placa: el script de importación del PHB en español (se probó en PC con el PDF del usuario).

## Hardware

La placa es una CYD (Cheap Yellow Display) con ESP32-D0WDQ6 y pantalla ILI9341 de 2,8" (320×240). Con PlatformIO, el entorno `cyd_touchtest` sirve para calibrar el touch.

| Elemento | Detalle |
| --- | --- |
| Pantalla | ILI9341, `TFT_eSPI` (fork ggaljoen), rotación 1 (horizontal) o 3 (girada 180°) |
| Touch | XPT2046 leído por software (bit-bang): CLK 25, MOSI 32, MISO 39, CS 33, `TOUCH_Z_MIN` 400 |
| microSD | Bus VSPI: CS 5, SCK 18, MISO 19, MOSI 23; FAT32, tarjeta de 32 GB; se prueba a 20 MHz y luego a 4 MHz |
| Luz de fondo | GPIO 21, PWM con `ledc` (5 kHz, 8 bits); 5 niveles de duty 51/102/153/204/255 |
| LED RGB | GPIO 22 (R), 16 (G), 17 (B), activo en bajo |
| Botón BOOT | GPIO 0; mantener 2 s entra en sleep, un toque despierta |

**Por qué el touch va por software:** el bus VSPI lo usa la microSD, así que el XPT2046 se lee con pines propios. Los valores de calibración están en `config.h` (`TOUCH_RAW_*`, `TOUCH_SWAP_XY`, `TOUCH_INVERT_*`).

**Sleep:** deep sleep con despertar `ext0` por GPIO 0. Se espera a soltar BOOT antes de dormir, y el hold solo se arma tras haberlo soltado una vez desde el arranque.

**Rotación:** Options alterna entre rotación 1 y 3; el touch se invierte en X e Y por software. La rotación vertical (90°/270°) no existe: todos los layouts están hechos para 320×240.

**Preferencias (NVS, espacio `grimoire`):** `pack` (libro activo), `bri` (brillo 1–5) y `rot` (rotación).

## Arquitectura del firmware

El firmware es `src/main.cpp` (unas 1.300 líneas) más `src/spellbook.h`, que se incluye una sola vez y comparte su estado. `config.h` tiene pines y rutas, y `touch_soft.h` el driver del touch.

**Pantallas:** HOME, CLASS, READER, ERROR, OPTIONS, SUBS, SHEET, EDIT, PICK, KEYBOARD, CAST, WZBOOK. Cada una tiene su función de dibujo y su zona de toques en `handleTap`.

**Estructuras clave**

- `Entry`: id, nivel, escuela, flags (concentración, ritual, PHB-only), origen (SRD o Neon), nombre y reskin base.
- `ClassInfo`: nombre, cantidad de hechizos, nivel máximo, subclases, color y clase base.
- `SubEntry`: id, nombre, si tiene hechizos y origen (Handbook, Overdrive o SRD).
- `Character` (spellbook): nombre, especie, libro, clase, subclase, opción, nivel, mod, DC, Atk, slots usados y preparados.

**Límites en memoria:** 16 clases, 300 entradas por lista, 8 subclases por clase, 260 líneas de texto, buffer de archivo de 6.144 bytes, 12 personajes, 48 preparados y 24 siempre-preparados. El fondo de la pantalla de clase usa un buffer de 51 KB (`malloc`) que se libera al salir.

**Texto:** el lector no copia nada; sus líneas apuntan al buffer del archivo y se parten por palabras (`wrapPara`). Con las fuentes de mapa de bits (`TFT_eSPI` fuente 2 para cuerpo y 4 para títulos) se reemplazan los caracteres especiales por ASCII (`foldAscii`: comillas, guiones, acentos, ñ, ¿, ¡). Hay soporte opcional para una fuente VLW en `/grimoire/fonts/body.vlw`.

**Loop:** el toque se confirma al soltar (tras 3 lecturas sin presión). Un arrastre de más de 10 px hace scroll por filas (en el lector, por líneas). BOOT mantenido 2 s duerme la placa.

**Temas:** SRD usa dorado y violeta; Neon usa cian y magenta (`applyTheme`). El color de cada clase sale de una tabla `CLASS_RGB` por posición en `classes.txt`; los scripts de iconos usan la misma tabla.

## Formato de datos en la SD

La tarjeta tiene dos libros (`/grimoire` y `/neon`, este último opcional) y una carpeta de personajes (`/characters`). Todo es texto plano en UTF-8, salvo los iconos.

| Ruta | Contenido |
| --- | --- |
| `<libro>/index/classes.txt` | `nombre\|hechizos\|nivelMax\|subclases\|basadaEn`. Con 0 hechizos es una clase marcial, que abre directo sus subclases |
| `<libro>/index/<clase>.idx` | `id\|nivel\|nombre\|escuela\|flags[\|base]`. Flags: C concentración, R ritual, P solo PHB (se muestra atenuado). El 6.º campo es el hechizo SRD del que es reskin |
| `<libro>/index/all.idx` | Todos los hechizos del libro |
| `<libro>/index/progress.txt` | `Clase\|tipo\|nivel\|trucos\|preparados\|slots`. Tipo `full`, `half` o `pact`; en pact los slots son `cantidad,nivelDelSlot` |
| `<libro>/index/always.txt` | `Clase\|nivel\|idHechizo`: hechizos siempre preparados que da la clase |
| `<libro>/index/species.txt` | Solo Neon: lista de especies |
| `<libro>/spells/NNN.txt` | Cabecera `clave=valor` (name, level, school, classes, time, range, comp, dur, flags), una línea `---` y el texto |
| `<libro>/sub/<clase>.idx` | `sid\|Nombre\|Fuente\|S?` (S = tiene lista de hechizos) |
| `<libro>/sub/NNN.txt` | Cabecera name/class/source/tagline, `---`, introducción y bloques opcionales `@SPELLS` y `@GRANT` |
| `<libro>/icons/<clase>_{t,w,b}.bin` | RGB565 crudo: t 36×36, w 34×34, b 160×160 |
| `/characters/cNN.txt` | Un personaje (00 a 11), texto `clave=valor` |

**Ids de hechizo.** En `/grimoire` van del 001 al 391: 001–339 son del SRD y 340–391 son los 52 hechizos solo del PHB (solo metadatos). En `/neon` un id `gNNN` es un hechizo del SRD (se lee de `/grimoire`) y `nNNN` es un hechizo propio de Neon.

**`@GRANT`** (dentro de una subclase): `nivel|grupo|ids`. Un grupo `*X` o vacío se concede siempre; cualquier otro grupo es de "elige uno" (por ejemplo Arid, Polar, Temperate o Tropical en Circle of the Land) y solo cuenta el elegido. Solo se aplican las líneas con nivel ≤ el del personaje.

**Personaje (`cNN.txt`):** `name`, `species`, `book` (0 D&D, 1 Neon), `class`, `subid`, `sub`, `choice`, `level`, `mod`, `auto` (DC/Atk automáticos), `dc`, `atk`, `used` (slots gastados por nivel 1–9) y `prep` (referencias como `g012` o `n003`). Se reescribe entero en cada cambio (se borra y se vuelve a crear, porque `FILE_WRITE` trunca en el core del ESP32).

## Fuentes de datos y licencias

El proyecto puede publicarse en git sin infringir derechos de autor porque cada fuente se trata según su licencia.

| Fuente | Qué se usó | Licencia y regla |
| --- | --- | --- |
| SRD 5.2.1 (D&D 2024) | 339 hechizos, 8 clases, subclases y tablas | CC BY 4.0. El aviso está en `SD_CARD/grimoire/ATTRIBUTION.txt` y en la pantalla About. Se puede redistribuir |
| Neon Odyssey, Playtest 1 (Outrunner's Handbook y Overdrive Expansion) | 13 clases, subclases y 95 hechizos nuevos | © Avantris, uso personal. `SD_CARD/neon/` y `data/neon_*.json` están en `.gitignore`: no se suben ni se comparten |
| Manual del Jugador 2024 en español | Solo se validó qué hechizos faltan en el SRD (52) | © Wizards of the Coast. No se incluye ni se traduce su texto: solo nombre, nivel, escuela, clases y flags |
| Arte de iconos (imágenes del usuario) | Iconos y fondos de las 12 clases de D&D y las 13 de Neon | Material del usuario, en `assets/src/` |

**Política del texto del PHB.** El script `tools/import_phb_es.py` permite al usuario cargar ese texto en español desde su propio PDF, en una copia privada de la tarjeta (`SD_PERSONAL/`, ignorada por git). Nada de ese texto viaja en el repositorio ni en el zip.

**Validación del SRD contra el PHB.** Se parsearon 391 encabezados del PHB en español (páginas 241–345). 339 coincidieron con el SRD, sin discrepancias de clase ni de tiempo de lanzamiento, y 52 no estaban. Los nombres en inglés de esos 52 los puse yo, porque el libro está en español; la tabla de equivalencias está en `tools/phb_crosscheck.csv`.

## Herramientas y pipeline de datos

Todos los scripts son de Python 3 y se corren desde la raíz del proyecto. Con `-I` Python ignora código cercano a los datos, una buena práctica con archivos descargados.

| Paso | Script y comando | Resultado |
| --- | --- | --- |
| SRD a datos | `1_extract_pages.py` y `2_parse_spells.py` sobre `SRD_CC_v5_2_1.pdf` | `data/spells.json` |
| Pack SRD | `python3 -I tools/3_build_sd.py data/spells.json SD_CARD` | `SD_CARD/grimoire` (hechizos, índices, subclases, progresión) |
| Neon a datos | `parse_neon_lists.py`, `parse_neon_spells.py`, `parse_neon_subclasses.py` | `data/neon_*.json` |
| Pack Neon | `python3 -I tools/build_neon_pack.py SD_CARD` | `SD_CARD/neon` |
| Tablas de slots | `parse_progress.py` | `data/progress_srd.txt` y `progress_neon.txt` |
| Validar contra el PHB | `phb_1_parse_es.py`, `phb_2_match.py`, `phb_3_make_only.py` | `data/phb_only.json` y `tools/phb_crosscheck.csv` |
| Iconos | `python3 -I tools/build_icons.py SD_CARD` (pillow, numpy, scipy) | `SD_CARD/<libro>/icons/*.bin` |
| Texto del PHB (opcional) | `python tools/import_phb_es.py "Manual.pdf" SD_PERSONAL` | Texto en español en la copia privada |

**Orden recomendado:** primero `3_build_sd.py`, luego `build_neon_pack.py` (reutiliza los datos del SRD) y por último `build_icons.py`. Los iconos se regeneran siempre que cambie una imagen o un color de clase.

**`import_phb_es.py`** extrae el texto con `pdftotext` (o `pdfplumber`), lo asocia a los 52 hechizos por su nombre en español y reescribe `spells/340–391.txt` y los flags de los índices. `--restore` devuelve los avisos originales. En la prueba cargaron 49 de 52; Staggering Smite, Swift Quiver y Thunderous Smite quedan con aviso porque su página a dos columnas sale mezclada y el script prefiere no escribir texto corrupto.

**Prueba en PC:** `hosttest/` compila el firmware contra stubs de Arduino, TFT, SD y Preferences:

```bash
cd hosttest
cp -r ../SD_CARD /tmp/sd_host   # el test escribe personajes aquí
g++ -std=c++17 -Wall -Wextra -Istubs -I../src test.cpp -o hosttest && ./hosttest
```

## Historial de versiones

| Versión | Qué trajo |
| --- | --- |
| v0.6 | Iconos de clase: icono en los tiles del Home y arte tenue de fondo en la pantalla de clase. Los iconos de D&D se recolorearon al color del marco de su tile |
| v0.5 | Options > Rotate (0/180), la pestaña Prepared incluye los siempre-preparados, y en Neon las clases sin hechizos pasan a una franja aparte |
| v0.4 | Spellbooks: pestaña en el Home, asistente de creación, teclado en pantalla, hoja con Slots/Prepared/All/Always, Cast con upcast, edición y borrado. Importador del texto del PHB en español y soporte de acentos en la fuente |
| v0.3 | 52 hechizos del PHB que faltan en el SRD (solo metadatos), brillo, sleep y BOOT de 2 s (como PlanarNexus) |
| v0.2 | Libro Neon Odyssey con 13 clases y 52 subclases, pantalla Options para cambiar de libro |
| v0.1 | SRD 5.2.1: 8 clases, 339 hechizos, pestañas por nivel, lector. Probada en la placa |

El proyecto hermano **PlanarNexus** (contador de MTG Commander para la misma placa) fue la referencia para brillo, sleep y BOOT.

## Spellbooks

Cada spellbook es un personaje y pertenece a un libro (D&D o Neon), lo que decide qué clases y hechizos se cargan.

**Crear:** Home > pestaña Spellbooks > `+ New spellbook` > libro > nombre (teclado en pantalla) > especie (lista del libro u "Other") > clase (solo lanzadores) > subclase (o None) > opción de la subclase si tiene grupos > nivel y modificador de lanzamiento.

**Reglas de cálculo**

- Bono de competencia = 2 + (nivel − 1) / 4, con división entera.
- Spell save DC = 8 + bono + modificador.
- Spell attack = bono + modificador.
- DC y Atk se pueden ajustar a mano; `Auto DC/Atk` vuelve al cálculo.
- Trucos y preparados salen de `progress.txt` para la clase y el nivel. Superarlos solo pinta el contador en rojo: es un aviso, no un bloqueo.
- Los máximos de slots salen de la misma tabla. Warlock y Cryptist usan slots de pacto (cantidad y nivel únicos) y tienen `Short rest`.

**Hoja del personaje**

| Pestaña | Qué muestra |
| --- | --- |
| Slots | Casillas por nivel. Tocar una marca como usadas hasta esa; tocar la última usada la libera. `Long rest` restaura todo |
| Prepared | Los elegidos más los siempre-preparados (en dorado) |
| All | La lista de la clase por nivel, solo hasta el mayor slot que tiene. Casilla = preparado; el nombre abre el hechizo |
| Always | Lo que dan la clase (`always.txt`) y la subclase (`@GRANT`) |

La cabecera muestra DC, Atk, `Cantrips x/y` y `Prepared x/y`.

**Siempre-preparados.** No gastan preparados. Vienen de `always.txt` (por clase y nivel) y de `@GRANT` de la subclase (por nivel y opción elegida, por ejemplo Circle of the Land).

**Cast.** Al abrir un hechizo de nivel 1 o más desde la hoja aparece el botón `Cast`. El diálogo lista los slots desde el nivel base hacia arriba, con los que quedan (upcast), y `No slot (free/ritual)`. Con pacto solo ofrece el nivel del pacto. No deja gastar un slot que no queda.

**Editar y borrar:** nombre, nivel, modificador, DC, Atk y subclase. Borrar pide tocar dos veces. Un personaje ocupa un archivo en `/characters` (máximo 12).

## Iconos y arte de fondo

Los iconos salen de las imágenes del usuario en `assets/src/` y se convierten con `tools/build_icons.py` a RGB565 crudo (sin cabecera).

| Archivo | Tamaño | Uso |
| --- | --- | --- |
| `<clase>_t.bin` | 36×36 | Icono del tile grande del Home (D&D) |
| `<clase>_w.bin` | 34×34 | Marca de agua tenue (30%) en tiles compactos y franja de Neon |
| `<clase>_b.bin` | 160×160 | Fondo atenuado (30%) y desaturado de la pantalla de clase |

**Fuentes de arte.** D&D: una imagen plana para tiles y una detallada para fondos (cuadrícula de 4×3). Neon: siluetas blancas (4×4, orden alfabético de las 13 clases) para tiles, un arte suave (3×4, sin Oracle) para fondos y el ojo de otra imagen para el fondo de Oracle.

**Color.** El tile D&D toma el color de su marco (`CLASS_RGB`), igual que las siluetas de Neon; el fondo mezcla 50% del color propio y 50% del color de la clase. El negro del arte pasa a transparente al mezclarlo con el color de fondo de la app.

**Dibujo en el firmware**

- Tile grande: icono arriba y nombre abajo (sin el conteo de hechizos).
- Tile compacto: marca de agua detrás del nombre, con texto transparente.
- Franja de clases sin hechizos: filas centrales de la marca de agua, solo el nombre.
- Pantalla de clase: el fondo se carga a RAM al entrar (51 KB), se repinta por franjas en cada scroll y se libera al volver al Home. Las filas pierden el panel y quedan con una línea fina para que se lea el texto.
- Si falta un archivo, el tile se dibuja como antes. Con fuente VLW el fondo se desactiva, porque el texto transparente necesita fuentes de mapa de bits.

**Detalles técnicos:** `pushImage` necesita `setSwapBytes(true)` con valores RGB565 nativos. El negro de los JPEG no es negro puro; el script lo sube a negro para que no se vea un cuadro, y recorta cada icono exactamente para que no entre arte vecino.

## Pruebas

**En PC (`hosttest/test.cpp`).** Compila el firmware real con stubs y recorre los datos y las pantallas. Termina en `RESULT: PASS` si todo cumple. Cubre:

- Los 987 hechizos del SRD (con los 52 PHB-only) y los 1.175 de Neon: todos abren, caben en el ancho de pantalla y no dejan caracteres fuera de ASCII.
- Los 95 hechizos nuevos de Neon y las 52 subclases.
- Options: cambio de libro, brillo, sleep, rotación y la lógica de BOOT de 2 s, con persistencia tras reiniciar.
- Home de Neon con la franja de clases sin hechizos y que todos los tiles quepan.
- Iconos: que se dibujen sin salirse de pantalla y que el fondo se cargue y se libere.
- Spellbooks completo: crear en ambos libros, DC/Atk automáticos y manuales, slots, long y short rest, Cast con upcast y pacto, preparar, siempre-preparados (Druid Circle of the Land, Cryptist), guardar y reabrir, editar y borrar.

**En la placa.** El usuario probó hasta la v0.6 y reporta que funciona. Lista de verificación que sigue en el README:

- [ ] Compila con PlatformIO y la pantalla se ve bien.
- [ ] La SD se monta y el touch coincide con lo que se dibuja.
- [ ] Home > clase > nivel > hechizo > volver, con scroll por arrastre y flechas.
- [ ] Options: libro Neon, brillo, Sleep now, BOOT 2 s y rotación; todo se recuerda al reiniciar.
- [ ] Spellbooks: crear, reiniciar, slots, Cast, preparados, siempre-preparados.
- [ ] Iconos en tiles, franja de clases sin hechizos y fondo atenuado con scroll fluido.
- [ ] Importador del PHB en español sobre una copia `SD_PERSONAL`.

## Decisiones, supuestos y limitaciones

**Decisiones del usuario:** la app está en inglés aunque él escribe en español; se usa el fork ggaljoen de `TFT_eSPI`; microSD de 32 GB; el texto del PHB nunca se incluye, solo se puede importar de su propio PDF.

**Supuestos míos que conviene revisar**

- En las subclases Neon con tablas de varias columnas (Cacophonous, Stargazer, Arbiter, Hivemind), la primera columna se trata como siempre concedida y las demás como "elige una".
- Los nombres en inglés de los 52 hechizos solo del PHB los puse yo a partir del libro en español.
- Los máximos de slots, trucos y preparados salen de las tablas de clase tal cual; no hay edición manual.
- El orden de los iconos de Neon sigue el alfabético de las clases; Icon, Scoundrel, Machinist y Disciple se confirmaron con el usuario.

**Limitaciones conocidas**

- Sin multiclase.
- Rotación solo 0° y 180°; la vertical exige rediseñar todos los layouts.
- Los hechizos solo del PHB no tienen texto en la tarjeta del repositorio.
- El importador no logra 3 de los 52 hechizos (páginas a dos columnas mezcladas): se rellenan a mano.
- El fondo atenuado solo existe en la pantalla de clase, no en las de Spellbooks.
- Con fuente VLW desaparecen los fondos de clase.
- Hasta 12 personajes, 48 preparados y 300 entradas por lista.

## Lecciones aprendidas

**Firmware y placa**

- `FILE_WRITE` en el core del ESP32 trunca el archivo: para reescribir un personaje se borra y se crea de nuevo.
- `LOW` es una macro de Arduino: no se puede usar como nombre de variable.
- `pushImage` con valores RGB565 nativos necesita `setSwapBytes(true)`.
- El texto transparente (`setTextColor(fg)`) solo sirve con fuentes de mapa de bits; con VLW se usa el color de fondo.
- El bus VSPI es de la microSD, por eso el touch va por software.
- Los límites de memoria (buffers fijos) se prueban en PC contra los datos reales antes de pasar a la placa.

**Datos y PDFs**

- Una línea de índice con campos vacíos al final pierde las barras `|` si se recorta: siempre se escriben los 6 campos y el firmware acepta desde 4.
- Para saber si una subclase es de Overdrive o del Handbook hay que comparar con `"Over"`, no con la inicial (las dos empiezan con O).
- El OCR del PHB en español rompe encabezados y nombres ("Tiempo de lanzamiento", nombres pegados): se ancló en las líneas de escuela y nivel, y el emparejamiento ES↔EN usa nivel, escuela, clases, alcance, duración, números y dados.
- Las páginas a dos columnas entremezclan hechizos con `pdftotext -raw`: el importador detecta texto mezclado y no lo escribe.

**Imágenes**

- El negro de un JPEG nunca es negro puro: hay que subirlo a negro al convertir.
- Cada icono se recorta por su caja exacta, o se cuela arte del vecino.
- Un icono tintado por luminosidad conserva el detalle y coincide con su color de clase.

**Trabajo con scripts**

- Los parches con `str.replace` deben anclar en la definición de la función, no en su prototipo (se borró código una vez por anclar mal).
- En pruebas de PC, el sleep y los toques simulados necesitan que BOOT se "suelte" solo, o el test se cuelga.
- No usar `pkill` con un texto que aparece en el propio comando: mata la shell.

## Pendientes e ideas futuras

- [ ] Icono pequeño junto al nombre del personaje en la lista y la hoja de Spellbooks.
- [ ] Fondo atenuado también en las pantallas de Spellbooks.
- [ ] Rotación vertical (90°/270°): rediseñar los layouts para 240×320.
- [ ] Multiclase y edición manual de slots; notas por personaje.
- [ ] Búsqueda por nombre y favoritos.
- [ ] Fuente VLW con acentos y símbolos.
- [ ] Texto de clase y subclase (features) en la app.
- [ ] Mejorar las tablas dentro de los hechizos.
- [ ] Rellenar a mano los 3 hechizos que el importador no pudo leer (Staggering Smite, Swift Quiver, Thunderous Smite).
- [ ] Confirmar la lectura de las subclases Neon de varias columnas contra las reglas.
