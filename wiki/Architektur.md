# Architektur

---

## Code-Struktur

```
src/
  main.c                    ← Einstiegspunkt, Spielschleife, Eingabe
  kern/                     ← Kern-Datentypen (nur Header, keine Logik)
    spielzustand.h          ← Master-Spielzustand-Struct + Fensterkonstanten
    zelle.h                 ← ZellenTyp-Enum + Zelle-Struct
    biene.h                 ← BienZustand, BienRolle, BienBewegung, Biene-Struct
    auftrag.h               ← AuftragsTyp-Enum + Auftrag-Struct
    trachtquelle.h          ← TrachtTyp-Enum + Trachtquelle-Struct
  szenen/                   ← Ansichts-Renderer (lesen Simulationszustand, zeichnen ihn)
    bienenstock.h/.c        ← ANSICHT_STOCK: Querschnitt des Stocks
    wiese.h/.c              ← ANSICHT_WIESE: Wiese von oben
  sim/                      ← Simulationsmodule (gesamte Logik hier)
    pheromon.h/.c           ← Pheromonfelder: Diffusion + Zerfall
    haushalt.h/.c           ← Reine Physikformeln (kein Zustand)
    agent_biene.h/.c        ← Bienen-KI: Wahrnehmen → Entscheiden → Handeln → Kosten
    sim_stock.h/.c          ← Stocksimulation auf Volksebene
    sim_welt.h/.c           ← Außenwelt: Wetter, Trachtquellen
    pfad_stock.h/.c         ← Indoor-Flowfields (5 Zonen)
    pfad_welt.h/.c          ← Outdoor-Flowfields (pro Quelle + Heimweg)
memory/                     ← Claude-Session-Memory (keine Spieldaten)
assets/                     ← Texturen, Sounds, Fonts (noch leer)
wiki/                       ← Diese Dokumentation
Makefile
CLAUDE.md
```

---

## Der Master-Zustand (`Spielzustand`)

Alles lebt in einem `static Spielzustand spiel` in `main.c`. Keine dynamische Speicherallokation. Alle Subsysteme lesen und schreiben diesen Struct.

```c
typedef struct {
    // Ansicht
    Ansicht ansicht;           // ANSICHT_STOCK oder ANSICHT_WIESE
    float   zeit;              // Verstrichene Zeit in Sekunden

    // Ressourcen (echte physikalische Einheiten)
    float honig_reif_g;        // Reifer Honig (Gramm)
    float nektar_unreif_g;     // Nektar in Verarbeitung
    float pollen_g;            // Pollenvorrat
    float wasser_g;            // Wasservorrat
    float wachs_mg;            // Wachsvorrat (Milligramm)

    // Brutzähler pro Stadium
    int brut_ei, brut_larve, brut_puppe;

    // Klima
    float temperatur_c;        // Brutnest-Temperatur (Ziel: 35 °C)
    float bedrohungslevel;     // Alarmpheromon-Level am Eingang

    // Wetter
    float tageszeit;           // 0,0 = Mitternacht, 0,5 = Mittag, 1,0 = Mitternacht
    float aussentemp_c;        // Außentemperatur
    float wind_staerke;        // 0,0 (windstill) bis 1,0 (Sturm)
    bool  regen;               // Regen blockiert alle Ausflüge
    float wetter_timer;
    bool  flug_gesperrt;       // Berechnet aus Wetter; wird von agent_biene.c gelesen

    // Flache Simulations-Arrays
    Biene        bienen[1000];
    Zelle        zellen[578];
    Auftrag      auftraege[256];
    Trachtquelle quellen[64];
    int bienen_anzahl, zellen_anzahl, auftraege_anzahl, quellen_anzahl;

    // Pheromonfelder
    PheromonFeld pheromonfelder[3];
    bool         pheromon_debug;

    // Legacy-HUD-Felder
    int   volksgroesse;
    float honig, pollen;       // 0,0–1,0 normiert, wird synchron gehalten
} Spielzustand;
```

**Warum `static`?** Der Struct ist über 300 KB groß. Stack-Allokation würde überlaufen. Heap-Allokation würde überall einen Zeiger erfordern. `static` gibt eine feste Adresse, wird null-initialisiert, keine Fragmentierung.

---

## Spielschleifen-Reihenfolge

