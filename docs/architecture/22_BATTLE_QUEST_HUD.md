# Kampf, Quests, HUD, erweiterte Events

## Kampf (`Battle`)

Turn-basiert: Angriff · Fertigkeit · Verteidigen · Flucht.

- Start über Event `battle` oder Taste **Bild-Ab** (Demo)
- EXP/Gold bei Sieg, Quest `hunt_001` fortschreiben
- HUD zeigt HP beider Seiten + Log

## Quests (`QuestLog`)

Defaults:

| ID | Titel |
|----|--------|
| `main_001` | Der Älteste |
| `hunt_001` | Schleim-Jagd |

Event-Befehle: `quest`, `quest_complete`.

## HUD (`HudBuilder`)

Strukturierte Panels: Status, Dialog, Kampf, Quest, Menü.  
`draw_imgui()` wenn ImGui gelinkt (Editor); Runtime loggt Status.

## Events neu

| Typ | Wirkung |
|-----|---------|
| `choice` | UI-Auswahl, `branches` pro Option |
| `battle` | Kampf starten |
| `shop` | Händler (kauft erstes Angebot + Liste) |
| `weather` | Wetter setzen |
| `quest` / `quest_complete` | Quest-Log |
| `camera` | Follow-Kamera Offsets |

## Demo-Steuerung

| Taste | Map |
|-------|-----|
| WASD | Laufen |
| Z/Enter | Sprechen / Bestätigen |
| Shift | Menü |
| Bild-Ab | Testkampf |
| Esc | Zurück / Titel-Beenden |
