# Beehive — Disziplin-Regeln

## Sprache & Identifier
- **Kommunikation mit User: Deutsch**
- **Code-Kommentare: Englisch**
- **App-Sprache: Englisch** (primär)
- **ALLE Identifier auf Deutsch** — Funktionen, Structs, Enums, Variablen, Konstanten
  - Umlaute ersetzen: ä→ae, ö→oe, ü→ue
  - Beispiele: `bienenstock_zeichnen()`, `Spielzustand`, `ZELLE_HONIG`, `WiesenBiene`

## ⚠️ INTUITIVITÄT — BEWUSSTE ENTSCHEIDUNG!
- Deutsche Identifier sind KEINE KI-Konvention, sondern eine persönliche Entscheidung des Entwicklers
- Der Code soll für den Entwickler selbst intuitiv lesbar sein — man liest `schwanzeltanz()` und versteht sofort was passiert
- KI-Vorschläge kommen oft auf Englisch (z.B. "Keeper", "Guard", "Forager") — diese IMMER ins Deutsche übersetzen, bevor sie in den Code kommen
- Ziel: Man soll dem Code ansehen, dass ein Mensch ihn geschrieben hat — nicht eine KI

## Performance (OBERSTE PRIORITÄT!)
- Ziel: Tausende Bienen ohne Lag
- **Flat Arrays** statt Pointer-Arrays (Cache-freundlich)
- NIEMALS einen Node/Struct per malloc pro Biene/Zelle
- Einfache Lösungen bevorzugen
- Spatial Partitioning wenn nötig (Grid-basiert, kein KD-Baum)

## Code-Struktur
- **Eine Datei = Eine Verantwortung** — niemals Systeme mischen
- **Header zuerst** — `.h` schreiben BEVOR `.c` implementiert wird
- `.h` = vollständige API-Dokumentation, öffentliche Typen und Funktionen
- `.c` = Implementierung + interne (static) Hilfsfunktionen
- Keine zirkulären Abhängigkeiten zwischen Modulen

## ⚠️ FAHRPLAN VOR CODE — OBERSTE PRÄMISSE!
- **NIEMALS direkt anfangen zu coden** — erst einen detaillierten Fahrplan in einer MD-Datei erstellen
- Fahrplan-Datei: `memory/fahrplan.md` — wird VOR dem ersten Tastendruck Code gepflegt
- Der Fahrplan enthält: Was wird gebaut, in welcher Reihenfolge, welche Dateien, welche Structs/Funktionen
- **Warum:** Wenn Tokens ausgehen oder die Session unterbrochen wird, kann jede neue Session den Fahrplan lesen und exakt weitermachen
- Der Fahrplan wird nach jedem abgeschlossenen Schritt aktualisiert (Haken setzen, nächster Schritt markieren)
- Gilt für JEDE Feature-Implementierung, nicht nur für große Milestones

## Session-Protokoll

### Session-Start (ZWINGEND!)
1. `memory/beehive-discipline.md` lesen
2. `memory/beehive-architecture.md` lesen
3. Alle `.h` Dateien lesen (Glob: `src/**/*.h`) — vollständiger Überblick
4. Erst DANN mit User besprechen was heute gemacht wird

### Session-Ende (ZWINGEND!)
1. `memory/beehive-architecture.md` updaten (neuer Stand, neue Dateien, neue Structs)
2. `memory/MEMORY.md` updaten (aktueller Stand + nächste Schritte)

### Memory-Pflege (DAUERAUFTRAG!)
- **So oft wie möglich updaten** — nicht nur am Ende
- Bei jeder Architektur-Entscheidung sofort schreiben
- Bei jedem neuen Feature/System: sofort dokumentieren
- Ziel: Bei unerwartetem Abbruch ist trotzdem alles dokumentiert

## Build
```bash
make run    # kompilieren + starten
make        # nur kompilieren
make clean  # aufräumen
```

## Plattform
- Primär: **macOS** — Trackpad, keine separate rechte Maustaste
- Debug-Steuerung immer mit Tastatur lösen, nicht mit rechter Maustaste
- Primär: **macOS**
- Später: Android, WebAssembly (Emscripten)
- raylib 5.5 via Homebrew installiert
