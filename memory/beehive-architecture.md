# Beehive — Architektur & aktueller Stand

## ⚠️ OBERSTE PRÄMISSE
**Realismus vor Spielbarkeit.** Der Simulator soll biologisch korrekt funktionieren.
Echte Mechanismen (Schwanzeltanz, Pheromonkommunikation, Energiehaushalt, Brutpflege)
werden so nah wie möglich an der Natur modelliert.

**Deutsche Identifier sind PFLICHT** — nicht nur Konvention, sondern bewusste Entscheidung:
- Der Code soll für den Entwickler (nicht für KI) intuitiv lesbar sein
- Englische Bezeichner aus KI-Vorschlägen IMMER ins Deutsche übersetzen
- Ziel: Man liest `schwanzeltanz()` und versteht sofort was passiert

---

## Dateistruktur

```
src/
  main.c                    ← Einstiegspunkt, Spielschleife, TAB-Wechsel
  kern/
    spielzustand.h          ← Spielzustand, Konstanten, inkludiert alle kern-Header
    zelle.h                 ← ZellenTyp-Enum, Zelle-Struct
    biene.h                 ← BienZustand, BienRolle, BienBewegung, Biene-Struct
    auftrag.h               ← AuftragsTyp-Enum, Auftrag-Struct
    trachtquelle.h          ← TrachtTyp-Enum, Trachtquelle-Struct
  szenen/
    bienenstock.h/.c        ← ANSICHT_STOCK: Querschnitt (Wabenraster, Bienen im Stock)
    wiese.h/.c              ← ANSICHT_WIESE: Vogelperspektive (Blumen, Flugbienen)
  sim/
    pheromon.h/.c           ← Pheromon-Gitter: Diffusion, Zerfall, Overlay [Phase 2]
    haushalt.h/.c           ← Physikalische Formeln: Honig, Energie, Wachs [Phase 3]
    agent_biene.h/.c        ← Bienen-KI: Sense→Decide→Act→PayCost [Phase 4]
    sim_stock.h/.c          ← Stocksimulation: Brut, Klima, Nektar, Aufträge [Phase 5]
    sim_welt.h/.c           ← Außenwelt: Trachtquellen, Tageszeit, Wetter [Phase 6]
    pfad_stock.h/.c         ← Indoor-Flowfields (5 Zonen, BFS auf 64×36 Grid) [Phase 7]
    pfad_welt.h/.c          ← Outdoor-Flowfields (pro Quelle + Heimweg) [Phase 7]
memory/
  beehive-discipline.md     ← Disziplin-Regeln, Naming, Session-Protokoll
  beehive-architecture.md   ← Diese Datei
assets/                     ← Texturen, Sounds, Fonts (noch leer)
Makefile
CLAUDE.md
```

---

## Kern-Typen (aktuell implementiert)

### `Spielzustand` (kern/spielzustand.h) — vollständig Phase 1-5
```c
typedef struct {
    Ansicht ansicht;  float zeit;
    // Ressourcen (echte Mengen)
    float honig_reif_g, nektar_unreif_g, pollen_g, wasser_g, wachs_mg;
    // Brut
    int brut_ei, brut_larve, brut_puppe;
    // Klima
    float temperatur_c, bedrohungslevel;
    // Flat Arrays
    Biene        bienen[MAX_BIENEN];       // 1000
    Zelle        zellen[MAX_ZELLEN];       // 578
    Auftrag      auftraege[MAX_AUFTRAEGE]; // 256 — mit aktiv+vergeben Flags
    Trachtquelle quellen[MAX_QUELLEN];     // 64
    int bienen_anzahl, zellen_anzahl, auftraege_anzahl, quellen_anzahl;
    PheromonFeld pheromonfelder[PHEROMON_ANZAHL];
    bool pheromon_debug;
    // Legacy HUD (bis Phase 8)
    int volksgroesse; float honig, pollen;
} Spielzustand;
```

### Fensterkonstanten
- `FENSTER_BREITE` = 1280
- `FENSTER_HOEHE` = 720
- `ZIEL_FPS` = 60

---

## Modul-APIs (aktuell implementiert)

### bienenstock (szenen/bienenstock.h)
```c
void bienenstock_init(Spielzustand* spiel);
void bienenstock_aktualisieren(Spielzustand* spiel, float delta);
void bienenstock_zeichnen(const Spielzustand* spiel);
void bienenstock_aufraeumen(Spielzustand* spiel);
```

### wiese (szenen/wiese.h)
```c
void wiese_init(Spielzustand* spiel);
void wiese_aktualisieren(Spielzustand* spiel, float delta);
void wiese_zeichnen(const Spielzustand* spiel);
void wiese_aufraeumen(Spielzustand* spiel);
```

