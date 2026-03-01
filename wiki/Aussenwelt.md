# Außenwelt & Wetter

Die Wiesen-Ansicht simuliert die Umgebung rund um den Stock — Trachtquellen, Tageszeit, Temperatur, Wind und Regen.

---

## Tageszeit

```c
#define SIM_TAG_DAUER_S  120.0f   // Ein simulierter Tag = 120 Echtzeit-Sekunden
```

`tageszeit` läuft von `0,0` (Mitternacht) bis `1,0` (nächste Mitternacht):
- `0,25` = 06:00 (Morgendämmerung)
- `0,50` = 12:00 (Mittag)
- `0,75` = 18:00 (Abenddämmerung)

Das HUD in der Wiesen-Ansicht zeigt die aktuelle Uhrzeit als `hh:mm`.

### Auswirkung auf das Sammeln

Bienen können außerhalb des Zeitfensters `[0,10 – 0,90]` (ca. 02:24–21:36 Uhr) nicht fliegen. Außerhalb dieser Zeiten ist `flug_gesperrt = true`.

Der Nektar-Ertrag der Trachtquellen folgt einer Sinuskurve mit Höchstwert zum Mittag:
```c
tageszeit_faktor = sinf(tageszeit × π)   // 0 bei Morgen-/Abenddämmerung, 1 zur Mittagszeit
```

---

## Wetter

`wetter_aktualisieren()` läuft jeden Tick und aktualisiert:

### Wind

Die Windstärke (`wind_staerke`, 0,0–1,0) verwendet überlagerte Sinuswellen für natürliche Variation:

```c
wind_staerke = 0,5 + 0,4 × sin(zeit × 0,017) + 0,1 × sin(zeit × 0,053)
```

Kein Zufallsgenerator beteiligt — deterministisch, wirkt aber organisch. Wind über **0,8** blockiert alle Ausflüge.

### Regen

Regen verwendet eine einfache Zustandsmaschine mit `rand()`:

```
PRÜFUNG: alle WETTER_INTERVALL_S = 20 Sekunden
  → 20 % Chance: Regen beginnt  (Dauer = 15–45 Sekunden)
  → Während des Regens: 10 % Chance je Prüfung: Regen endet frühzeitig
```

Regen setzt sofort `flug_gesperrt = true`. Sammlerinnen, die gerade draußen sind, beenden ihren aktuellen Auftrag und nehmen danach keine neuen Außenaufträge mehr an.

### Temperatur

Die Außentemperatur folgt einem Tag-Nacht-Rhythmus:

```c
#define AUSSENTEMP_BASIS_C       18,0f   // Durchschnittstemperatur
#define AUSSENTEMP_AMPLITUDE      8,0f   // Schwankungsbreite

aussentemp_c = AUSSENTEMP_BASIS_C + AUSSENTEMP_AMPLITUDE × sin(tageszeit × π)
```

Spannweite: 10 °C nachts → 26 °C mittags. Unter **10 °C** ist Fliegen nicht möglich (`FLUG_TEMP_MIN_C`).

---

## Flugsperre (`flug_gesperrt`)

`flug_moeglich()` gibt `false` zurück (und setzt `flug_gesperrt = true`), wenn **eine** dieser Bedingungen zutrifft:

| Bedingung | Schwellenwert |
|-----------|--------------|
| Regen | `regen == true` |
| Kälte | `aussentemp_c < 10 °C` |
| Sturm | `wind_staerke > 0,8` |
| Nacht | `tageszeit < 0,10 || tageszeit > 0,90` |

Bei Flugsperre überspringt `agent_biene.c` alle Außenauftragsvergaben.

---

## Trachtquellen (`Trachtquelle`)

Vier Quellen werden beim Start von `welt_init()` platziert:

| Typ | Position (Weltkoordinaten) | Kapazität |
|-----|---------------------------|----------|
| Nektar (`TRACHT_NEKTAR`) | (750, 220) | 400 g |
| Pollen (`TRACHT_POLLEN`) | (950, 380) | 200 g |
| Wasser (`TRACHT_WASSER`) | (680, 560) | 1000 g |
| Nektar 2 (`TRACHT_NEKTAR`) | (1050, 180) | 250 g |

### Quell-Datenstruktur

```c
typedef struct {
    TrachtTyp typ;
    float pos_x, pos_y;           // Weltposition
    float vorrat_g;               // Aktueller Vorrat (Gramm)
    float kapazitaet_g;           // Maximalkapazität
    float regeneration_g_s;       // Nachfüllrate pro Sekunde
    float risiko;                 // Gefahrenlevel (0,0 = sicher, 1,0 = tödlich)
    bool  aktiv;                  // Für Bienen zugänglich
} Trachtquelle;
```

### Regeneration

Jede Quelle füllt sich passiv jeden Tick auf, modifiziert durch Bedingungen:

```
ertrag_faktor = tageszeit_faktor × temp_faktor × wind_faktor × regen_faktor
vorrat_g += regeneration_g_s × ertrag_faktor × dt
vorrat_g = min(vorrat_g, kapazitaet_g)
```

- `regen_faktor = 0` bei Regen (Blüten geschlossen)
- `wind_faktor = 1 − wind_staerke × 0,5` (Wind reduziert den Nektarfluss)
- `temp_faktor` steigt mit der Wärme

---

## Trachtquellen-Anzeige in der Wiesen-Ansicht

Trachtquellen werden als Kreise dargestellt:
- **Äußerer Ring** — Quellumriss (Farbe nach Typ: Orange = Nektar, Gelb = Pollen, Blau = Wasser)
- **Gefüllte Scheibe** — aktueller Füllstand (proportional zu `vorrat_g / kapazitaet_g`)
- **Buchstaben-Label** — N (Nektar), P (Pollen), W (Wasser)

Wenn eine Quelle erschöpft ist, schrumpft die Scheibe auf nichts zusammen, während der Ring sichtbar bleibt.

---

## Stockeingang (Portal)

Der Stockeingang ist das **Portal** zwischen beiden Ansichten:

- **Innenposition**: `STOCK_EINGANG_X = 130, STOCK_EINGANG_Y = 370` (links im Hive-Grid)
- **Außenposition**: gleiche Koordinaten — das Stockgebäude befindet sich links in der Wiesen-Ansicht

Übergang: `ZUM_AUSGANG` → Eingang überschreiten → `AUSSENFLUG`. Bei Rückkehr: `HEIMFLUG` → Ankunft → `ZUM_ABLADEN`.
