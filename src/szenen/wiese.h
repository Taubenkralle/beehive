#ifndef WIESE_H
#define WIESE_H

#include "../kern/spielzustand.h"

// Wiese initialisieren (Blumen, Baeume, Bienen platzieren)
void wiese_init(Spielzustand* spiel);

// Bienenfluege und Blumenschwingen aktualisieren
void wiese_aktualisieren(Spielzustand* spiel, float delta);

// Vogelperspektive der Wiese zeichnen
void wiese_zeichnen(const Spielzustand* spiel);

// Ressourcen freigeben
void wiese_aufraeumen(Spielzustand* spiel);

#endif // WIESE_H