### pheromon (sim/pheromon.h) — Phase 2
```c
void  pheromon_init(PheromonFeld felder[PHEROMON_ANZAHL]);
void  pheromon_aktualisieren(PheromonFeld felder[PHEROMON_ANZAHL], float dt);
void  pheromon_abgeben(PheromonFeld* feld, float x, float y, float menge);
float pheromon_lesen(const PheromonFeld* feld, float x, float y);
void  pheromon_zeichnen(const PheromonFeld* feld, PheromonTyp typ);
// 3 Typen: PHEROMON_SPUR, PHEROMON_ALARM, PHEROMON_NASONOV
// Gitter: 128×72 Zellen, Diffusion + Zerfall per Laplace-Operator
```

### haushalt (sim/haushalt.h) — Phase 3
```c
float honig_aus_nektar(float nektar_g, float zuckergehalt);
float energie_verbrauch(float leistung_w, float dt);   // W×s = Joule
float energie_aufnahme(float zucker_g);
float wachs_aus_zucker(float zucker_g);
float nektar_verdunstung(int faecher_anzahl, float dt);
float patch_profit(float zucker_pro_min, float flugzeit_s, float risiko);
int   rekrutierung(float profit, float max_rekrutierung, float skalierung_k);
float brut_sterblichkeit(float temperatur_c, float nahrungs_mangel);
void  haushalt_selbsttest(void);
// ENERGIE_START_J=1.5, ZUCKERGEHALT_NEKTAR=0.30, WASSERANTEIL_HONIG=0.18
// EI_DAUER_T=3d, LARVEN_DAUER_T=6d, PUPPEN_DAUER_T=12d
// Zeitskala: dt/86400.0f Sekunden → Tage (passend zu biene.alter_t)
```

### agent_biene (sim/agent_biene.h) — Phase 4
```c
void biene_init(Biene* b, float x, float y, BienRolle rolle);
void biene_aktualisieren(Biene* b, Spielzustand* spiel, float dt);
void alle_bienen_aktualisieren(Spielzustand* spiel, float dt);
// Sense→Decide→Act→PayCost Loop pro Tick
// Altersbasierte Rollenentwicklung: <3d=PUTZERIN, <10d=AMME, ... ≥24d=SAMMLERIN
// STOCK_EINGANG_X=130, STOCK_EINGANG_Y=370 (Außenwelt-Portal)
// STOCK_LAGER_X=640, STOCK_LAGER_Y=360 (Abgabezone Innen)
```

### sim_stock (sim/sim_stock.h) — Phase 5
```c
void stock_init(Spielzustand* spiel);         // Ressourcen, Brut, Startbienen, Auftragsqueue
void stock_aktualisieren(Spielzustand* spiel, float dt); // Haupt-Tick: alle Sub-Systeme
void stock_auftraege_generieren(Spielzustand* spiel);    // Bedarf → Auftragsqueue
void stock_brut_aktualisieren(Spielzustand* spiel, float dt); // Ei→Larve→Puppe→Biene
void stock_nektar_verarbeiten(Spielzustand* spiel, float dt);  // Nektar→Honig (Fächer)
void stock_thermoregulieren(Spielzustand* spiel, float dt);    // Bruttemp 35°C
void stock_wabe_bauen(Spielzustand* spiel, float dt);          // Wachs→neue Zellen
// Auftrag.aktiv-Flag: false=Slot frei für Wiederverwendung (Index stabil für Bienen)
// ZONE_BRUT_X/Y, ZONE_HONIG_X/Y etc. als Platzhalter-Koordinaten bis Phase 8
```

### sim_welt (sim/sim_welt.h) — Phase 6
```c
void welt_init(Spielzustand* spiel);              // 4 Trachtquellen + Startwetter
void welt_aktualisieren(Spielzustand* spiel, float dt); // Tageszeit, Wetter, Quellen
void trachtquelle_aktualisieren(Trachtquelle* q, const Spielzustand* spiel, float dt);
void wetter_aktualisieren(Spielzustand* spiel, float dt);
bool flug_moeglich(const Spielzustand* spiel);
// SIM_TAG_DAUER_S=120 (2 Min/Tag), Tageszeit 0.0→1.0 (sin-Kurve für Nektar)
// Wetter: Regen blockt Flug, Wind dämpft Nektar, Temperatur per sin(tageszeit*π)
// spiel->flug_gesperrt = !flug_moeglich() — von agent_biene.c gelesen
// 4 Quellen: Nektar×2, Pollen, Wasser — in ANSICHT_WIESE-Koordinaten
```

