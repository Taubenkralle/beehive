# Beehive — Implementierungs-Fahrplan

## ⚠️ WICHTIG: Diesen Fahrplan VOR dem Coden lesen!
- Jede Phase wird vollständig abgeschlossen bevor die nächste beginnt
- Nach jedem Schritt: Haken setzen [ ] → [x] und Memory updaten
- Wenn Session unterbrochen wird: Aktuellen Schritt markieren, Memory updaten

---

## Aktueller Stand
**Aktive Phase:** Spätere Milestones (Phase 10 abgeschlossen)
**Letzter abgeschlossener Schritt:** Phase 10 vollständig ✅

---

## Übersicht aller Phasen

| Phase | Titel | Status |
|---|---|---|
| 1 | Datenmodell — Fundament | ✅ |
| 2 | Pheromon-System | ✅ |
| 3 | Haushalt — Physikalische Formeln | ✅ |
| 4 | Agent-Biene — KI & Zustandsmaschine | ✅ |
| 5 | Sim-Stock — Stocksimulation | ✅ |
| 6 | Sim-Welt — Außenweltsimulation | ✅ |
| 7 | Pathfinding — Flowfields | ✅ |
| 8 | Integration — Simulation trifft Rendering | ✅ |
| 9 | Visuelle Überarbeitung — Realistische Farben | ✅ |
| 10 | Speichersystem | ✅ |

---

## Phase 1: Datenmodell — Fundament
**Ziel:** Alle Kern-Datenstrukturen definieren. Kein Verhalten, nur Typen und Structs.
**Dateien:** `src/kern/` (neue + erweiterte Header)
**Abhängigkeiten:** Keine — kann sofort starten.

### Schritt 1.1 — Zellentypen erweitern (`src/kern/zelle.h`)
- [x] Neue Datei `src/kern/zelle.h` anlegen
- [ ] `ZellenTyp`-Enum definieren:
  ```c
  ZELLE_LEER, ZELLE_BRUT_EI, ZELLE_BRUT_LARVE, ZELLE_BRUT_PUPPE,
  ZELLE_HONIG_OFFEN, ZELLE_HONIG_VERD, ZELLE_POLLEN,
  ZELLE_WACHSBAU, ZELLE_KOENIGIN, ZELLE_BLOCKIERT
  ```
- [ ] `Zelle`-Struct definieren:
  ```c
  typedef struct {
      ZellenTyp typ;
      float     inhalt;      // Füllstand (0.0 - 1.0)
      float     alter_t;     // Alter des Inhalts in Tagen
      bool      sauber;      // Für Eiablage geeignet
  } Zelle;
  ```

### Schritt 1.2 — Bienentypen definieren (`src/kern/biene.h`)
- [ ] Neue Datei `src/kern/biene.h` anlegen
- [ ] `BienZustand`-Enum (Entwicklungsstadien):
  ```c
  BIEN_EI, BIEN_LARVE, BIEN_PUPPE, BIEN_ADULT
  ```
- [ ] `BienRolle`-Enum (Rollen erwachsener Bienen):
  ```c
  PUTZERIN, AMME, BAUERIN, EMPFAENGERIN, FAECHER, WAECHTER,
  BESTATERIN, SAMMLERIN_NEKTAR, SAMMLERIN_POLLEN,
  SAMMLERIN_WASSER, SAMMLERIN_PROPOLIS, ERKUNDERIN
  ```
- [ ] `BienBewegung`-Enum (Bewegungszustände):
  ```c
  IM_STOCK_ARBEITEND, ZUM_AUSGANG, AUSSENFLUG,
  AM_SAMMELN, HEIMFLUG, ZUM_ABLADEN, RASTEN
  ```
- [ ] `Biene`-Struct definieren:
  ```c
  typedef struct {
      BienZustand zustand;
      BienRolle   rolle;
      BienBewegung bewegung;
      float alter_t;           // Alter in Tagen
      float energie_j;         // Energievorrat
      float honigmagen_mg;     // Nektar im Honigmagen
      float pollenladung_mg;   // Pollen an Hinterbeinen
      float wasser_mg;
      float gesundheit;        // 0.0 - 1.0
      float pos_x, pos_y;
      float ziel_x, ziel_y;
      int   auftrag_id;        // -1 = kein Auftrag
      bool  aktiv;             // false = tot (Slot wiederverwendbar)
  } Biene;
  ```

