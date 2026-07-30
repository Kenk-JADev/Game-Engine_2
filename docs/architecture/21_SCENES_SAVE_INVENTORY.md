# Szenen, Speichern, Inventar, Shop

## Szenen-Stack (`GameSceneStack`)

| Szene | ID | Zweck |
|-------|-----|--------|
| TitleScene | Title | Neues Spiel / Fortsetzen / Beenden |
| MapScene | Map | Spielwelt (Logik in Runtime) |
| MenuScene | Menu | Pause-Menü |
| DialogScene | Dialog | Nachrichten (Confirm weiter) |
| SaveLoadScene | SaveLoad | Slot wählen |

Navigation: `push` / `pop` / `replace`.

## Steuerung im Spiel

| Taste | Map | Menü/Titel |
|-------|-----|------------|
| WASD / Pfeile | Laufen | Auswahl |
| Z / Space / Enter | Event / Bestätigen | Bestätigen |
| Shift / V | Menü öffnen | – |
| X / Esc | – | Zurück / Abbrechen |

## Inventar & Party (`PartyInventory`)

- Gold, Items (ID + Anzahl)
- Party-Mitglieder (HP/MP/Level/EXP)
- `use_item` heilt aus Datenbank-Werten
- `gain_exp` mit einfacher Levelkurve

## Save/Load (`SaveSystem`)

Dateien: `<projekt>/saves/save01.json` …

Gespeichert: Map-ID, Position, Facing, Inventar/Party, Switches/Variables, Wetter, Spielzeit.

## Shop (`Shop`)

Angebote mit Item-ID und optionalem Preis. Kaufen/Verkaufen gegen Gold.

## Runtime-Flow

```
Title → (Neues Spiel) → Map
Map + Menu → Items/Speichern/Laden/Titel
Map + Confirm → Event → Dialog
Headless + max-frames → auto New Game (CI)
```