### pfad_stock + pfad_welt (sim/pfad_stock.h, pfad_welt.h) — Phase 7
```c
// Grid: PFAD_ZELLEN_GROESSE=20px → PFAD_FF_BREITE=64, PFAD_FF_HOEHE=36 (2304 Zellen)
// Richtung: int8_t dx/dy (-1..+1), normalisiert bei Diagonal-Schritten

// Indoor (ANSICHT_STOCK) — 5 Zonen:
typedef enum { STOCK_ZONE_EINGANG, STOCK_ZONE_LAGER, STOCK_ZONE_BRUT,
               STOCK_ZONE_HONIG, STOCK_ZONE_POLLEN, STOCK_ZONE_ANZAHL } StockZone;
void    pfad_stock_init(void);                              // Alle 5 BFS berechnen
void    pfad_stock_berechnen(StockZone zone);
Vector2 pfad_stock_richtung(float x, float y, StockZone);

// Outdoor (ANSICHT_WIESE) — pro Quelle + Heimweg:
void    pfad_welt_init(const Spielzustand* spiel);
void    pfad_welt_berechnen(int quellen_id, const Spielzustand* spiel);
void    pfad_eingang_berechnen(void);
Vector2 pfad_welt_richtung(float x, float y, int quellen_id);
Vector2 pfad_eingang_richtung(float x, float y);

// biene_bewegen nutzt Flowfields als primäre Navigation, Direct-Steering als Fallback
// biene_bewegen Signatur: (Biene*, const Spielzustand*, float dt)
// Obstacle-Karte: noch leer → Phase 9 füllt sie mit Waben-Geometrie
```

---

## Implementierter Stand (Session 2 — 01.03.2026) — Phasen 1–8 abgeschlossen

### ANSICHT_STOCK (bienenstock.c) — Phase 8 integriert
- `bienenstock_init()` füllt alle 578 `spiel->zellen[]` aus Hex-Grid (34×17)
- Zellentyp-Rendering liest direkt `spiel->zellen[z*HEX_SPALTEN+s].typ`
- Bienen aus `spiel->bienen[]` gezeichnet (Indoor: IM_STOCK, ZUM_AUSGANG, ZUM_ABLADEN, RASTEN)
- Königin immer bei Hex-Mitte (`koenigin_pos`)
- HUD: Volksgroesse, Honig (g), Nektar (g), Pollen (g), Temperatur (°C), Brut (E/L/P Counts)
- Holzrahmen, Einflugsloch, Legende weiterhin vorhanden

### ANSICHT_WIESE (wiese.c) — Phase 8 integriert
- Trachtquellen-Indikatoren aus `spiel->quellen[]` (Füllstand-Kreis, Typ-Label N/P/W)
- Outdoor-Sim-Bienen aus `spiel->bienen[]` (AUSSENFLUG/AM_SAMMELN/HEIMFLUG)
- 18 dekorative Wiesenbienen bleiben als Hintergrundleben
- HUD: Uhrzeit (hh:mm), Außentemp, Windstärke, Regen/Flug-gesperrt, Ressourcen-Übersicht
- 22 Blumen, 7 Bäume, Teich, Stock-Gebäude

### Simulation (sim/ Module)
- Phase 2: Pheromon-System (Alarm, Spur, Nasonov) — vollständig
- Phase 3: Haushalt-Formeln (Honig, Energie, Wachs) — vollständig
- Phase 4: Agent-Biene (Sense→Decide→Act→PayCost, 7 Bewegungszustände) — vollständig
- Phase 5: Sim-Stock (Brut, Thermoregulation, Nektar→Honig, Auftrags-Queue) — vollständig
- Phase 6: Sim-Welt (4 Trachtquellen, Tageszeit, Wetter, flug_gesperrt) — vollständig
- Phase 7: Flowfields (BFS 64×36 Grid, 5 Indoor-Zonen, pro Quelle Outdoor) — vollständig

### Core Loop (main.c)
```
bienenstock_init → wiese_init → pheromon_init → haushalt_selbsttest
→ welt_init → stock_init → pfad_stock_init → pfad_welt_init
Game Loop: welt_aktualisieren → pheromon_aktualisieren → alle_bienen_aktualisieren
           → stock_aktualisieren → [szene aktualisieren] → [szene zeichnen]
```

### Steuerung
- **TAB** wechselt zwischen ANSICHT_STOCK und ANSICHT_WIESE
- **P** Pheromon-Debug-Overlay ein/aus
- **LMB** Alarm-Pheromon setzen (Debug), **S** Spur-Pheromon (Debug)
- **ESC** beendet das Spiel