### Schritt 1.3 — Auftragssystem definieren (`src/kern/auftrag.h`)
- [ ] Neue Datei `src/kern/auftrag.h` anlegen
- [ ] `AuftragsTyp`-Enum:
  ```c
  AUFTRAG_ZELLE_REINIGEN, AUFTRAG_LARVE_FUETTERN,
  AUFTRAG_WABE_BAUEN, AUFTRAG_NEKTAR_VERARBEITEN,
  AUFTRAG_FAECHELN, AUFTRAG_EINGANG_BEWACHEN,
  AUFTRAG_LEICHE_ENTFERNEN, AUFTRAG_NEKTAR_SAMMELN,
  AUFTRAG_POLLEN_SAMMELN, AUFTRAG_WASSER_HOLEN,
  AUFTRAG_PROPOLIS_HOLEN, AUFTRAG_ERKUNDEN
  ```
- [ ] `Auftrag`-Struct:
  ```c
  typedef struct {
      AuftragsTyp typ;
      int   ziel_zelle;    // Zell-Index (-1 wenn nicht relevant)
      float ziel_x, ziel_y;
      int   prioritaet;    // 1 = höchste
      bool  vergeben;      // Bereits einer Biene zugewiesen
  } Auftrag;
  ```

### Schritt 1.4 — Trachtquellen definieren (`src/kern/trachtquelle.h`)
- [ ] Neue Datei `src/kern/trachtquelle.h` anlegen
- [ ] `TrachtTyp`-Enum: `TRACHT_NEKTAR, TRACHT_POLLEN, TRACHT_WASSER, TRACHT_PROPOLIS`
- [ ] `Trachtquelle`-Struct:
  ```c
  typedef struct {
      TrachtTyp typ;
      float pos_x, pos_y;
      float vorrat_g;        // Aktueller Vorrat
      float kapazitaet_g;    // Maximum
      float regeneration_g_s; // Nachfüllrate pro Sekunde
      float risiko;          // 0.0 - 1.0 (Gefahrenlevel)
      bool  aktiv;
  } Trachtquelle;
  ```

### Schritt 1.5 — Spielzustand erweitern (`src/kern/spielzustand.h`)
- [ ] `Spielzustand`-Struct um Simulationsdaten erweitern:
  ```c
  // Ressourcen (reale Mengen, nicht mehr 0.0-1.0)
  float honig_reif_g;
  float nektar_unreif_g;
  float pollen_g;
  float wasser_g;
  float wachs_mg;
  // Brut
  int brut_ei, brut_larve, brut_puppe;
  // Klima
  float temperatur_c;
  float bedrohungslevel;
  // Flat Arrays (Performance!)
  Biene        bienen[MAX_BIENEN];       // z.B. MAX_BIENEN 5000
  Zelle        zellen[MAX_ZELLEN];       // Wabenraster flat
  Auftrag      auftraege[MAX_AUFTRAEGE];
  Trachtquelle quellen[MAX_QUELLEN];
  int bienen_anzahl;
  int zellen_anzahl;
  int auftraege_anzahl;
  int quellen_anzahl;
  ```
- [ ] Konstanten festlegen: `MAX_BIENEN`, `MAX_ZELLEN`, `MAX_AUFTRAEGE`, `MAX_QUELLEN`

**Phase 1 abgeschlossen wenn:** Alle Header compilieren ohne Fehler, keine .c Dateien nötig.

---

## Phase 2: Pheromon-System (`src/sim/pheromon.h/.c`)
**Ziel:** Flat 2D-Grid für Pheromonkonzentrationen. Diffusion, Zerfall, Emission.
**Abhängigkeiten:** Phase 1 abgeschlossen.

### Schritt 2.1 — Header (`src/sim/pheromon.h`)
- [ ] Neues Verzeichnis `src/sim/` anlegen
- [ ] `PheromonTyp`-Enum:
  ```c
  PHEROMON_SPUR,    // Spurpheromon (Sammlerinnen → Trachtquelle)
  PHEROMON_ALARM,   // Alarmpheromon (Wächter → Gefahr)
  PHEROMON_NASONOV  // Orientierung (Heimweg)
  ```
