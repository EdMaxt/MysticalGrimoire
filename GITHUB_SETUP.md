# Guía para Subir a GitHub

## Configuración Inicial

Si aún no has inicializado git en el proyecto:

```bash
cd C:\Users\edmax\Projects\Mystical_Grimoire\Mystical_Grimoire
git init
git add .
git commit -m "Initial commit: Mystical Grimoire ESP32 D&D companion"
```

## Conectar con Repositorio Remoto

Si el repositorio remoto ya existe:

```bash
git remote add origin https://github.com/EdMaxt/MysticalGrimoire.git
git branch -M main
git push -u origin main
```

Si el repositorio es nuevo:

```bash
git remote add origin https://github.com/EdMaxt/MysticalGrimoire.git
git push -u origin main
```

## Verificar Estado

```bash
# Ver remoto configurado
git remote -v

# Ver estado actual
git status

# Ver commits pendientes
git log --oneline -5
```

## Estructura de Archivos en GitHub

```
MysticalGrimoire/
├── .github/
│   └── ISSUE_TEMPLATE/
│       ├── bug_report.md
│       └── feature_request.md
├── .gitignore
├── LICENSE
├── README.md
├── CHANGELOG.md
├── DEVELOPMENT.md
├── GITHUB_SETUP.md
├── platformio.ini
├── src/
│   ├── main.cpp
│   ├── spellbook.h
│   ├── battery.h
│   ├── config.h
│   └── touch_soft.h
├── data/
│   ├── spells.json
│   ├── neon_spells.json
│   ├── neon_subclasses.json
│   └── phb_only.json
├── SD_CARD/
│   ├── grimoire/
│   │   ├── fonts/
│   │   ├── icons/
│   │   ├── index/
│   │   └── spells/
│   └── neon/
│       ├── icons/
│       ├── index/
│       └── spells/
├── tools/
│   └── (scripts generadores)
└── README.md (en raíz si es diferente)
```

## Commits Iniciales Recomendados

```bash
# Si no está todo en un commit

# 1. Estructura base
git add platformio.ini src/ data/
git commit -m "✨ feat: Add ESP32 firmware and spell database"

# 2. Documentación
git add README.md CHANGELOG.md DEVELOPMENT.md LICENSE
git commit -m "📚 docs: Add comprehensive documentation"

# 3. Datos SD
git add SD_CARD/
git commit -m "📦 data: Add microSD card structure and assets"

# Push todo
git push origin main
```

## Configuración de GitHub (Web)

### 1. Descripción del Repositorio
Ve a Settings → General y actualiza:
- **Description:** "D&D 5e companion app for ESP32 with touchscreen"
- **Website:** (Opcional)
- **Topics:** `esp32`, `d&d`, `embedded-systems`, `c++`, `arduino`

### 2. Crear Issues Template

Crear `.github/ISSUE_TEMPLATE/bug_report.md`:
```markdown
---
name: Bug Report
about: Report a bug in Mystical Grimoire
---

## Describe the bug
...

## Steps to reproduce
1. Go to...
2. Click...
3. See error...

## Expected behavior
...

## Screenshots
...

## Environment
- Device: CYD ESP32
- Firmware Version: 1.0.0
- OS: Windows / Linux / Mac
```

### 3. Crear Feature Request Template

Crear `.github/ISSUE_TEMPLATE/feature_request.md`:
```markdown
---
name: Feature Request
about: Suggest an idea for Mystical Grimoire
---

## Is your feature request related to a problem?
...

## Describe the solution you'd like
...

## Additional context
...
```

## Etiquetas Recomendadas (Labels)

En Settings → Labels, crear:
- `bug` - Rojo
- `enhancement` - Azul
- `documentation` - Verde
- `question` - Amarillo
- `good first issue` - Verde claro
- `help wanted` - Naranja
- `wontfix` - Gris

## Protecciones de Rama

Settings → Branches → Add rule:
- Branch name: `main`
- ✅ Require pull request reviews before merging
- ✅ Require status checks to pass before merging
- ✅ Require branches to be up to date before merging

## Comandos Útiles para Desarrollo

```bash
# Ver cambios sin commitear
git status
git diff

# Revertir cambios
git restore <archivo>

# Deshacer último commit (local)
git reset --soft HEAD~1

# Crear rama de feature
git checkout -b feature/nueva-caracteristica

# Cambiar entre ramas
git checkout main
git checkout feature/otra-rama

# Merger rama (desde main)
git merge feature/nueva-caracteristica

# Push de rama
git push origin feature/nueva-caracteristica
```

## Workflow Git Recomendado

```bash
# 1. Crear rama de feature
git checkout -b feature/rotacion-mejorada

# 2. Hacer cambios
# ... editar archivos ...

# 3. Commit
git add .
git commit -m "✨ feat: Improve rotation handling"

# 4. Push
git push origin feature/rotacion-mejorada

# 5. Crear Pull Request en GitHub
# (usar interfaz web)

# 6. Mergear en main (después de review)
git checkout main
git pull origin main
git merge feature/rotacion-mejorada
git push origin main

# 7. Deletear rama (después de merge)
git branch -d feature/rotacion-mejorada
git push origin --delete feature/rotacion-mejorada
```

## Estado Actual

- ✅ Archivos de código compilables
- ✅ Documentación completa (README, CHANGELOG, DEVELOPMENT)
- ✅ License (MIT + D&D SRD CC BY 4.0)
- ✅ .gitignore configurado
- ✅ Estructura apropiada para GitHub
- ✅ Firmware testeado y funcionando

## Próximos Pasos

1. ✅ Verificar que todo está committeado
2. ✅ Hacer push a GitHub
3. ⏳ Crear GitHub Pages (opcional)
4. ⏳ Configurar GitHub Actions (opcional)
5. ⏳ Configurar releases automáticos (opcional)

---

**¡Listo para GitHub!** 🚀

*Para problemas o preguntas, abrir un issue en GitHub.*