## NÄCHSTE SESSION
- Phase 9: Visuelle Überarbeitung — Realistische Farben
  - Zellenfarben aus Referenz-Farbpalette (beehive-architecture.md → Visuelles Design)
  - Bienen-Sprites: Dunkelbraun + dezente Gelb-Streifen
  - Stock-Gebäude auf Wiese: Oval/hängend statt rechteckig

---

## Simulator-Blueprint (Session 2 — 01.03.2026)

### 1. Bienenmodell: Zustandsmaschine pro Biene

#### Entwicklungsstadien (BienZustand)
```
EI → LARVE → PUPPE → ADULT
```

#### Rollen erwachsener Bienen (BienRolle)
| Deutsch (Code) | Englisch (Original) | Bedeutung |
|---|---|---|
| `PUTZERIN` | CLEANER | Zellen reinigen |
| `AMME` | NURSE | Larven füttern |
| `BAUERIN` | BUILDER | Waben bauen |
| `EMPFAENGERIN` | RECEIVER | Nektar annehmen & umtragen |
| `FAECHER` | FANNER | Ventilation, Wasser verdunsten |
| `WAECHTER` | GUARD | Wache am Einflugsloch |
| `BESTATERIN` | UNDERTAKER | Tote aus dem Stock entfernen |
| `SAMMLERIN_NEKTAR` | FORAGER_NECTAR | Nektar sammeln |
| `SAMMLERIN_POLLEN` | FORAGER_POLLEN | Pollen sammeln |
| `SAMMLERIN_WASSER` | FORAGER_WATER | Wasser holen |
| `SAMMLERIN_PROPOLIS` | FORAGER_PROPOLIS | Harz/Propolis sammeln |
| `ERKUNDERIN` | SCOUT | Neue Trachtquellen suchen (optional) |

#### Minimal-Stats pro Biene (BienStats)
```c
typedef struct {
    float alter_t;         // Alter in Tagen
    float energie_j;       // Energievorrat (Zucker als Treibstoff)
    float honigmagen_mg;   // Honigmagen-Füllung (Nektar in mg)
    float pollenladung_mg; // Pollenladung an Hinterbeinen
    float wasser_mg;       // Wasservorrat
    float gesundheit;      // Gesundheit (0.0 - 1.0)
    float krankheitslast;  // Parasitenbelastung (optional)
    BienRolle rolle;
    BienZustand zustand;
    float pos_x, pos_y;
    float geschw_x, geschw_y;
} BienStats;
```

---

### 2. Funktionskatalog (geplante Implementierung)

#### Bienen-Aktionen im Stock (innen)
| Funktion (Deutsch) | Funktion (Original) | Beschreibung |
|---|---|---|
| `zelle_reinigen(zelle_id)` | CleanCell | Leere Zelle säubern → belegbar |
| `ei_legen(zelle_id)` | LayEgg | Nur Königin; nur saubere Zellen |
| `larve_fuettern(larven_id)` | FeedLarva | Amme: Honig + Pollen verbrauchen |
| `thermoregulieren()` | Thermoregulate | Brutnesttemperatur stabil halten |
| `zelle_verdeckeln(zelle_id)` | CapCell | Puppe einschließen; kostet Wachs |
| `wachs_produzieren()` | ProduceWax | Viel Energie → Wachs |
| `wabe_bauen(bereich)` | BuildComb | Wachs → neue Zellenkapazität |
| `nektar_empfangen(von_biene)` | ReceiveNectar | Übergabe Sammler → Empfängerin |
| `nektar_verarbeiten()` | ProcessNectar | Enzymatisch + Entwässern → Honig |
| `luft_faecheln(zone)` | FanAir | Wasseranteil senken, Klima regulieren |
| `wasser_verteilen(zielzone)` | DistributeWater | Kühlung + Honigreifung |
| `luecken_abdichten()` | SealGaps | Propolis: Hygiene + Verteidigung |
| `eingang_bewachen()` | GuardEntrance | Freund/Feind-Erkennung |
| `alarm_ausloesen()` | ReleaseAlarmPheromone | Wächter alarmieren, Aggression steigern |
| `leiche_entfernen(id)` | RemoveCorpse | Seuchenrisiko senken |
| `sich_pflegen()` | GroomSelf | Parasitenlast reduzieren |
| `andere_pflegen(biene_id)` | GroomOther | Parasitenlast bei anderer reduzieren |