- [ ] `PheromonFeld`-Struct (ein Feld pro Typ):
  ```c
  #define PHEROMON_BREITE 128
  #define PHEROMON_HOEHE  72
  typedef struct {
      float zellen[PHEROMON_BREITE * PHEROMON_HOEHE];
      float diffusion;    // D-Koeffizient
      float zerfall;      // k-Koeffizient
  } PheromonFeld;
  ```
- [ ] Öffentliche API:
  ```c
  void pheromon_init(PheromonFeld felder[3]);
  void pheromon_aktualisieren(PheromonFeld felder[3], float dt);
  void pheromon_abgeben(PheromonFeld* feld, float x, float y, float menge);
  float pheromon_lesen(const PheromonFeld* feld, float x, float y);
  ```

### Schritt 2.2 — Implementierung (`src/sim/pheromon.c`)
- [ ] `pheromon_aktualisieren()`: Diffusion via diskreter Laplace-Operator + Zerfall
  ```
  C_neu = C + D * Laplace(C) * dt - k * C * dt
  ```
- [ ] `pheromon_abgeben()`: Quelle S addieren an Position
- [ ] `pheromon_lesen()`: Bilineare Interpolation für glatte Gradienten
- [ ] Randbedingungen: Kein Pheromon außerhalb des Grids

### Schritt 2.3 — Test (visuell)
- [ ] Pheromon-Overlay in einer der Ansichten anzeigen (Heatmap, optional abschaltbar)
- [ ] Manuell Pheromon setzen und Diffusion beobachten

**Phase 2 abgeschlossen wenn:** Pheromon verbreitet sich sichtbar, zerfällt, bleibt in Bounds.

---

## Phase 3: Haushalt — Physikalische Formeln (`src/sim/haushalt.h/.c`)
**Ziel:** Alle Rechen-Formeln als reine Funktionen (kein Zustand, nur Mathe).
**Abhängigkeiten:** Phase 1 abgeschlossen.

### Schritt 3.1 — Header (`src/sim/haushalt.h`)
- [ ] Konstanten definieren:
  ```c
  #define ZUCKERGEHALT_NEKTAR   0.30f  // 30% Zucker im Nektar
  #define WASSERANTEIL_HONIG    0.18f  // 18% Wasser im reifen Honig
  #define ENERGIE_DICHTE_J_G    17.0f  // Joule pro Gramm Zucker
  #define LEISTUNG_FLUG_W       0.014f // Watt beim Fliegen
  #define LEISTUNG_LAUFEN_W     0.003f // Watt beim Laufen
  #define LEISTUNG_STEHEN_W     0.001f // Watt im Ruhezustand
  #define WACHS_AUSBEUTE        8.0f   // mg Wachs pro g Zucker
  #define WACHS_PRO_ZELLE_MG   1300.0f // mg Wachs für eine Zelle
  ```
- [ ] API:
  ```c
  float honig_aus_nektar(float nektar_g, float zuckergehalt);
  float energie_verbrauch(float leistung_w, float dt);
  float energie_aufnahme(float zucker_g);
  float wachs_aus_zucker(float zucker_g);
  float nektar_verdunstung(float wasser_g, int faecher_anzahl, float dt);
  float patch_profit(float zucker_pro_min, float flugzeit_s, float risiko);
  int   rekrutierung(float profit, float max_rekrutierung, float skalierung_k);
  ```

### Schritt 3.2 — Implementierung (`src/sim/haushalt.c`)
- [ ] Alle Formeln aus dem Blueprint implementieren (reine Funktionen, kein Side-Effect)
- [ ] Unit-Tests als einfache assert()-Aufrufe (optional aber empfohlen)

**Phase 3 abgeschlossen wenn:** Alle Formeln rechnen korrekt, kompilieren ohne Fehler.

---

## Phase 4: Agent-Biene — KI & Zustandsmaschine (`src/sim/agent_biene.h/.c`)
**Ziel:** Jede Biene kann: Wahrnehmen → Entscheiden → Handeln → Kosten zahlen.
**Abhängigkeiten:** Phase 1, 2, 3.

