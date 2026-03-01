#ifndef PHEROMON_H
#define PHEROMON_H

// Pheromone grid dimensions (1 cell = 10x10 pixels at 1280x720)
#define PHEROMON_BREITE  128
#define PHEROMON_HOEHE    72
#define PHEROMON_ANZAHL    3   // Number of distinct pheromone types

// Pheromone types used by the colony
typedef enum {
    PHEROMON_SPUR = 0,   // Trail pheromone: foragers mark path to patch
    PHEROMON_ALARM,      // Alarm pheromone: guards signal danger
    PHEROMON_NASONOV,    // Nasonov pheromone: orientation / home signal
} PheromonTyp;

// One flat grid per pheromone type — cache-friendly, no malloc
typedef struct {
    float zellen[PHEROMON_BREITE * PHEROMON_HOEHE];
    float diffusion;     // D: spread speed (grid-cells² / s)
    float zerfall;       // k: decay rate (1/s)
} PheromonFeld;

// Initialize all fields with zeroes and tuned constants
void  pheromon_init(PheromonFeld felder[PHEROMON_ANZAHL]);

// Diffusion + decay step — call once per sim tick
void  pheromon_aktualisieren(PheromonFeld felder[PHEROMON_ANZAHL], float dt);

// Bee emits pheromone at world position (pixels)
void  pheromon_abgeben(PheromonFeld* feld, float welt_x, float welt_y, float menge);

// Read concentration at world position — bilinear interpolation
float pheromon_lesen(const PheromonFeld* feld, float welt_x, float welt_y);

// Debug: draw colored heatmap overlay (requires raylib)
void  pheromon_zeichnen(const PheromonFeld* feld, PheromonTyp typ);

#endif // PHEROMON_H