#### Bienen-Aktionen außen (Sammeln)
| Funktion (Deutsch) | Funktion (Original) | Beschreibung |
|---|---|---|
| `stock_verlassen()` | LeaveHive | Ausflug starten |
| `ziel_ansteuern(ziel)` | NavigateToTarget | Zur Trachtquelle fliegen |
| `ressource_sammeln(typ)` | Collect | Nektar/Pollen/Wasser/Propolis |
| `zum_stock_zurueckkehren()` | ReturnToHive | Heimflug |
| `ressource_abladen()` | Unload | An Empfängerin / in Zelle |
| `sammler_rekrutieren()` | Recruit | Schwanzeltanz / Duftspur |
| `trachtquelle_erkunden()` | ScoutForPatch | Neue Quelle suchen |
| `trachtquelle_bewerten(quelle)` | EvaluatePatch | Score: Distanz + Ertrag + Risiko |
| `schwanzeltanz(quelle)` | WaggleDance | Neue Sammler rekrutieren |

#### Stocksystem-Funktionen (Hive-Level)
| Funktion (Deutsch) | Funktion (Original) | Beschreibung |
|---|---|---|
| `stock_aktualisieren(dt)` | Hive_Update | Haupt-Update-Loop des Stocks |
| `stock_thermoregulieren(dt)` | Hive_ThermoControl | Brutbereich-Temperatur |
| `stock_nektar_verarbeiten(dt)` | Hive_ProcessNectar | Nektar → Honig Pipeline |
| `stock_wabe_bauen(dt)` | Hive_BuildComb | Wachs verbrauchen, Zellen erzeugen |
| `stock_brut_aktualisieren(dt)` | Hive_BroodUpdate | Ei→Larve→Puppe→Biene Zeitprogression |
| `stock_aufgaben_verteilen()` | Hive_AssignJobs | Bedarfsbasiert: viel Brut → mehr Ammen |

#### Weltsystem-Funktionen
| Funktion (Deutsch) | Funktion (Original) | Beschreibung |
|---|---|---|
| `welt_aktualisieren(dt)` | World_Update | Haupt-Update der Außenwelt |
| `welt_blumen_nachwachsen(dt)` | World_RegrowFlowers | Blüten regenerieren |
| `welt_pheromonfeld_aktualisieren(dt)` | World_PheromoneFieldUpdate | Diffusion + Zerfall |
| `wetter_aktualisieren(dt)` | Weather_Update | Flugwetter, Temperatur |

---

### 3. Physikalische Modelle & Formeln

#### 3.1 Nektar → Honig (Masse & Wasser)
```
zucker_g = nektar_g * zuckergehalt          // zuckergehalt z.B. 0.30
honig_g  = zucker_g / (1 - wasseranteil_honig)  // wasseranteil_honig z.B. 0.18
```

#### 3.2 Energiehaushalt pro Biene
```
energie_neu_j = energie_alt_j - (leistung_w * dt_s)   // Verbrauch
energie_neu_j = energie_alt_j + (zucker_g * energie_dichte_j_pro_g)  // Aufnahme
```
Leistungswerte: Flug > Heizen > Laufen (Weite Tracht kann unprofitabel werden!)

#### 3.3 Pheromonfeld (Diffusion + Zerfall)
```
C(x,y,t+dt) = C(x,y,t) + D * Laplace(C) * dt - k * C * dt + S(x,y) * dt
```
- `C` = Pheromonkonzentration
- `D` = Diffusionskoeffizient
- `k` = Zerfallskonstante
- `S` = Quelle (Biene emittiert)
- Gilt für: Alarmpheromon, Spurpheromon, Nasonov (Orientierung)

#### 3.4 Rekrutierung durch Schwanzeltanz
```
gewinn = zucker_pro_minute
kosten = flugzeit * energie_pro_sekunde + risiko
profit = gewinn - kosten
neue_sammler_pro_min = max_rekrutierung * (profit / (profit + skalierung_k))
```

#### 3.5 Brutentwicklung & Sterblichkeit
```
alter_stadium += dt
// Übergänge bei Schwellen: ei_dauer_t, larven_dauer_t, puppen_dauer_t
sterblichkeit_p = basis + stress(temperatur, nahrung, parasiten)
```
Emergente Dynamik: Futter knapp → weniger Brut → weniger Sammler → noch weniger Futter

#### 3.6 Wachsproduktion vs. Honig (Trade-off)
```
wachs_mg += wachs_ausbeute * zucker_verbraucht_g
neue_zellen += wachs_mg / wachs_pro_zelle_mg
```

---

### 4. Zu trackende Ressourcen (Spielzustand, zukünftig)
- `honig_g` — Vorrat fertiger Honig
- `nektar_g` — unreifer Nektar (wird verarbeitet)
- `pollen_g` — Pollenvorrat
- `wasser_g` — Wasservorrat
- `wachs_mg` — Wachsvorrat
- `brut_anzahl[EI|LARVE|PUPPE]` — pro Stadium
- `bienen_anzahl[ROLLE]` — pro Rolle
- `temperatur_c` — Brutbereich-Temperatur
- `bedrohungslevel` — Alarmpheromon am Eingang
- `trachtquellen[]` — Ertrag, Regeneration, Distanz, Tageszeit-Funktion
- `wetter` — Flugmöglich ja/nein, beeinflusst Sammelrate & Energie

