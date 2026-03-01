# Rollen & Aufgaben

Das Volk läuft über eine **bedarfsgesteuerte Auftragswarteschlange**. Jeden Tick misst `stock_auftraege_generieren()`, was das Volk braucht, schreibt Aufträge in die Warteschlange, und Bienen übernehmen den jeweils passenden Auftrag nach Rolle und Alter.

---

## Arbeiterinnen-Rollen (`BienRolle`)

| Code | Rolle | Beschreibung |
|------|-------|-------------|
| `PUTZERIN` | Putzerin | Reinigt leere Zellen, damit die Königin Eier legen kann |
| `AMME` | Amme | Füttert Larven (verbraucht Honig + Pollen) |
| `BAUERIN` | Bauerin | Verarbeitet Wachs zu neuen Wabenzellen |
| `EMPFAENGERIN` | Empfängerin | Nimmt Nektar von Sammlerinnen an und verarbeitet ihn |
| `FAECHER` | Fächlerin | Fächelt Luft, verdunstet Wasser aus Nektar, kühlt den Stock |
| `WAECHTER` | Wächterin | Bewacht den Eingang, hält Eindringlinge ab |
| `BESTATERIN` | Bestatterin | Entfernt tote Bienen aus dem Stock |
| `SAMMLERIN_NEKTAR` | Nektar-Sammlerin | Fliegt aus und sammelt Nektar von Blüten |
| `SAMMLERIN_POLLEN` | Pollen-Sammlerin | Sammelt Pollen (Proteinquelle für Larven) |
| `SAMMLERIN_WASSER` | Wasser-Sammlerin | Holt Wasser zur Kühlung und Honigreifung |
| `SAMMLERIN_PROPOLIS` | Propolis-Sammlerin | Sammelt Harz zum Abdichten von Spalten |
| `ERKUNDERIN` | Erkunderin | Sucht neue Trachtquellen |

### Altersbasierte Rollenprogression

Echte Honigbienen wechseln ihre Rolle mit zunehmendem Alter (temporale Polymorphie). Die Simulation bildet das nach:

```
Schlüpfen  →  PUTZERIN  →  AMME  →  BAUERIN  →  FAECHER/EMPFAENGERIN  →  SAMMLERIN
   Tag 0         Tag 3      Tag 10     Tag 14           Tag 21               Tag 24+
```

`biene_rolle_waehlen()` läuft jeden Tick und vergleicht `b->alter_t` mit den Schwellenwerten. Wenn das Volk dringend Ammenbienen braucht, kann eine Biene länger in ihrer Rolle gehalten werden.

---

## Bewegungszustände (`BienBewegung`)

Jede Biene hat eine Zustandsmaschine mit 7 Zuständen:

| Code | Zustand | Beschreibung |
|------|---------|-------------|
| `IM_STOCK_ARBEITEND` | Arbeitet im Stock | Führt einen Innenauftrag am Zielort aus |
| `ZUM_AUSGANG` | Zum Ausgang | Läuft zum Einflugsloch |
| `AUSSENFLUG` | Außenflug | Fliegt zu einer Trachtquelle |
| `AM_SAMMELN` | Am Sammeln | Sammelt aktiv Ressourcen an der Quelle |
| `HEIMFLUG` | Heimflug | Fliegt zurück zum Stock |
| `ZUM_ABLADEN` | Zum Ablageplatz | Sucht Abgabestelle im Stock |
| `RASTEN` | Rastet | Erholt sich, regeneriert Energie |

Zustandsübergänge werden von `biene_aktualisieren()` → `biene_entscheiden()` gesteuert.

---

## Die Auftragswarteschlange

Aufträge liegen in `spiel->auftraege[]` (max. 256 Slots). Jeder `Auftrag` enthält:

```c
typedef struct {
    AuftragsTyp typ;
    int   ziel_zelle;     // Zell-Index (-1 = zonenbasiert)
    float ziel_x, ziel_y; // Zielposition in der Welt
    int   prioritaet;     // 1 = höchste Dringlichkeit
    bool  vergeben;       // Bereits von einer Biene beansprucht
    bool  aktiv;          // Slot wird genutzt (false = wiederverwendbar)
} Auftrag;
```

### Index-Stabilität

Wenn eine Biene einen Auftrag annimmt, speichert sie den **Array-Index** (`auftrag_id`). Würde die Warteschlange durch Verschieben komprimiert, wären diese Referenzen ungültig. Stattdessen erhalten abgeschlossene/stornierte Aufträge `aktiv = false`, und ihr Slot wird **in-place** von neuen Aufträgen überschrieben. Kein Verschieben, keine kaputten Referenzen.

### Auftragstypen

**Innenaufträge:**

| Code | Priorität | Wird generiert wenn |
|------|-----------|---------------------|
| `AUFTRAG_LARVE_FUETTERN` | 1 | `brut_larve > 0` |
| `AUFTRAG_EINGANG_BEWACHEN` | 1 | Immer mindestens 1 Wächterin |
| `AUFTRAG_FAECHELN` | 2 | `temperatur_c > 36 °C` |
| `AUFTRAG_NEKTAR_VERARBEITEN` | 2 | `nektar_unreif_g > 0,1` |
| `AUFTRAG_ZELLE_REINIGEN` | 2 | Schmutzige Leerzellen vorhanden |
| `AUFTRAG_WABE_BAUEN` | 3 | `wachs_mg ≥ 1300 mg` |

**Außenaufträge:**

| Code | Priorität | Wird generiert wenn |
|------|-----------|---------------------|
| `AUFTRAG_NEKTAR_SAMMELN` | 3 | Immer (≈30 % des Volkes) |
| `AUFTRAG_POLLEN_SAMMELN` | 3 | `pollen_g < 100 g` |
| `AUFTRAG_WASSER_HOLEN` | 3 | Bei Bedarf |
| `AUFTRAG_PROPOLIS_HOLEN` | 3 | Bei Bedarf |

Außenaufträge werden **blockiert, wenn `flug_gesperrt = true`** (Regen, Sturm, Nacht — siehe [Außenwelt](Aussenwelt)).

---

## Wie eine Biene einen Auftrag annimmt

1. Biene geht in `RASTEN` oder beendet vorherigen Auftrag → `biene_auftrag_holen()` wird aufgerufen
2. Durchsucht `auftraege[]` nach: `aktiv && !vergeben && typ passt zu rolle`
3. Wählt den Eintrag mit der niedrigsten `prioritaet`-Zahl (höchste Dringlichkeit)
4. Setzt `vergeben = true`, speichert Index als `b->auftrag_id`
5. Setzt Bewegungszustand: Innen → `IM_STOCK_ARBEITEND`, Außen → `ZUM_AUSGANG`

Wenn der Auftrag erledigt ist, setzt `auftrag_freigeben()` → `aktiv = false`.

---

## Lastentransport

Sammlerinnen tragen Ressourcen in dedizierten Feldern des `Biene`-Structs:

| Feld | Inhalt |
|------|--------|
| `honigmagen_mg` | Nektar im Honigmagen (mg) |
| `pollenladung_mg` | Pollen an den Hinterbeinen (mg) |
| `wasser_mg` | Getragenes Wasser (mg) |
