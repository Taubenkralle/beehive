#include "speicher.h"
#include <stdio.h>
#include <stdint.h>

// ---------------------------------------------------------------------------
// File format
//   Byte 0–11 : SpeicherHeader (magic, version, data size)
//   Byte 12–  : raw Spielzustand binary dump
//
// The size field acts as a struct-layout guard: if the binary changes between
// versions the size will differ and the load is refused gracefully.
// ---------------------------------------------------------------------------

#define SPEICHER_MAGIC    0x42454548U   // ASCII "BEEH"
#define SPEICHER_VERSION  1U

typedef struct {
    uint32_t magic;         // Identifies this file as a Beehive save
    uint32_t version;       // Format version — must match SPEICHER_VERSION
    uint32_t datengroesse;  // sizeof(Spielzustand) — layout sanity check
} SpeicherHeader;

// ---------------------------------------------------------------------------
bool speicher_schreiben(const Spielzustand* spiel, const char* pfad) {
    FILE* f = fopen(pfad, "wb");
    if (!f) return false;

    SpeicherHeader header = {
        .magic        = SPEICHER_MAGIC,
        .version      = SPEICHER_VERSION,
        .datengroesse = (uint32_t)sizeof(Spielzustand),
    };

    if (fwrite(&header, sizeof(header), 1, f) != 1) { fclose(f); return false; }
    if (fwrite(spiel, sizeof(Spielzustand), 1, f)  != 1) { fclose(f); return false; }

    fclose(f);
    return true;
}

// ---------------------------------------------------------------------------
bool speicher_lesen(Spielzustand* spiel, const char* pfad) {
    FILE* f = fopen(pfad, "rb");
    if (!f) return false;

    SpeicherHeader header;
    if (fread(&header, sizeof(header), 1, f) != 1) { fclose(f); return false; }

    // Validate: wrong magic → not a Beehive save file
    if (header.magic != SPEICHER_MAGIC) { fclose(f); return false; }

    // Validate: version mismatch → refuse to load incompatible format
    if (header.version != SPEICHER_VERSION) { fclose(f); return false; }

    // Validate: struct layout changed → save is stale, do not load
    if (header.datengroesse != (uint32_t)sizeof(Spielzustand)) { fclose(f); return false; }

    if (fread(spiel, sizeof(Spielzustand), 1, f) != 1) { fclose(f); return false; }

    fclose(f);
    return true;
}

// ---------------------------------------------------------------------------
bool speicher_vorhanden(const char* pfad) {
    FILE* f = fopen(pfad, "rb");
    if (!f) return false;
    fclose(f);
    return true;
}