---

---

## Geplante C-Modul-Struktur (Zielzustand)

| Datei (Deutsch) | Original | Verantwortung |
|---|---|---|
| `sim_welt.c/.h` | sim_world | `welt_aktualisieren()`, `trachtquelle_aktualisieren()`, `wetter_aktualisieren()` |
| `sim_stock.c/.h` | sim_hive | `stock_aktualisieren()`, `stock_auftraege_erstellen()`, `stock_nektar_verarbeiten()` |
| `agent_biene.c/.h` | agent_bee | `biene_aktualisieren()`, `biene_entscheiden()`, `biene_handeln()`, `biene_ansicht_wechseln()` |
| `pfad_stock.c/.h` | path_hive | `stroemungsfeld_stock_aktualisieren()` — Flowfield pro Innenzone |
| `pfad_welt.c/.h` | path_world | `stroemungsfeld_welt_aktualisieren()` — Flowfield pro Trachtquelle |
| `pheromon.c/.h` | pheromone | `pheromonfeld_aktualisieren()`, `pheromon_abgeben()` |
| `haushalt.c/.h` | economy | Honig/Nektar-Formeln, Energieberechnungen |
| `speicher.c/.h` | save | Spielstand serialisieren (binär oder JSON) |

---

## Welt-Architektur: Zwei Karten, ein Simulator

### Innenwelt — ANSICHT_STOCK (Querschnitt)
- Grid/Waben: Jede Wabenzelle ist ein Tile im Querschnitt
- Bienen laufen NICHT in Zellen, sondern **an Zellrändern entlang** (Gangwege)
- Pathfinding innen: **Flowfield pro Zone** (nicht A* — zu viele Bienen!)
  - Zonen: Brutbereich, Honiglager, Pollenbereich, Königinbereich, Eingang
  - Jede Biene schaut: "Welche Zone brauche ich?" → folgt dem Flowfield dieser Zone
  - Normale Gang-Kosten: 1.0
  - Bei Gedränge (viele Bienen): + `gedraenge_strafe`

#### Zellentypen (ZellenTyp-Enum, geplant)
| Deutsch (Code) | Original | Bedeutung |
|---|---|---|
| `ZELLE_LEER` | EMPTY | Unbesetzt, sauber |
| `ZELLE_BRUT_EI` | BROOD_EGG | Ei drin |
| `ZELLE_BRUT_LARVE` | BROOD_LARVA | Larve drin |
| `ZELLE_BRUT_PUPPE` | BROOD_PUPA | Verdeckelte Puppe |
| `ZELLE_HONIG_OFFEN` | HONEY_UNCAPPED | Nektar/unreifer Honig |
| `ZELLE_HONIG_VERD` | HONEY_CAPPED | Reifer Honig, verdeckelt |
| `ZELLE_POLLEN` | POLLEN | Pollenspeicher |
| `ZELLE_WACHSBAU` | WAX_BUILDING | Im Bau |
| `ZELLE_KOENIGIN` | QUEEN_AREA | Bereich der Königin |
| `ZELLE_BLOCKIERT` | PATH_BLOCKED | Nicht betretbar |

### Außenwelt — ANSICHT_WIESE (Top-Down)
- Kein Zell-Grid wie innen — stattdessen: **Patch/POI + lokale Kollision**
- Pathfinding außen: Flowfield/Potentialfeld je Patch + lokale Kollisionsvermeidung (Option A, empfohlen) oder A* auf Tilemap
- Bienen reagieren auf: `gefahr_meiden()` (Spinne/Wespe/Pestizid), `flugwetter_pruefen()` (Regen/Wind stoppt Sammeln)

#### Außenwelt-Objekte (POIs)
- Trachtquellen: Blumen/Obstbäume/Gewächse-Patches
- Wasserstellen
- Harz/Propolis-Quellen
- Gefahrenzonen (Spinnen, Wespen, Pestizide, Regen)

### Verbindung (Portal/Eingang)
- `EINGANG_KACHEL` innen ↔ `STOCK_EINGANG_POS` außen
- Übergang = State-Switch (kein Teleport nötig, kann optional animiert werden)

---

## Core Loop (Tick-Struktur)

