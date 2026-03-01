# Navigation & Pathfinding

Mit potenziell tausenden gleichzeitig bewegten Bienen wäre klassisches Pathfinding (A* pro Biene) viel zu rechenintensiv. Stattdessen nutzt die Simulation **Flowfields** — eine Technik, bei der das Pathfinding einmal pro Ziel berechnet wird und beliebig viele Bienen ihre Richtung dann in O(1) ablesen können.

---

## Das Grundprinzip

Ein Flowfield ist ein 2D-Gitter, in dem jede Zelle einen Richtungsvektor speichert, der zum Ziel zeigt. Anstatt dass jede Biene selbst einen Weg sucht, schaut jede Biene an Position `(x, y)` einfach die vorberechnete Richtung nach:

```
Biene möchte zur Honiglagerzone
→ Flowfield STOCK_ZONE_HONIG an Gitterzelle (biene.pos_x/20, biene.pos_y/20) nachschlagen
→ Richtungsvektor (dx, dy) erhalten
→ In diese Richtung bewegen
```

10.000 Bienen = 10.000 Gitter-Lookups. Keine Suche, keine Prioritätswarteschlangen.

---

## Gitter-Spezifikation

```c
#define PFAD_ZELLEN_GROESSE   20    // Pixel pro Zelle
#define PFAD_FF_BREITE        64    // 1280 / 20
#define PFAD_FF_HOEHE         36    // 720 / 20
#define PFAD_FF_ZELLEN       2304   // 64 × 36
```

Jede Zelle ist ein `Richtung`-Struct (Richtung = direction):
```c
typedef struct { int8_t dx, dy; } Richtung;   // Werte: -1, 0 oder +1
```

Diagonale Richtungen werden beim Umrechnen in Weltbewegung durch √2 ≈ 0,707 normiert.

---

## BFS-Berechnung

Das Flowfield für jedes Ziel wird per **Breitensuche (BFS) vom Ziel aus** berechnet (Rückwärts-BFS):

```
1. Zielzelle als Kosten 0 markieren, in Warteschlange legen
2. Für jede herausgeholte Zelle:
   a. Für jeden der 8 Nachbarn (inkl. Diagonal):
      - Falls nicht besucht: Kosten = Elternkosten + 1, in Warteschlange legen
      - Richtung setzen: Nachbar → Elternteil (zum Ziel hin)
3. Alle Zellen haben jetzt eine Richtung, die zum Ziel zeigt
```

Dies wird einmalig beim Start berechnet. Hindernisse können später (Phase 9: Wände, Wachsstrukturen) als nicht passierbare Zellen in die BFS eingetragen werden.

---

## Indoor-Flowfields (5 Zonen)

`pfad_stock_init()` berechnet ein Flowfield pro Zone:

| Zone | Code | Zielposition | Genutzt von |
|------|------|-------------|-------------|
| Eingang | `STOCK_ZONE_EINGANG` | (180, 370) | Bienen, die nach draußen wollen (`ZUM_AUSGANG`) |
| Lager | `STOCK_ZONE_LAGER` | (640, 360) | Bienen beim Abladen (`ZUM_ABLADEN`) |
| Brut | `STOCK_ZONE_BRUT` | (400, 360) | Ammenbienen, Putzerinnen |
| Honig | `STOCK_ZONE_HONIG` | (680, 250) | Verarbeitung, Wabenbau |
| Pollen | `STOCK_ZONE_POLLEN` | (680, 470) | Pollenlagerpflege |

```c
Vector2 pfad_stock_richtung(float pos_x, float pos_y, StockZone zone);
```

---

## Outdoor-Flowfields (pro Quelle + Heimweg)

`pfad_welt_init()` berechnet ein Flowfield pro aktiver Trachtquelle sowie ein Feld, das zum Stockeingang zeigt:

```c
Vector2 pfad_welt_richtung(float pos_x, float pos_y, int quellen_id);  // → Trachtquelle
Vector2 pfad_eingang_richtung(float pos_x, float pos_y);               // → Stockeingang
```

Sammlerinnen nutzen `quellen_id_fuer_biene()`, um nachzuschauen, welche Quelle ihr aktueller Auftrag ansteuert.

Flowfields werden in statischen Arrays gecacht und als gültig/ungültig markiert. Bewegt sich eine Quelle oder kommt eine neue hinzu, berechnet `pfad_welt_berechnen(id)` nur dieses eine Feld neu.

---

## Wie `biene_bewegen()` Flowfields nutzt

```c
void biene_bewegen(Biene* b, const Spielzustand* spiel, float dt);
```

Zustand → Flowfield-Zuordnung:

| Bienen-Zustand | Genutztes Flowfield |
|----------------|---------------------|
| `ZUM_AUSGANG` | `pfad_stock_richtung(STOCK_ZONE_EINGANG)` |
| `ZUM_ABLADEN` | `pfad_stock_richtung(STOCK_ZONE_LAGER)` |
| `IM_STOCK_ARBEITEND` | `pfad_stock_richtung(zone_fuer_rolle(rolle))` |
| `AUSSENFLUG` | `pfad_welt_richtung(quellen_id)` |
| `HEIMFLUG` | `pfad_eingang_richtung()` |
| `RASTEN`, `AM_SAMMELN` | Keine Bewegung |

**Fallback:** Gibt das Flowfield einen Nullvektor zurück (Zielzelle oder nicht initialisiert), fällt die Biene auf **direktes Steuern** zurück — die Richtung zu `(ziel_x, ziel_y)` wird manuell berechnet.

**Überschießschutz:** Ist die Biene näher am Ziel als `geschwindigkeit × dt` Pixel, springt sie direkt zum Ziel statt darüber hinauszuschießen.

---

## Performance-Vergleich

| Ansatz | Kosten pro Biene pro Frame |
|--------|---------------------------|
| A* pro Biene | O(N log N) Gitterzellen durchsucht |
| Flowfield-Lookup | O(1) — ein einziger Array-Zugriff |

Bei 60 FPS mit 1.000 Bienen: **60.000 O(1)-Lookups** statt 60.000 Suchläufe. Der Unterschied wird bei 10.000+ Bienen dramatisch — was das Designziel ist.

---

## Ausblick: Hinderniskarten

Derzeit hat der BFS keine Hindernisse — jede Zelle ist passierbar. Die Flowfield-Infrastruktur ist bereit, ab Phase 9 folgendes hinzuzufügen:

- Wachswaben-Wände als nicht passierbare Zellen
- Gedränge-Strafen (Zellenkosten steigen, wenn viele Bienen in der Nähe sind)
- Gefahrenzonen (Abstoßungsfelder nahe Bedrohungen)

Hindernisse hinzufügen bedeutet schlicht: Zellen vor dem BFS als `Kosten = UINT16_MAX` markieren.