### Schritt 4.1 — Header (`src/sim/agent_biene.h`)
- [ ] API:
  ```c
  void biene_init(Biene* b, float x, float y, BienRolle rolle);
  void biene_aktualisieren(Biene* b, Spielzustand* spiel, float dt);
  void biene_rolle_waehlen(Biene* b, const Spielzustand* spiel);
  void biene_auftrag_holen(Biene* b, Spielzustand* spiel);
  void biene_bewegen(Biene* b, float dt);
  void biene_energie_verbrauchen(Biene* b, float leistung_w, float dt);
  void biene_ressource_laden(Biene* b, TrachtTyp typ, float menge_mg);
  void biene_ressource_abladen(Biene* b, Spielzustand* spiel);
  void biene_pheromon_abgeben(Biene* b, PheromonFeld* feld, PheromonTyp typ, float dt);
  bool biene_ist_tot(const Biene* b);
  ```

### Schritt 4.2 — Sense (Wahrnehmen)
- [ ] Nahe Pheromonkonzentration lesen
- [ ] Nächste passende Zelle finden (für Auftrag)
- [ ] Energielevel prüfen → ggf. Heimflug erzwingen

### Schritt 4.3 — Decide (Entscheiden)
- [ ] Auftrag aus Warteschlange holen (nach Priorität)
- [ ] Rolle wechseln wenn Bedarf sich ändert
- [ ] Bewegungszustand wechseln (IM_STOCK → ZUM_AUSGANG → AUSSENFLUG etc.)

### Schritt 4.4 — Act (Handeln)
- [ ] Je Bewegungszustand: korrekte Aktion ausführen
- [ ] Zellen-Aktionen: `zelle_reinigen()`, `larve_fuettern()`, `nektar_empfangen()` etc.
- [ ] Pheromon abgeben je nach Aktion/Rolle

### Schritt 4.5 — PayCost (Kosten zahlen)
- [ ] Energie abziehen je nach Aktion (Haushalt-Formeln nutzen)
- [ ] Bei Energie = 0: Biene stirbt (`aktiv = false`)

**Phase 4 abgeschlossen wenn:** Einzelne Biene kann einen kompletten Auftrag durchführen (holen → ausführen → abliefern → neue Aufgabe).

---

## Phase 5: Sim-Stock — Stocksimulation (`src/sim/sim_stock.h/.c`)
**Ziel:** Der Stock als Ganzes simuliert: Brut, Klima, Auftrags-Generierung, Nektar-Pipeline.
**Abhängigkeiten:** Phase 1, 3, 4.

### Schritt 5.1 — Header (`src/sim/sim_stock.h`)
- [ ] API:
  ```c
  void stock_init(Spielzustand* spiel);
  void stock_aktualisieren(Spielzustand* spiel, float dt);
  void stock_auftraege_generieren(Spielzustand* spiel);
  void stock_brut_aktualisieren(Spielzustand* spiel, float dt);
  void stock_nektar_verarbeiten(Spielzustand* spiel, float dt);
  void stock_thermoregulieren(Spielzustand* spiel, float dt);
  void stock_wabe_bauen(Spielzustand* spiel, float dt);
  ```

### Schritt 5.2 — Brut-Update
- [ ] Alter jedes Ei/Larve/Puppe um dt erhöhen
- [ ] Schwellen: Ei→Larve nach `EI_DAUER_T` Tagen, Larve→Puppe nach `LARVEN_DAUER_T`, Puppe→Biene nach `PUPPEN_DAUER_T`
- [ ] Neue Biene: `biene_init()` aufrufen, Rolle zuweisen
- [ ] Sterblichkeit: `p_tod = basis + stress(temp, nahrung)`

### Schritt 5.3 — Auftrags-Generierung (Bedarf → Aufträge)
- [ ] Brut prüfen → `AUFTRAG_LARVE_FUETTERN` erstellen wenn Larven vorhanden
- [ ] Leere saubere Zellen prüfen → Königin bekommt `AUFTRAG_EI_LEGEN` (implizit)
- [ ] Temperatur prüfen → `AUFTRAG_FAECHELN` wenn zu heiß
- [ ] Nektar-Vorrat prüfen → `AUFTRAG_NEKTAR_VERARBEITEN`
- [ ] Platz prüfen → `AUFTRAG_WABE_BAUEN` wenn Lager voll
- [ ] Außen-Aufträge nach Ressourcenbedarf

### Schritt 5.4 — Nektar-Pipeline
- [ ] Nektar (unreif) → Wasser verdunsten lassen (Fächer-Bienen)
- [ ] Wenn Wasseranteil < `WASSERANTEIL_HONIG`: Honig ist reif → Zelle verdeckeln
- [ ] `honig_reif_g` aktualisieren