Pro Frame / Sim-Tick (`dt`):
```
1. welt_aktualisieren(dt)       // Außenwelt: Patches, Wetter, Pheromonfeld außen
2. stock_aktualisieren(dt)      // Innenwelt: Brut, Klima, Nektar→Honig, Aufträge
3. agenten_aktualisieren(dt)    // Jede Biene: Wahrnehmen → Entscheiden → Handeln → Kosten
4. transfer_aktualisieren(dt)   // Ressourcen am Portal → landen in Stocklagern/Zellen
```
**Wichtig:** Rendering/View vollständig von Simulation getrennt. View zeigt nur Zustände, verändert nichts.

---

## Bienen-KI: Zustandsautomat + Auftragssystem

### Bewegungszustände (BienBewegung)
| Deutsch (Code) | Original | Bedeutung |
|---|---|---|
| `IM_STOCK_ARBEITEND` | IN_HIVE_WORKING | Führt Innenauftrag aus |
| `ZUM_AUSGANG` | GO_TO_EXIT | Läuft zum Einflugsloch |
| `AUSSENFLUG` | OUTSIDE_TRAVEL | Fliegt zur Trachtquelle |
| `AM_SAMMELN` | FORAGE | Sammelt aktiv |
| `HEIMFLUG` | RETURN_TRAVEL | Fliegt zurück zum Stock |
| `ZUM_ABLADEN` | GO_TO_DROP | Sucht Abgabestelle im Stock |
| `RASTEN` | REST | Pause, Energie regenerieren |

### Auftragstypen (AuftragsTyp)
**Innenaufträge:**
| Deutsch (Code) | Original |
|---|---|
| `AUFTRAG_ZELLE_REINIGEN` | JOB_CLEAN_CELL |
| `AUFTRAG_LARVE_FUETTERN` | JOB_FEED_LARVA |
| `AUFTRAG_WABE_BAUEN` | JOB_BUILD_WAX |
| `AUFTRAG_NEKTAR_VERARBEITEN` | JOB_PROCESS_NECTAR |
| `AUFTRAG_FAECHELN` | JOB_FAN |
| `AUFTRAG_EINGANG_BEWACHEN` | JOB_GUARD |
| `AUFTRAG_LEICHE_ENTFERNEN` | JOB_REMOVE_CORPSE |

**Außenaufträge:**
| Deutsch (Code) | Original |
|---|---|
| `AUFTRAG_NEKTAR_SAMMELN` | JOB_FORAGE_NECTAR |
| `AUFTRAG_POLLEN_SAMMELN` | JOB_FORAGE_POLLEN |
| `AUFTRAG_WASSER_HOLEN` | JOB_FETCH_WATER |
| `AUFTRAG_PROPOLIS_HOLEN` | JOB_FETCH_PROPOLIS |
| `AUFTRAG_ERKUNDEN` | JOB_SCOUT |

### Entscheidungslogik (Prioritäten)
Der Stock erzeugt "Bedarf" → daraus werden Aufträge → Bienen ziehen Aufträge nach Priorität + Eignung:

1. **Brut am Leben erhalten** — Füttern, Wärmen (höchste Priorität)
2. **Klima** — Fächeln, Wasser bei Hitze
3. **Nektar-Prozess** — sonst kippt Honigreifung
4. **Wabenbau** — wenn Platz knapp
5. **Sammeln** — wenn genug Innenkräfte vorhanden
6. **Wache / Abfall** — abhängig von Bedrohungslevel / Seuchenwert

---

## Patch-Ökonomie (Außenwelt)

```
// Trachtquellen-Regeneration (realistisch, endlich):
nektar_g(t+dt) = min(kapazitaet, nektar_g(t) + regeneration_g_pro_s * dt - entnommen_g)
// Je Patch: Tageszeitkurve (morgens/mittags/abends unterschiedlich)
```

---

## Zu trackende Ressourcen (Spielzustand, zukünftig erweitert)

### Stockwerte (global)
- `honig_reif_g` — fertiger Honig
- `nektar_unreif_g` — wird noch verarbeitet
- `pollen_g`, `wasser_g`, `wachs_mg`
- `brut_anzahl[EI|LARVE|PUPPE]`
- `bienen_anzahl[ROLLE]`
- `temperatur_c` — Brutbereich
- `bedrohungslevel` — Alarmpheromon am Eingang
- `auftragsschlange[]` — offene Aufträge

### Außenweltwerte
- Je Trachtquelle: `nektar_g`, `pollen_g`, `regenerationsrate`, `distanz`, `risiko`
- Wetter: `regen`, `wind`, `temperatur_c`

---

## Milestone-Plan (aktualisiert)

