# Beehive — Wiki

**Beehive** ist ein biologisch realistischer 2D-Bienenvolk-Simulator, geschrieben in C mit raylib.

Die Simulation modelliert ein lebendiges Bienenvolk mit echten Mechanismen: Bienenentwicklung, Aufgabenverteilung, Ressourcenwirtschaft, Thermoregulation, Pheromonkommunikation und Wetter.

---

## Wiki-Seiten

| Seite | Inhalt |
|-------|--------|
| [Bienen-Lebenszyklus](Bienen-Lebenszyklus) | Ei → Larve → Puppe → Adult, Entwicklungszeiten |
| [Rollen & Aufgaben](Rollen-und-Aufgaben) | Alle 12 Arbeiterinnen-Rollen, das Auftragssystem |
| [Ressourcen](Ressourcen) | Honig, Nektar, Pollen, Wachs, Wasser — wie sie fließen |
| [Brutpflege & Thermoregulation](Brutpflege) | 35 °C im Brutnest halten |
| [Pheromon-System](Pheromon-System) | Alarm, Spur, Nasonov — wie Bienen kommunizieren |
| [Außenwelt & Wetter](Aussenwelt) | Trachtquellen, Tag-Nacht-Rhythmus, Wetter |
| [Navigation & Pathfinding](Navigation) | Flowfield-BFS — tausende Bienen, kein Ruckeln |
| [Speichersystem](Speichersystem) | Spielstand speichern und laden (F5 / F9) |
| [Architektur](Architektur) | Code-Struktur, Module, Datenfluss |

---

## Zwei Ansichten

Das Spiel hat zwei Kameraperspektiven, zwischen denen mit **TAB** gewechselt wird:

### Stock-Ansicht (Querschnitt)
Innenleben des Bienenstocks — Wabenzellen, arbeitende Bienen, die Königin in der Mitte.
Zeigt: Zellinhalte, Bienenpositionen, Volksdaten (Honig, Pollen, Temperatur, Brut).

### Wiesen-Ansicht (Vogelperspektive)
Außenwelt — Blumen, Bäume, Teich, das Stockgebäude.
Zeigt: Trachtquellen mit Füllstandsanzeige, fliegende Sammlerinnen, Wetterinfos.

---

## Steuerung

| Taste | Aktion |
|-------|--------|
| TAB | Zwischen Stock- und Wiesen-Ansicht wechseln |
| P | Pheromon-Debug-Overlay ein/ausschalten |
| LMB | Alarmpheromon an Mausposition setzen (Debug) |
| S (gehalten) | Spurpheromon an Mausposition setzen (Debug) |
| F5 | Spielstand speichern |
| F9 | Spielstand laden |
| ESC | Beenden |