### Schritt 5.5 — Thermoregulation
- [ ] Ziel: 35°C im Brutbereich
- [ ] Zu kalt → Heiz-Bienen erhöhen Energieverbrauch
- [ ] Zu heiß → Fächer-Bienen + Wasser-Kühlung

**Phase 5 abgeschlossen wenn:** Stock generiert Aufträge, Brut entwickelt sich zeitabhängig, Honig reift.

---

## Phase 6: Sim-Welt — Außenweltsimulation (`src/sim/sim_welt.h/.c`)
**Ziel:** Trachtquellen regenerieren, Wetter simulieren, Gefahren.
**Abhängigkeiten:** Phase 1, 3.

### Schritt 6.1 — Header (`src/sim/sim_welt.h`)
- [ ] API:
  ```c
  void welt_init(Spielzustand* spiel);
  void welt_aktualisieren(Spielzustand* spiel, float dt);
  void trachtquelle_aktualisieren(Trachtquelle* q, float dt);
  void wetter_aktualisieren(Spielzustand* spiel, float dt);
  bool flug_moeglich(const Spielzustand* spiel);
  ```

### Schritt 6.2 — Trachtquellen-Regeneration
- [ ] Vorrat auffüllen nach `regeneration_g_s * dt`
- [ ] Tageszeitkurve: Multiplikator je Tageszeit (morgens/mittags/abends)
- [ ] Saisonfaktor (später: Winter = keine Blüten)

### Schritt 6.3 — Wetter
- [ ] `regen` (bool) → `flug_moeglich()` = false
- [ ] `wind` (float) → erhöht Flug-Energiekosten
- [ ] `temperatur_c` → beeinflusst Trachtqualität

**Phase 6 abgeschlossen wenn:** Trachtquellen werden endlich, regenerieren, Wetter blockt Ausflüge.

---

## Phase 7: Pathfinding — Flowfields (`src/sim/pfad_stock.h/.c`, `pfad_welt.h/.c`)
**Ziel:** Bienen navigieren effizient — auch bei tausenden gleichzeitig.
**Abhängigkeiten:** Phase 1, 4.

### Schritt 7.1 — Innen-Flowfield (`src/sim/pfad_stock.h/.c`)
- [ ] Zonen definieren: `ZONE_BRUT`, `ZONE_HONIG`, `ZONE_POLLEN`, `ZONE_EINGANG`, `ZONE_KOENIGIN`
- [ ] Pro Zone: Ein Flowfield auf dem Gangweg-Grid berechnen (BFS vom Ziel aus)
- [ ] `stroemungsfeld_stock_berechnen(zone)` → 2D-Array mit Richtungsvektoren
- [ ] Biene liest Flowfield an ihrer Position → Bewegungsrichtung

### Schritt 7.2 — Außen-Flowfield (`src/sim/pfad_welt.h/.c`)
- [ ] Pro Trachtquelle: Ein Flowfield berechnen
- [ ] `stroemungsfeld_welt_berechnen(quelle_id)` → 2D-Array mit Richtungsvektoren
- [ ] Flowfield nur neu berechnen wenn Quelle sich ändert (cachen!)
- [ ] Gefahrenzonen als erhöhte Kosten einrechnen

**Phase 7 abgeschlossen wenn:** Hunderte Bienen navigieren kollisionsfrei zu Zielen ohne A* pro Biene.

---

## Phase 8: Integration — Simulation trifft Rendering
**Ziel:** Bestehende Szenen-Dateien lesen echte Simulationsdaten statt Dummy-Daten.
**Abhängigkeiten:** Phase 1–7.

### Schritt 8.1 — `main.c` anpassen
- [x] Core Loop: `welt_aktualisieren()` → `stock_aktualisieren()` → `agenten_aktualisieren()` in main.c
- [x] Rendering vollständig von Simulation getrennt

### Schritt 8.2 — `bienenstock.c` mit echten Daten verbinden
- [x] `bienenstock_init()`: füllt alle 578 `spiel->zellen[]` aus Hex-Grid
- [x] `bienenstock_aktualisieren()`: leer (kein dummy, kein zeit-Inkrement)
- [x] Wabenraster liest `spiel->zellen[z*HEX_SPALTEN+s].typ`
- [x] Bienen aus `spiel->bienen[]` (nur Indoor-States gezeichnet)
- [x] Königin bei `koenigin_pos` (Hex-Mitte)
- [x] HUD: Volksgroesse, honig_reif_g, nektar_unreif_g, pollen_g, temperatur_c, Brutzahlen

