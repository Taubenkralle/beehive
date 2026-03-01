# Pheromon-System

Bienen kommunizieren fast ausschließlich über Chemikalien. Die Simulation modelliert drei Pheromontypen als unabhängige 2D-Konzentrationsfelder, die das gesamte Fenster abdecken.

---

## Pheromontypen

| Code | Typ | Biologische Funktion |
|------|-----|---------------------|
| `PHEROMON_ALARM` | Alarmpheromon | Wird bei Bedrohung ausgeschüttet; rekrutiert Wächterinnen, löst Verteidigungsverhalten aus |
| `PHEROMON_SPUR` | Spurpheromon | Markiert den Weg von der Beute zur guten Trachtquelle; rekrutiert weitere Sammlerinnen |
| `PHEROMON_NASONOV` | Nasonov-Pheromon | Orientierungssignal — „hier ist der Stockeingang"; hilft heimkehrenden Bienen, das Flugloch zu finden |

---

## Das Gitter

Jeder Pheromontyp hat sein eigenes `PheromonFeld`-Struct:

```c
#define PHEROMON_BREITE 128   // Zellen horizontal
#define PHEROMON_HOEHE   72   // Zellen vertikal
// Gesamt: 9216 float-Werte pro Feld, deckt das volle 1280×720-Fenster ab
// Jede Zelle = 10×10 Pixel

typedef struct {
    float zellen[PHEROMON_BREITE * PHEROMON_HOEHE];
    float diffusion;   // D — wie schnell es sich ausbreitet
    float zerfall;     // k — wie schnell es verblasst
} PheromonFeld;
```

---

## Physik

Jeden Tick wendet `pheromon_aktualisieren()` die Diffusions-Zerfall-Gleichung auf alle drei Felder an:

```
C(x,y,t+dt) = C(x,y,t)
             + D × Laplace(C) × dt    ← Diffusion (breitet sich aus)
             − k × C × dt            ← Zerfall (verblasst)
             + S(x,y) × dt           ← Quellterm (Biene emittiert)
```

`Laplace(C)` ist die diskrete 4-Nachbar-Näherung:
```
Laplace = C[x-1,y] + C[x+1,y] + C[x,y-1] + C[x,y+1] − 4×C[x,y]
```

### Abstimmungsparameter

| Pheromon | Diffusion (D) | Zerfall (k) | Charakter |
|----------|--------------|------------|-----------|
| Alarm | Schnell | Schnell | Dringendes, kurzlebiges Signal |
| Spur | Mittel | Langsam | Persistente Routenmarkierung |
| Nasonov | Langsam | Mittel | Gleichmäßiges Leuchtfeuer am Eingang |

---

## Emission

```c
void pheromon_abgeben(PheromonFeld* feld, float x, float y, float menge);
```

Addiert `menge` zur Gitterzelle an der Weltposition `(x, y)`. Bienen emittieren Pheromone kontextabhängig:

- **Heimkehrende Sammlerin mit Nektar** → gibt Spurpheromon entlang der Route ab
- **Wächterin bei Bedrohung** → gibt Alarmpheromon am Eingang ab
- **Biene am Eingang** → gibt Nasonov-Pheromon ab (hilft Schwarm bei der Orientierung)

---

## Feld lesen

```c
float pheromon_lesen(const PheromonFeld* feld, float x, float y);
```

Verwendet **bilineare Interpolation** zwischen den vier umgebenden Gitterzellen, sodass Bienen einen glatten Gradientenwert erhalten statt einen abgestuften. Das ist wichtig für gradientenbasiertes Navigationsverhalten.

---

## Debug-Overlay

**P** drücken im Spiel, um die Pheromon-Heatmap einzublenden:

- **Rot/Orange** — Alarmpheromon
- **Grün/Gelb** — Spurpheromon
- **Blau** — Nasonov-Pheromon

Das Overlay wird über der Szene, aber unter dem HUD gerendert.

Debug-Steuerung:
- **LMB** — Alarmpheromon manuell an Mausposition setzen
- **S (gehalten)** — Spurpheromon manuell an Mausposition setzen

---

## Biologische Hintergründe

Echte Honigbienen kennen über 15 verschiedene Pheromontypen. Die Simulation modelliert aktuell die drei wichtigsten für das Volksverhalten:

- **Alarm (Isoamylacetat):** Aus der Stachteldrüse freigesetzt. Löst massiven Verteidigungsreflex aus. Sehr flüchtig — verflüchtigt sich in Minuten.
- **Spur-/Fußabdruck-Pheromon:** Wird auf Oberflächen abgesetzt. Markiert gut genutzte Wege.
- **Nasonov (Geraniol + andere Terpenoide):** Fächelnde Biene öffnet die Nasonov-Drüse nahe der Hinterleibsspitze und verbreitet den Duft nach oben. Entscheidend beim Schwärmen für die Orientierung.