```
Startup:
  bienenstock_init   → 578 Hexgitterzellen in spiel->zellen[] füllen
  wiese_init         → Dekorative Blumen, Bäume, Wiesenbienen platzieren
  pheromon_init      → Alle drei 128×72-Felder auf null setzen
  haushalt_selbsttest → Physikformeln per Assertion verifizieren
  welt_init          → 4 Trachtquellen platzieren, Startzeit setzen
  stock_init         → Ressourcen setzen, Brut zählen, 10 Startbienen erzeugen
  pfad_stock_init    → BFS für 5 Indoor-Flowfields berechnen
  pfad_welt_init     → BFS für Outdoor-Flowfields pro Quelle berechnen

Pro Frame (60 FPS Ziel):
  1. Eingabe verarbeiten (TAB, P, LMB, S)
  2. welt_aktualisieren        → Zeit vorrücken, Wetter, Quellen nachfüllen
  3. pheromon_aktualisieren    → Diffusions- und Zerfallsschritt
  4. alle_bienen_aktualisieren → KI für alle aktiven Bienen ausführen
  5. stock_aktualisieren       → Brut, Thermoregulation, Nektar, Auftragsqueue
  6. [Szene aktualisieren]     → bienenstock_aktualisieren oder wiese_aktualisieren
  7. BeginDrawing()
  8. [Szene zeichnen]          → bienenstock_zeichnen oder wiese_zeichnen
  9. [Pheromon-Overlay]        → wenn pheromon_debug aktiv
  10. EndDrawing()
```

**Grundregel:** Renderer lesen `spiel` nur — sie schreiben nie. Die Simulation läuft mit voller Geschwindigkeit unabhängig davon, welche Ansicht gerade aktiv ist.

---

## Datendesign-Philosophie

### Flache Arrays (kein malloc pro Biene)

```c
Biene bienen[1000];   // NICHT: Biene** bienen; bienen[i] = malloc(sizeof(Biene))
```

Alle Bienen liegen in einem zusammenhängenden Speicherblock. Das ist cache-freundlich: Das Iterieren über alle Bienen liest den Speicher linear, was moderne CPUs effizient vorausladen (prefetchen) können.

Tote Bienen haben `aktiv = false`. Ihr Slot wird von der nächsten schlüpfenden Biene wiederverwendet. Kein Freigeben, keine Fragmentierung, kein Zeiger-Jagen.

### Eine Datei = Eine Verantwortung

Jede `.c`-Datei besitzt genau einen Zuständigkeitsbereich:
- `sim_stock.c` — Stocksimulation auf Volksebene
- `agent_biene.c` — Bienen-KI pro Biene
- `bienenstock.c` — Rendering der Stock-Ansicht
- Simulationslogik gehört **nie** in Renderer, Rendering-Code **nie** in die Simulation

### Header zuerst

Jede `.h`-Datei definiert die vollständige öffentliche API, bevor die `.c`-Datei geschrieben wird. Das erzwingt klares Interface-Design und macht das gesamte Projekt allein durch die Header verständlich.

---

## Hexgitter

Die Stock-Ansicht nutzt ein **flach liegendes Hexagon-Gitter** (flat-top):

```
Radius (Spitze zu Spitze / 2):   18 px
Spaltenschritt (x):               18 × 1,5   = 27 px
Zeilenschritt (y):                18 × √3    ≈ 31,2 px
Ungerade-Spalten-Versatz (y):     18 × 0,866 ≈ 15,6 px
Gitter-Start:                     (162, 72)
Gittergröße:                      34 Spalten × 17 Zeilen = 578 Zellen
```

Zellen-Index-Formel: `index = zeile × HEX_SPALTEN + spalte`

Der Zellentyp wird per Abstand von der Mitte bestimmt (`berechne_zellentyp`):

| Abstand von der Mitte | Zone |
|-----------------------|------|
| < 18 % des Radius | Brut-Eier (innerer Kern) |
| 18–50 % | Brut-Larven + etwas Pollen |
| 50–80 % | Honig + Pollen |
| > 80 % | Honig + Leer (äußerer Ring) |

Mit deterministischem Rauschen pro Zelle für natürliche Variation.

---

## Build

```bash
make        # nur kompilieren → ./beehive
make run    # kompilieren + starten
make clean  # .o-Dateien und Binary löschen
```

Compiler: `gcc -Wall -O2`
Abhängigkeiten: raylib 5.5 (via Homebrew unter `/opt/homebrew`)
Plattform: macOS (Apple Silicon M2, OpenGL 4.1 Metal)