| # | Status | Beschreibung |
|---|--------|--------------|
| 1 | ✅ | Visuelles Grundgerüst (Waben, zwei Ansichten) |
| 2 | ⬜ | Pheromon-System (Feld, Diffusion, Zerfall, Typen) |
| 3 | ⬜ | Bienenmodell + Task-System (Rollen, Zustandsmaschine, Stats) |
| 4 | ⬜ | Lebenszyklus (Ei → Larve → Puppe → Biene, Königin legt Eier) |
| 5 | ⬜ | Ressourcensystem (Honig, Nektar, Pollen, Wachs, Wasser) |
| 6 | ⬜ | Energiehaushalt (Bienen "bezahlen" Aktionen mit Energie) |
| 7 | ⬜ | Schwanzeltanz + Rekrutierung |
| 8 | ⬜ | Jahreszeiten (Winter-Überlebens-Mechanik) |
| 9 | ⬜ | Feinde (Wespen, Hornissen) |

---

## Geplanter Backend-Stack (FINAL entschieden, noch nicht gebaut)
- **Auth**: Firebase Auth (Google + Apple Login)
- **DB**: Firestore (Spielstände, Bestenlisten, Hive-Snapshots)
- **Desktop → Backend**: libcurl (HTTP aus C)
- **Web**: WebAssembly via Emscripten (`make web`)
- **Frontend**: React + Firebase SDK

→ Backend erst nach Milestone 4-5 angehen

---

---

## Visuelles Design — Farbpalette (Referenzbilder)

### Außenansicht (ANSICHT_WIESE) — Referenz: hive.jpeg (Downloads)
Natürlicher Wildschwarm, hängend am Ast. Orientierung für Stock-Gebäude auf der Wiese:

| Element | Farbe (ungefähr) | Hex-Richtwert |
|---|---|---|
| Stock-Außenhülle (dicht) | Fast schwarz, dunkelbraun | `#1A0F0A` – `#2E1A0E` |
| Stock-Mitte (Tiefe) | Warmes Dunkelbraun | `#3D2010` – `#5C3318` |
| Stock-Highlights (Amber) | Honigbraun, warm | `#8B5E2A` – `#C4852A` |
| Bienen auf Oberfläche | Dunkelbraun + Gelb-Akzente | `#2A1A08` + `#D4A820` |
| Ast/Holz | Dunkelgrau-braun | `#4A3828` |
| Hintergrund Wiese | Kräftiges Grün | `#2D5A1B` – `#4A8C2A` |
| Rote Akzente (Blüten) | Kräftiges Rot | `#C42020` – `#E03030` |

**Form:** Oval/tropfenförmig hängend — kein rechteckiger Kasten!
**Wichtig:** Natürlich wirken — kein Plastik-Look, warme organische Töne.

### Innenansicht (ANSICHT_STOCK) — Referenz: colors.jpeg (Downloads)
Echte Honigwaben mit offenem und verdeckeltem Honig. Warme, organische Goldtöne.

| Element | Farbe | Hex-Richtwert |
|---|---|---|
| Zellwände (Wachs) | Warmes Goldgelb | `#C17A00` – `#D4890A` |
| Offener Honig (`ZELLE_HONIG_OFFEN`) | Tief-Amber, glänzend | `#B85C00` – `#D4700A` |
| Verdeckelter Honig (`ZELLE_HONIG_VERD`) | Mattes Cremegelb | `#E8C84A` – `#F0D060` |
| Neues Wachs (`ZELLE_WACHSBAU`) | Hell, cremeweiß-gelb | `#EDD870` – `#F5E8A0` |
| Zellkanten / Propolis | Tiefbraun bis schwarz | `#2A1A08` – `#5C3208` |
| Pollen (`ZELLE_POLLEN`) | Kräftiges Orange-Gelb | `#E8820A` – `#F5A020` |
| Brut Larve (`ZELLE_BRUT_LARVE`) | Cremeweiß, leicht perlmut | `#F0ECD0` – `#E8E0B0` |
| Brut Puppe (`ZELLE_BRUT_PUPPE`) | Goldbeige, verdeckelt | `#C8A050` – `#D4B060` |
| Leere Zelle (`ZELLE_LEER`) | Dunkles Amber, tief | `#7A4A10` – `#8B5520` |

### ⚠️ TODO — Visuelle Überarbeitung (nach Blueprint-Implementierung)
- [ ] Stock-Gebäude auf der Wiese: Oval/hängend statt rechteckig, Farbpalette aus hive.jpeg
- [ ] Bienen-Sprites: Dunkelbraun + dezente Gelb-Streifen (nicht Comic-gelb!)
- [ ] Waben-Farben: Referenz aus Innenleben-Foto (kommt noch)
- [ ] Hintergrund-Grün: Kräftiger, wärmer als aktuell
- [ ] Gesamtstimmung: Warm, organisch, naturalistisch

---

