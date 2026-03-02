#ifndef SPEICHER_H
#define SPEICHER_H

#include "../kern/spielzustand.h"
#include <stdbool.h>

// Default save file path (relative to working directory)
#define SPEICHER_DATEI   "beehive.sav"

// Write full game state to a binary file.
// Returns true on success, false on any I/O error.
bool speicher_schreiben(const Spielzustand* spiel, const char* pfad);

// Read game state from a binary file.
// Returns true on success. Rejects files with wrong magic, version, or size.
bool speicher_lesen(Spielzustand* spiel, const char* pfad);

// True if a readable save file exists at pfad.
bool speicher_vorhanden(const char* pfad);

#endif // SPEICHER_H
