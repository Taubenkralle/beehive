# Brutpflege & Thermoregulation

Das Brutnest exakt auf **35 °C** zu halten ist eine der wichtigsten Aufgaben des Volkes. Zu kalt oder zu heiß, und Larven sowie Puppen sterben.

---

## Zieltemperatur

```
Optimales Brutnest:     35 °C
Gefährlich kalt unter:  32 °C
Gefährlich heiß über:   36 °C  (Fächeln setzt ein)
Letale Extreme:         < 15 °C oder > 45 °C (in der Simulation begrenzt)
```

---

## Thermoregulations-Schleife

Jeden Tick berechnet `stock_thermoregulieren()` den Temperatur-Delta:

```c
float bienen_waerme   = volksgroesse × 0,003;         // Stoffwechselwärme aller Bienen (°C/s)
float ambient_verlust = (temperatur_c − 20,0) × 0,004; // Passives Abkühlen Richtung 20 °C außen
float faecheln_kuehl  = anzahl_faecher × 0,05;         // Aktive Kühlung durch Fächelbienen

delta = (bienen_waerme − ambient_verlust − faecheln_kuehl) × dt;
temperatur_c += delta;
```

### Heizen

Alle Bienen erzeugen Stoffwechselwärme durch Vibration der Flugmuskulatur. Ein größeres Volk heizt den Stock stärker. Im Winter (noch nicht implementiert) ballen sich Bienen eng zusammen und vibrieren kontinuierlich, um zu überleben.

### Kühlen

Wenn `temperatur_c > 36 °C`, generiert die Auftragswarteschlange `AUFTRAG_FAECHELN`. Fächelbienen postieren sich am Eingang und schlagen ihre Flügel, um kühle Luft durch den Stock zu ziehen.

Anzahl der generierten Fächelaufträge:
```c
faecher_bedarf = (int)((temperatur_c − 35,0) × 4,0) + 1
```

### Fächeln trocknet auch Nektar

Der gleiche Luftstrom, der den Stock kühlt, verdunstet auch Wasser aus unfertigem Nektar — zwei Aufgaben auf einmal. Sowohl `FAECHER`- als auch `EMPFAENGERIN`-Bienen zählen zur Verdunstungsrate.

---

## Brut-Sterblichkeit

`brut_sterblichkeit(temperatur_c, nahrungs_mangel)` gibt eine Sterbewahrscheinlichkeit pro Tick zurück.

| Bedingung | Auswirkung |
|-----------|-----------|
| Temperatur 32–36 °C | Minimale Sterblichkeit |
| Temperatur außerhalb 32–36 °C | Steigende Sterblichkeit (quadratisch) |
| Pollen < 5 g (Amme kann nicht füttern) | `nahrungs_mangel = 0,8` — hohe Larvensterben |
| Kombinierter Stress | Additiv |

Die Sterblichkeit wird probabilistisch per deterministischem Hash aus `alter_t` angewendet (kein `rand()`), sodass die Ergebnisse für einen gegebenen Volkszustand konsistent und reproduzierbar sind.

```c
if (p_tod > 0.0f && (z->alter_t * 137.0f − (int)(z->alter_t * 137.0f)) < p_tod * tage_dt) {
    z->typ    = ZELLE_LEER;
    z->sauber = false;
}
```

---

## Ammenbienen-Fütterung

Die `AMME`-Rolle übernimmt das Larvenfüttern. Jede Amme mit `AUFTRAG_LARVE_FUETTERN`:
1. Navigiert zur Brutzone (Flowfield: `STOCK_ZONE_BRUT`)
2. Verbringt Zeit am Zielort
3. Verbraucht Honig + Pollen aus dem Volkvorrat
4. Markiert den Auftrag als erledigt → Slot wird freigegeben

Das Volk schreibt immer **einen Fütterungsauftrag pro Larve** in die Warteschlange:
```c
int bedarf  = spiel->brut_larve;
int bereits = offene_auftraege(spiel, AUFTRAG_LARVE_FUETTERN);
int anzahl  = bedarf − bereits;
```

---

## Wabenzellen & Sauberkeit

Nachdem eine Puppe geschlüpft ist (oder eine Larve gestorben ist), wird die Zelle zu `ZELLE_LEER` mit `sauber = false`. Eine schmutzige Zelle:
- Kann von der Königin nicht zur Eiablage genutzt werden
- Generiert `AUFTRAG_ZELLE_REINIGEN` in der Warteschlange
- Eine `PUTZERIN` übernimmt den Auftrag, reinigt die Zelle → `sauber = true`

Das schafft einen natürlichen Engpass: Sind zu viele Zellen schmutzig und nicht genug Putzerinnen verfügbar, verlangsamt sich die Eiablage — selbst wenn rechnerisch Platz vorhanden wäre.

---

## Rückkopplungsschleife des Volkes

```
Mehr Brut → mehr Ammenbienen nötig → mehr Pollenverbrauch
     ↓                                        ↓
Mehr Sammlerinnen nötig             Pollen wird knapp → Larven sterben
     ↓                                        ↓
Weniger Brutabdeckung → weniger Brut → Volk schrumpft
```

Diese emergente Rückkopplungsschleife ist die Kern-Überlebensmechanik. Ein gesundes Volk reguliert sich selbst; ein gestresstes Volk kann in eine Abwärtsspirale geraten.