### Schritt 8.3 — `wiese.c` mit echten Daten verbinden
- [x] `wiese_aktualisieren()`: kein `spiel->zeit += delta` mehr
- [x] Trachtquellen-Indikatoren aus `spiel->quellen[]` (Kreis-Füllstand, N/P/W Labels)
- [x] Outdoor-Sim-Bienen aus `spiel->bienen[]` (AUSSENFLUG/AM_SAMMELN/HEIMFLUG)
- [x] Dekorative `wbienen[]` bleiben als Hintergrundleben
- [x] HUD: Uhrzeit (hh:mm), Temp, Wind, Regen/Flug-Gesperrt, Ressourcen

### Schritt 8.4 — sim_stock.c repariert
- [x] `stock_init()`: kein `zellen_anzahl`-Reset mehr; zählt Brut aus bestehenden Zellen

**Phase 8 abgeschlossen wenn:** Simulation läuft durch, Rendering zeigt echte Daten, kein Dummy mehr.
→ **✅ ABGESCHLOSSEN** (01.03.2026)

---

## Phase 9: Visuelle Überarbeitung — Realistische Farben
**Ziel:** Farben aus Referenzbildern umsetzen. Natürlich, warm, organisch.
**Abhängigkeiten:** Phase 8.

### Schritt 9.1 — Innenansicht (Waben)
- [ ] Zellenfarben nach Typ (Farbpalette aus colors.jpeg):
  - `ZELLE_HONIG_OFFEN`: `#B85C00` – `#D4700A` (Tief-Amber)
  - `ZELLE_HONIG_VERD`: `#E8C84A` – `#F0D060` (Mattes Cremegelb)
  - `ZELLE_POLLEN`: `#E8820A` – `#F5A020` (Orange-Gelb)
  - `ZELLE_WACHSBAU`: `#EDD870` – `#F5E8A0` (Cremeweiß)
  - `ZELLE_LEER`: `#7A4A10` – `#8B5520` (Dunkles Amber)
  - Zellwände: `#C17A00` – `#D4890A` (Goldgelb)
- [ ] Bienen-Sprite: Dunkelbraun + dezente Gelb-Streifen (kein Comic-Gelb)

### Schritt 9.2 — Außenansicht (Wiese + Stock-Gebäude)
- [ ] Stock-Gebäude: Oval/tropfenförmig hängend (Referenz: hive.jpeg)
  - Außen: `#1A0F0A` – `#2E1A0E` (Fast schwarz)
  - Mitte: `#3D2010` – `#5C3318` (Dunkelbraun)
  - Highlights: `#8B5E2A` – `#C4852A` (Honigbraun)
- [ ] Wiesen-Hintergrund: Kräftiges Grün `#2D5A1B` – `#4A8C2A`
- [ ] Blumen: Rote Akzente `#C42020` – `#E03030`

**Phase 9 abgeschlossen wenn:** Spiel sieht naturalistisch aus, keine Plastikfarben mehr.

---

## Phase 10: Speichersystem (`src/sim/speicher.h/.c`)
**Ziel:** Spielstand speichern und laden.
**Abhängigkeiten:** Phase 8.

- [x] Binäres Format: SpeicherHeader (magic "BEEH", version, datengroesse) + raw Spielzustand
- [x] `speicher_schreiben(const Spielzustand* spiel, const char* pfad)` → beehive.sav
- [x] `speicher_lesen(Spielzustand* spiel, const char* pfad)` — prüft magic, version, size
- [x] `speicher_vorhanden(const char* pfad)` — Existenz-Check
- [x] Auto-Load beim Start wenn beehive.sav vorhanden
- [x] F5: Speichern, F9: Laden — Status-Meldung 2 Sekunden eingeblendet

---

## Spätere Milestones (nach Phase 10)
- Jahreszeiten (Winter-Überlebens-Mechanik)
- Feinde (Wespen, Hornissen, Spinnennetz)
- Schwanzeltanz visuell darstellen
- Backend / Cloud-Save (Firebase)
- WebAssembly-Build (`make web`)
