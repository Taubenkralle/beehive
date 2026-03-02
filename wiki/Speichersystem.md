# Speichersystem

Der Spielstand wird in einer einzigen Binärdatei (`beehive.sav`) gespeichert. Das System ist minimalistisch und schnell: keine Serialisierungsbibliothek, kein JSON, kein XML — nur ein direkter Speicherabzug des gesamten `Spielzustand`-Structs.

---

## Steuerung

| Taste | Aktion |
|-------|--------|
| **F5** | Spielstand speichern → `beehive.sav` |
| **F9** | Spielstand laden ← `beehive.sav` |

Beim **Programmstart** wird automatisch geladen, wenn eine `beehive.sav` im Arbeitsverzeichnis existiert.

Nach Speichern oder Laden erscheint 2 Sekunden lang eine Bestätigungsmeldung in der Bildschirmmitte.

---

## Dateiformat

```
Byte  0 –  3 : magic       = 0x42454548  ("BEEH" in ASCII)
Byte  4 –  7 : version     = 1
Byte  8 – 11 : datengroesse = sizeof(Spielzustand)
Byte 12 – …  : rohe Binärdaten des Spielzustand-Structs
```

Der Header hat drei Felder:

| Feld | Typ | Zweck |
|------|-----|-------|
| `magic` | `uint32_t` | Datei-Identifikation — ist das überhaupt eine Beehive-Datei? |
| `version` | `uint32_t` | Format-Version — ermöglicht später inkompatible Änderungen abzufangen |
| `datengroesse` | `uint32_t` | `sizeof(Spielzustand)` — Struct-Layout-Check |

Beim Laden wird geprüft:
1. `magic == 0x42454548` — sonst: keine Beehive-Datei
2. `version == 1` — sonst: veraltetes Format
3. `datengroesse == sizeof(Spielzustand)` — sonst: Struct hat sich geändert (z.B. neues Feld hinzugefügt)

Schlägt eine dieser Prüfungen fehl, wird das Laden abgebrochen und der aktuelle Zustand bleibt erhalten.

---

## Warum binär?

Der `Spielzustand`-Struct enthält über 300 KB Daten:

- `bienen[1000]` — je Biene: Position, Energie, Auftrag, Zustand, ...
- `zellen[578]` — alle Wabenzellen mit Typ, Füllstand, Alter
- `pheromonfelder[3]` — je `128×72 = 9216` Floats pro Feld (~108 KB allein für Pheromone)
- Wetter, Ressourcen, Trachtquellen, Auftragsqueue, ...

Ein binärer Dump mit `fwrite` ist:
- **schnell**: ein einziger Systemaufruf, kein Parsing
- **einfach**: kein Formatierungscode nötig
- **vollständig**: alle Felder werden automatisch mitgespeichert

Der Nachteil ist die fehlende Lesbarkeit für Menschen — aber das ist für einen Spielstand kein Problem.

---

## Implementierung (`src/sim/speicher.h/.c`)

### Öffentliche API

```c
// Standard-Dateiname (relativ zum Arbeitsverzeichnis)
#define SPEICHER_DATEI   "beehive.sav"

// Spielstand in Datei schreiben — gibt true bei Erfolg zurück
bool speicher_schreiben(const Spielzustand* spiel, const char* pfad);

// Spielstand aus Datei laden — gibt true bei Erfolg zurück
bool speicher_lesen(Spielzustand* spiel, const char* pfad);

// Prüft, ob eine lesbare Speicherdatei existiert
bool speicher_vorhanden(const char* pfad);
```

### Speichern

```c
bool speicher_schreiben(const Spielzustand* spiel, const char* pfad) {
    FILE* f = fopen(pfad, "wb");
    if (!f) return false;

    SpeicherHeader header = {
        .magic        = SPEICHER_MAGIC,    // 0x42454548
        .version      = SPEICHER_VERSION,  // 1
        .datengroesse = sizeof(Spielzustand),
    };

    if (fwrite(&header, sizeof(header), 1, f) != 1) { fclose(f); return false; }
    if (fwrite(spiel,   sizeof(*spiel), 1, f) != 1) { fclose(f); return false; }

    fclose(f);
    return true;
}
```

### Laden

```c
bool speicher_lesen(Spielzustand* spiel, const char* pfad) {
    FILE* f = fopen(pfad, "rb");
    if (!f) return false;

    SpeicherHeader header;
    fread(&header, sizeof(header), 1, f);

    if (header.magic        != SPEICHER_MAGIC)         { fclose(f); return false; }
    if (header.version      != SPEICHER_VERSION)        { fclose(f); return false; }
    if (header.datengroesse != sizeof(Spielzustand))    { fclose(f); return false; }

    fread(spiel, sizeof(Spielzustand), 1, f);
    fclose(f);
    return true;
}
```

---

## Auto-Load beim Start

In `main.c` wird vor der ersten Spielschleife geprüft, ob ein Spielstand existiert:

```c
if (speicher_vorhanden(SPEICHER_DATEI))
    speicher_lesen(&spiel, SPEICHER_DATEI);
```

Das bedeutet: Das Spiel startet immer dort weiter, wo man aufgehört hat — ohne Hauptmenü, ohne Klick.

---

## Dateigrößen-Abschätzung

| Komponente | Größe |
|------------|-------|
| `SpeicherHeader` | 12 Byte |
| `bienen[1000]` | ~80 KB |
| `zellen[578]` | ~9 KB |
| `pheromonfelder[3]` | ~108 KB |
| Rest (Ressourcen, Wetter, Aufträge, Quellen, ...) | ~30 KB |
| **Gesamt** | **~227 KB** |

Eine `beehive.sav` ist also etwa 200–250 KB groß — vernachlässigbar klein.

---

## Versionierung

Wird dem `Spielzustand`-Struct ein neues Feld hinzugefügt (z.B. `jahreszeit`), ändert sich `sizeof(Spielzustand)`. Alte Spielstände werden dann beim Laden erkannt und abgelehnt (`datengroesse`-Check). In zukünftigen Versionen kann hier eine Migration eingebaut werden.

Aktuell: `SPEICHER_VERSION = 1`
