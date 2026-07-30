# GitHub Actions Workflows (manuell aktivieren)

Die Arena-/GitHub-App darf **keine** Dateien unter `.github/workflows/` pushen
(`workflows`-Permission fehlt). Deshalb liegen die fertigen YAMLs hier.

## Aktivierung (einmalig)

### Option A – GitHub Web UI
1. Repo → **Actions** → „set up a workflow yourself“
2. Inhalt von `ci.yml` einfügen, speichern als `.github/workflows/ci.yml`
3. Ebenso `release.yml` als `.github/workflows/release.yml`

### Option B – Lokal mit deinem Token
```bash
mkdir -p .github/workflows
cp docs/dev/github-workflows/ci.yml .github/workflows/
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
  "/repos/Kenk-JADev/Game-Engine_2/contents/.github/workflows/ci.yml" \
  -f message='ci: add CI workflow' \
  -f content="$(base64 -w0 docs/dev/github-workflows/ci.yml)" \
  -f branch=arena/019fb2b9-game-engine-2
```

Nach dem Anlegen startet der Workflow bei Push/PR automatisch.

## Achtung beim Einfügen

Manche Editoren wandeln ``${{ matrix.build_type }}`` fälschlich in einen Markdown-Link um
(``[matrix.build](http://matrix.build)_type``). Das **bricht die CI**.

Immer **Raw-Datei** aus diesem Ordner kopieren oder per:

```bash
mkdir -p .github/workflows
cp docs/dev/github-workflows/ci.yml .github/workflows/
cp docs/dev/github-workflows/release.yml .github/workflows/
```

Bei Build-Fehlern:
- Job ist rot (exit code ≠ 0)
- Step **Show build errors on failure** zeigt `error:` / Linker-Fehler
- Artifact `*-failure-logs` enthält `build.log` und `ctest.log`
