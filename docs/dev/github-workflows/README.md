# GitHub Actions Workflows

Aktuell aktiv in `.github/workflows/`:
- `ci-build.yml` – CI (Linux Debug/Release, Linux Headless, Windows, macOS)

**Release-Pakete** (`release.yml`) sind vorbereitet, aber noch nicht aktiv –
die GitHub-App ohne `workflows`-Permission kann keine Workflow-Dateien
pushen. Siehe „Aktivierung“.

## Dateien in diesem Ordner

| Datei | Inhalt |
|-------|--------|
| `ci-build.yml` | **Referenz-Kopie des aktiven CI-Workflows** (Stand: letzter Update) – inkl. Diagnose: Configure-/Build-Fehler als GitHub-Annotationen (`::error::`), `configure.log` + `CMakeError.log`-Dump im Failure-Artifact, macOS in getrennten Schritten mit pip-Fallback |
| `ci.yml` | Identische Kopie (Legacy-Name, für manuelle Aktivierung) |
| `release.yml` | Release-Pakete bei Tags `v*` (Linux/Windows-Archive) – noch nicht aktiv |

## Aktivierung (einmalig)

> **Hinweis:** `.github/workflows/` erfordert die `workflows`-Permission.
> Die Arena-GitHub-App hat sie nicht – deshalb liegen die Dateien hier
> als Referenz. Aktiviere sie mit deinem eigenen Token (Option B/C).

### Option A – GitHub Web UI
1. Repo → **Actions** → „set up a workflow yourself“
2. Inhalt von `ci-build.yml` einfügen, speichern als `.github/workflows/ci-build.yml`
3. Ebenso `release.yml` als `.github/workflows/release.yml`

### Option B – Lokal mit deinem Token
```bash
mkdir -p .github/workflows
cp docs/dev/github-workflows/ci-build.yml .github/workflows/
cp docs/dev/github-workflows/release.yml .github/workflows/
git add .github/workflows
git commit -m "ci: enable GitHub Actions workflows"
git push
```
(Benötigt ein Personal Access Token / SSH-Key mit `workflow`-Scope.)

### Option C – gh CLI
```bash
gh api --method PUT \
  -H "Accept: application/vnd.github+json" \
  "/repos/Kenk-JADev/Game-Engine_2/contents/.github/workflows/ci-build.yml" \
  -f message='ci: add CI workflow' \
  -f content="$(base64 -w0 docs/dev/github-workflows/ci-build.yml)" \
  -f branch=arena/019fb75b-game-engine-2
```

Nach dem Anlegen startet der Workflow bei Push/PR automatisch.

## Achtung beim Einfügen

Manche Editoren wandeln ``${{ matrix.build_type }}`` fälschlich in einen Markdown-Link um
(``[matrix.build](http://matrix.build)_type``). Das **bricht die CI**.

Immer **Raw-Datei** aus diesem Ordner kopieren oder per:

```bash
mkdir -p .github/workflows
cp docs/dev/github-workflows/ci-build.yml .github/workflows/
cp docs/dev/github-workflows/release.yml .github/workflows/
```

## Fehlerdiagnose

Bei Fehlern zeigt die aktive CI: Job rot, Step „Show … errors on failure“,
Artifact `*-failure-logs` mit `build.log`/`ctest.log`/`configure.log`, und
die exakten Fehlerzeilen als GitHub-Annotation (im Checks-Tab sichtbar).
