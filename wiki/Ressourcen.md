# Ressourcen

Das Volk verwaltet fünf physikalische Ressourcen, alle in **echten Einheiten** (Gramm oder Milligramm) — keine abstrakten Prozentwerte.

---

## Ressourcenfelder

| Feld | Einheit | Beschreibung |
|------|---------|-------------|
| `honig_reif_g` | Gramm | Reifer Honig — langfristiger Energiespeicher |
| `nektar_unreif_g` | Gramm | Rohner Nektar, der noch verarbeitet wird |
| `pollen_g` | Gramm | Pollen — Proteinquelle für Larven |
| `wasser_g` | Gramm | Wasser — Kühlung und Nektarverdünnung |
| `wachs_mg` | Milligramm | Wachs — zum Bau neuer Wabenzellen |

---

## Nektar → Honig: Die Verarbeitungspipeline

Das ist die komplexeste Ressourcenkette. Es braucht mehrere reale Schritte, um rohen Blütennektar in haltbaren Honig zu verwandeln.

```
BLÜTENNEKTAR (30 % Zucker)
       ↓  Sammlerin sammelt → honigmagen_mg
       ↓  Sammlerin kehrt zurück → übergibt an Empfängerin
       ↓  wird zu nektar_unreif_g addiert
       ↓  Fächelbienen verdunsten Wasser
       ↓  REIFER HONIG (82 % Zucker, 18 % Wasser)
       ↓  wird zu honig_reif_g addiert
```

### Umrechnungsformel

```
zucker_g = nektar_g × zuckergehalt        (zuckergehalt = 0,30 für typischen Nektar)
honig_g  = zucker_g / (1 − wasseranteil)  (wasseranteil = 0,18 für reifen Honig)
```

Das bedeutet: Etwa **3,6 kg Nektar** ergeben **1 kg Honig**.

### Verdunstungsrate

Fächelbienen (`FAECHER` + `EMPFAENGERIN`) treiben die Wasserverdunstung an:

```c
float nektar_verdunstung(int faecher_anzahl, float dt);
```

Mehr Fächlerinnen → schnellere Verdunstung → schnellere Honigproduktion.

### Verarbeitungsrate

```c
#define FAECHER_NEKTAR_RATE_G_S  0.0005f   // Gramm verarbeitet pro Fächlerin pro Sekunde
```

Jeden Tick: `rate = (faecher + 1) × 0,0005 × dt` Gramm konzentrierter Nektar werden zu reifem Honig.

---

## Pollen

- Wird von `SAMMLERIN_POLLEN` an `TRACHT_POLLEN`-Trachtquellen gesammelt
- Wird von Ammenbienen (`AMME`) beim Larvenfüttern verbraucht — gemischt mit Honig zu „Bienenbrot"
- **Kritische Schwelle**: Unter **5 g** steigt das Larven-Hungerrisiko stark an
- Das Volk generiert Pollen-Sammelaufträge wenn `pollen_g < 100 g`

---

## Wachsproduktion

Wachs ist ein Nebenprodukt des Bienenstoffwechsels — Bienen verbrauchen Honig, um Wachsplättchen zu sekretieren.

```c
#define BIENE_WACHSPROD_G_S    0.00005f   // Honigverbrauch pro Bauerin pro Sekunde
#define WACHS_PRO_ZELLE_MG    1300.0f    // Wachs für eine neue Zelle
```

```
zucker_g = honig_verbraucht × (1 − WASSERANTEIL_HONIG)
wachs_mg += wachs_aus_zucker(zucker_g)    // 8 mg Wachs pro 1 g Zucker
```

Wenn angesammeltes Wachs 1300 mg überschreitet, wandelt `stock_wabe_bauen()` es in eine neue `ZELLE_LEER`-Zelle im Simulations-Grid um.

**Biologische Anmerkung:** Echte Bienen müssen ca. 6–8 kg Honig verbrauchen, um 1 kg Wachs zu produzieren. Die Simulation verwendet dasselbe Verhältnis.

---

## Startressourcen

Bei der Volkinitialisierung (`stock_init`):

| Ressource | Startmenge |
|-----------|-----------|
| Reifer Honig | 500 g |
| Unreifer Nektar | 0 g |
| Pollen | 80 g |
| Wasser | 15 g |
| Wachs | 3000 mg (~2 Zellen) |

---

## Energiehaushalt

Jede Biene hat eine persönliche Energiereserve (`energie_j`, in Joule):

| Aktivität | Leistungsaufnahme |
|-----------|------------------|
| Rasten | 0,001 W |
| Laufen | 0,003 W |
| Fliegen | 0,014 W |

```c
energie_verbrauch(leistung_w, dt) = leistung_w × dt   // Joule = Watt × Sekunden
energie_aufnahme(zucker_g)        = zucker_g × 17,0   // 17 J pro Gramm Zucker
```

Wenn `energie_j ≤ 0`, stirbt die Biene (`aktiv = false`). Eine Sammlerin, die zu weit fliegt ohne ausreichende Energiereserven, kehrt nicht mehr zurück.

---

## Der Honigmagen (`honigmagen_mg`)

Sammelbienen tragen Nektar in einem speziellen Magen, der vom eigenen Verdauungssystem getrennt ist. Nach der Rückkehr zum Stock wird der Nektar von Mund zu Mund an eine Empfängerin weitergegeben und schließlich zu `nektar_unreif_g` addiert.
