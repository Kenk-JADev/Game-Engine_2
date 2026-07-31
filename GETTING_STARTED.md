# 🎮 AetherRPG Maker – Sofort loslegen (ohne Terminal!)

## Die einfachste Methode (empfohlen für alle)

### 1. Lade die fertige Version herunter

Gehe zu → **[Releases](https://github.com/Kenk-JADev/Game-Engine_2/releases/latest)**

### 2. Lade das passende Paket herunter

| Dein Betriebssystem | Datei herunterladen                  | Danach machen |
|---------------------|--------------------------------------|---------------|
| **Windows**         | `AetherRPG-Maker-Windows-x64.zip`    | Entpacken → `start_editor.bat` **doppelklicken** |
| **Linux**           | `AetherRPG-Maker-Linux-x64.tar.gz`   | Entpacken → `./start_editor.sh` starten |

### 3. Fertig!

Der Editor startet **ohne** dass du irgendetwas kompilieren oder in die Kommandozeile tippen musst.

---

## Was passiert dann?

1. Editor öffnet sich
2. **Projekt** → Neues Projekt anlegen
3. **Karte** Tab → einfach Objekte anklicken und in die Welt klicken
4. **Events** Tab → Dialoge, Teleporter, Kämpfe per Knopfdruck hinzufügen
5. **Export** → Dein eigenes Spiel als `Game.exe` erstellen

**Kein Programmieren nötig!**

---

## Wenn du von GitHub den Quellcode klonst (nur für Entwickler)

Nur dann brauchst du:

```bash
git clone https://github.com/Kenk-JADev/Game-Engine_2.git
cd Game-Engine_2
make package
```

Danach kannst du `start_editor.bat` oder `start_editor.sh` im Root benutzen.

---

**Du brauchst normalerweise nur die Releases herunterzuladen.**
