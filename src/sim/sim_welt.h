#ifndef SIM_WELT_H
#define SIM_WELT_H

#include "../kern/spielzustand.h"

// ---------------------------------------------------------------------------
// World timing
// ---------------------------------------------------------------------------

// Duration of one simulated day in real seconds.
// 120 s/day → full day/night cycle every 2 minutes of play.
// Adjust for taste; brood timing uses a separate real-time conversion.
#define SIM_TAG_DAUER_S     120.0f

// ---------------------------------------------------------------------------
// Weather tuning
// ---------------------------------------------------------------------------

#define AUSSENTEMP_BASIS_C   18.0f   // Mean daytime temperature (°C)
#define AUSSENTEMP_AMPLITUDE  8.0f   // Day/night swing (°C)
#define FLUG_TEMP_MIN_C      10.0f   // Below this bees stay home
#define FLUG_WIND_MAX        0.8f    // Above this wind blocks flight
#define WETTER_INTERVALL_S   20.0f   // Weather state checked every N seconds
#define WIND_AENDERUNG       0.04f   // Max wind change per weather check

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

// Set up forage patches and initial world state (call once at startup)
void welt_init(Spielzustand* spiel);

// Main world tick: advances time, regenerates patches, updates weather
void welt_aktualisieren(Spielzustand* spiel, float dt);

// Regenerate nectar/pollen/water at a forage source for this tick
void trachtquelle_aktualisieren(Trachtquelle* q, const Spielzustand* spiel, float dt);

// Simulate one weather tick (wind, rain, temperature)
void wetter_aktualisieren(Spielzustand* spiel, float dt);

// True if weather allows bees to fly outdoors
bool flug_moeglich(const Spielzustand* spiel);

#endif // SIM_WELT_H
