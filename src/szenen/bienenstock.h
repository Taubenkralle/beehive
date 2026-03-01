#ifndef BIENENSTOCK_H
#define BIENENSTOCK_H

#include "../kern/spielzustand.h"

// Bienenstock initialisieren (Waben befuellen, Bienen platzieren)
void bienenstock_init(Spielzustand* spiel);

// Bienenbewegung und Animation aktualisieren
void bienenstock_aktualisieren(Spielzustand* spiel, float delta);

// Querschnitt-Ansicht des Stocks zeichnen
void bienenstock_zeichnen(const Spielzustand* spiel);

// Ressourcen freigeben
void bienenstock_aufraeumen(Spielzustand* spiel);

#endif // BIENENSTOCK_H
