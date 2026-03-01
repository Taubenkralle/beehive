# Bienen-Lebenszyklus

Jede Biene in der Simulation durchläuft vier Entwicklungsstadien, bevor sie zur Arbeiterin wird.

---

## Entwicklungsstadien

```
EI  →  LARVE  →  PUPPE  →  ADULT
 3d      6d       12d      Lebensdauer: 21–45d
```

Alle Zeitwerte werden in **simulierten Tagen** (`alter_t`) gemessen. Ein simulierter Tag entspricht 120 Echtzeit-Sekunden (`SIM_TAG_DAUER_S = 120`). Ein vollständiger Entwicklungszyklus vom Ei bis zur Arbeiterin dauert also etwa **42 Echtminuten**.

---

## Ei (`ZELLE_BRUT_EI`)

- Von der Königin in eine saubere, leere Zelle gelegt
- Dauer: **3 simulierte Tage** (`EI_DAUER_T = 3.0`)
- Keine Fütterung nötig
- Temperaturabfall unter 35 °C erhöht das Sterblichkeitsrisiko
- Nach 3 Tagen → Zellentyp wechselt zu `ZELLE_BRUT_LARVE`

## Larve (`ZELLE_BRUT_LARVE`)

- Dauer: **6 simulierte Tage** (`LARVEN_DAUER_T = 6.0`)
- Muss aktiv von Ammenbienen (`AMME`) gefüttert werden
  - Jede Amme verbraucht dabei Honig + Pollen aus dem Volkvorrat
- **Hungerrisiko**: Sinken die Pollenreserven unter 5 g, steigt die Sterblichkeitswahrscheinlichkeit stark an
- **Temperaturstress**: Abweichung von 35 °C erhöht `p_tod` (Sterbewahrscheinlichkeit)
- Sterblichkeitsformel: `brut_sterblichkeit(temperatur_c, nahrungs_mangel)`
- Nach 6 Tagen → Zellentyp wechselt zu `ZELLE_BRUT_PUPPE`

## Puppe (`ZELLE_BRUT_PUPPE`)

- Dauer: **12 simulierte Tage** (`PUPPEN_DAUER_T = 12.0`)
- Keine Fütterung, aber weiterhin temperaturempfindlich (weniger tolerant als Larven)
- Nach 12 Tagen → Puppe schlüpft: `biene_schluepfen()` wird aufgerufen
  - Eine neue erwachsene Biene (Rolle: `PUTZERIN`) erscheint in der Lagerzone
  - Die Zelle wird zu `ZELLE_LEER` mit `sauber = false` (muss gereinigt werden)

## Adult

Neue Bienen beginnen als `PUTZERIN` — die jüngste Rolle. Der altersbasierte Rollenwechsel erfolgt automatisch:

| Alter (Tage) | Zugewiesene Rolle |
|--------------|------------------|
| < 3 | `PUTZERIN` — reinigt Zellen nach dem Schlüpfen |
| < 10 | `AMME` — füttert Larven |
| < 14 | `BAUERIN` — baut Waben |
| < 21 | `EMPFAENGERIN` / `FAECHER` — nimmt Nektar an, fächelt Luft |
| ≥ 24 | `SAMMLERIN_*` — sammelt draußen |

Siehe [Rollen & Aufgaben](Rollen-und-Aufgaben) für alle Details.

---

## Gestaffelte Startalter

Beim Spielstart füllt `bienenstock_init()` alle 578 Hexfeld-Zellen per `berechne_zellentyp()`. Brutzellen erhalten **zufällig gestaffelte Startalter**, damit nicht alle gleichzeitig schlüpfen:

| Stadium | Zufälliger Altersbereich |
|---------|--------------------------|
| Ei | 0,0 – 3,0 Tage |
| Larve | 0,0 – 6,0 Tage |
| Puppe | 0,0 – 12,0 Tage |

---

## Tod & Slot-Wiederverwendung

Gestorbene Bienen (`energie_j = 0` oder Altersschwäche) erhalten `aktiv = false`. Ihr Slot im flachen `bienen[]`-Array wird von der nächsten schlüpfenden Biene wiederverwendet — keine Speicherverwaltung nötig.

---

## Sterblichkeitsformel

```c
float brut_sterblichkeit(float temperatur_c, float nahrungs_mangel);
```

- Grundsterblichkeit: sehr klein pro Tick
- Temperaturstress: steigt stark außerhalb des Bereichs 32–36 °C
- Hungerfaktor: addiert sich bei `nahrungs_mangel > 0`
- Wird probabilistisch angewendet per deterministischem Hash aus `alter_t`
