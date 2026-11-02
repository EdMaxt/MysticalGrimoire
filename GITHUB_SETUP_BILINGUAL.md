# GitHub Setup Guide / Guía de Configuración de GitHub

[**English**](#english) | [**Español**](#español)

---

## English

### Initial Setup

If you haven't initialized git in the project:

```bash
cd C:\Users\edmax\Projects\Mystical_Grimoire\Mystical_Grimoire
git init
git add .
git commit -m "Initial commit: Mystical Grimoire ESP32 D&D companion v1.0.0"
```

### Connect to Remote Repository

```bash
git remote add origin https://github.com/EdMaxt/MysticalGrimoire.git
git branch -M main
git push -u origin main
```

### Verify Status

```bash
# View configured remote
git remote -v

# View current status
git status

# View pending commits
git log --oneline -5
```

### Repository Structure

```
MysticalGrimoire/
├── .github/
│   └── ISSUE_TEMPLATE/
│       ├── bug_report.md
│       └── feature_request.md
├── .gitignore
├── LICENSE
├── README.md
├── README_BILINGUAL.md
├── CHANGELOG.md
├── CHANGELOG_BILINGUAL.md
├── DEVELOPMENT.md
├── DEVELOPMENT_BILINGUAL.md
├── GITHUB_SETUP.md
├── GITHUB_SETUP_BILINGUAL.md
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
│   └── srd_subclasses.json
├── SD_CARD/
│   ├── grimoire/
│   │   ├── fonts/
│   │   ├── icons/
│   │   ├── index/
│   │   ├── spells/
│   │   └── sub/
│   └── neon/
│       ├── icons/
│       ├── index/
│       └── spells/
└── tools/
    └── (generator scripts)
```

### Recommended Initial Commits

```bash
# 1. Base structure
git add platformio.ini src/ data/
git commit -m "✨ feat: Add ESP32 firmware and spell database"

# 2. Documentation
git add README*.md CHANGELOG*.md DEVELOPMENT*.md GITHUB_SETUP*.md LICENSE
git commit -m "📚 docs: Add comprehensive bilingual documentation"

# 3. SD data
git add SD_CARD/
git commit -m "📦 data: Add microSD card structure and assets"

# Push all
git push origin main
```

### GitHub Repository Configuration

#### 1. Repository Description
Go to Settings → General and update:
- **Description:** "D&D 5e companion app for ESP32 with touchscreen and dual rotation"
- **Topics:** `esp32`, `d&d`, `embedded-systems`, `c++`, `arduino`, `touchscreen`

#### 2. Create Issue Templates

Create `.github/ISSUE_TEMPLATE/bug_report.md`:
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

## Environment
- Device: CYD ESP32
- Firmware Version: 1.0.0
- OS: Windows / Linux / Mac
```

Create `.github/ISSUE_TEMPLATE/feature_request.md`:
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

#### 3. Recommended Labels
In Settings → Labels, create:
- `bug` - Red
- `enhancement` - Blue
- `documentation` - Green
- `question` - Yellow
- `good first issue` - Light green
- `help wanted` - Orange
- `wontfix` - Gray

#### 4. Branch Protection
Settings → Branches → Add rule:
- Branch name: `main`
- ✅ Require pull request reviews before merging
- ✅ Require status checks to pass before merging
- ✅ Require branches to be up to date before merging

### Useful Git Commands

```bash
# View uncommitted changes
git status
git diff

# Revert changes
git restore <file>

# Undo last commit (local)
git reset --soft HEAD~1

# Create feature branch
git checkout -b feature/new-feature

# Switch branches
git checkout main
git checkout feature/other-branch

# Merge branch (from main)
git merge feature/new-feature

# Push branch
git push origin feature/new-feature
```

### Recommended Git Workflow

```bash
# 1. Create feature branch
git checkout -b feature/improved-rotation

# 2. Make changes
# ... edit files ...

# 3. Commit
git add .
git commit -m "✨ feat: Improve rotation handling"

# 4. Push
git push origin feature/improved-rotation

# 5. Create Pull Request on GitHub
# (use web interface)

# 6. Merge to main (after review)
git checkout main
git pull origin main
git merge feature/improved-rotation
git push origin main

# 7. Delete branch (after merge)
git branch -d feature/improved-rotation
git push origin --delete feature/improved-rotation
```

---

## Español

### Configuración Inicial

Si aún no has inicializado git en el proyecto:

```bash
cd C:\Users\edmax\Projects\Mystical_Grimoire\Mystical_Grimoire
git init
git add .
git commit -m "Initial commit: Mystical Grimoire ESP32 D&D companion v1.0.0"
```

### Conectar con Repositorio Remoto

```bash
git remote add origin https://github.com/EdMaxt/MysticalGrimoire.git
git branch -M main
git push -u origin main
```

### Verificar Estado

```bash
# Ver remoto configurado
git remote -v

# Ver estado actual
git status

# Ver commits pendientes
git log --oneline -5
```

### Estructura del Repositorio

```
MysticalGrimoire/
├── .github/
│   └── ISSUE_TEMPLATE/
│       ├── bug_report.md
│       └── feature_request.md
├── .gitignore
├── LICENSE
├── README.md
├── README_BILINGUAL.md
├── CHANGELOG.md
├── CHANGELOG_BILINGUAL.md
├── DEVELOPMENT.md
├── DEVELOPMENT_BILINGUAL.md
├── GITHUB_SETUP.md
├── GITHUB_SETUP_BILINGUAL.md
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
│   └── srd_subclasses.json
├── SD_CARD/
│   ├── grimoire/
│   │   ├── fonts/
│   │   ├── icons/
│   │   ├── index/
│   │   ├── spells/
│   │   └── sub/
│   └── neon/
│       ├── icons/
│       ├── index/
│       └── spells/
└── tools/
    └── (scripts generadores)
```

### Commits Iniciales Recomendados

```bash
# 1. Estructura base
git add platformio.ini src/ data/
git commit -m "✨ feat: Add ESP32 firmware and spell database"

# 2. Documentación
git add README*.md CHANGELOG*.md DEVELOPMENT*.md GITHUB_SETUP*.md LICENSE
git commit -m "📚 docs: Add comprehensive bilingual documentation"

# 3. Datos SD
git add SD_CARD/
git commit -m "📦 data: Add microSD card structure and assets"

# Push todo
git push origin main
```

### Configuración del Repositorio GitHub

#### 1. Descripción del Repositorio
Ve a Settings → General y actualiza:
- **Description:** "D&D 5e companion app for ESP32 with touchscreen and dual rotation"
- **Topics:** `esp32`, `d&d`, `embedded-systems`, `c++`, `arduino`, `touchscreen`

#### 2. Crear Templates de Issues

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

## Environment
- Device: CYD ESP32
- Firmware Version: 1.0.0
- OS: Windows / Linux / Mac
```

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

#### 3. Etiquetas Recomendadas
En Settings → Labels, crear:
- `bug` - Rojo
- `enhancement` - Azul
- `documentation` - Verde
- `question` - Amarillo
- `good first issue` - Verde claro
- `help wanted` - Naranja
- `wontfix` - Gris

#### 4. Protecciones de Rama
Settings → Branches → Add rule:
- Branch name: `main`
- ✅ Require pull request reviews before merging
- ✅ Require status checks to pass before merging
- ✅ Require branches to be up to date before merging

### Comandos Git Útiles

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

# Mergear rama (desde main)
git merge feature/nueva-caracteristica

# Push de rama
git push origin feature/nueva-caracteristica
```

### Workflow Git Recomendado

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

---

## Legal Notice / Aviso Legal

### English

This repository includes content licensed under:
- **Application Code:** MIT License
- **D&D 5.2.1 SRD Content:** CC BY 4.0 (© 2016 Wizards of the Coast LLC)
- **Neon Odyssey Content:** Personal use only (playtest materials)

For complete licensing information, see [LICENSE](LICENSE).

### Español

Este repositorio incluye contenido licenciado bajo:
- **Código de Aplicación:** Licencia MIT
- **Contenido D&D 5.2.1 SRD:** CC BY 4.0 (© 2016 Wizards of the Coast LLC)
- **Contenido de Neon Odyssey:** Solo uso personal (materiales de playtest)

Para información completa de licencias, ver [LICENSE](LICENSE).

---

*Last updated / Última actualización: 2026-11-02*
