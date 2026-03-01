#include "raylib.h"
#include "kern/spielzustand.h"
#include "szenen/bienenstock.h"
#include "szenen/wiese.h"
#include "sim/pheromon.h"
#include "sim/haushalt.h"
#include "sim/agent_biene.h"
#include "sim/sim_stock.h"
#include "sim/sim_welt.h"
#include "sim/pfad_stock.h"
#include "sim/pfad_welt.h"

int main(void) {
    InitWindow(FENSTER_BREITE, FENSTER_HOEHE, "Beehive");
    SetTargetFPS(ZIEL_FPS);

    static Spielzustand spiel = {0};
    spiel.ansicht = ANSICHT_STOCK;

    bienenstock_init(&spiel);
    wiese_init(&spiel);
    pheromon_init(spiel.pheromonfelder);
    haushalt_selbsttest();

    // Außenwelt initialisieren (Trachtquellen, Startzeit, Wetter)
    welt_init(&spiel);

    // Stocksimulation initialisieren (Bienen, Brut, Ressourcen, Auftraege)
    stock_init(&spiel);

    // Flowfields berechnen (einmalig; schnell da BFS auf 64×36 = 2304 Zellen)
    pfad_stock_init();
    pfad_welt_init(&spiel);

    while (!WindowShouldClose()) {
        float delta = GetFrameTime();

        // --- Eingabe ---
        // TAB: Ansicht wechseln
        if (IsKeyPressed(KEY_TAB)) {
            spiel.ansicht = (spiel.ansicht == ANSICHT_STOCK)
                            ? ANSICHT_WIESE : ANSICHT_STOCK;
        }
        // P: Pheromon-Debug-Overlay ein/aus
        if (IsKeyPressed(KEY_P))
            spiel.pheromon_debug = !spiel.pheromon_debug;

        // Linke Maustaste: Alarmpheromon an Mausposition abgeben (Debug/Test)
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 maus = GetMousePosition();
            pheromon_abgeben(&spiel.pheromonfelder[PHEROMON_ALARM],
                             maus.x, maus.y, 8.0f);
        }
        // S gehalten: Spurpheromon an Mausposition (Debug/Test)
        if (IsKeyDown(KEY_S)) {
            Vector2 maus = GetMousePosition();
            pheromon_abgeben(&spiel.pheromonfelder[PHEROMON_SPUR],
                             maus.x, maus.y, 5.0f);
        }

        // --- Simulation ---
        welt_aktualisieren(&spiel, delta);          // Tageszeit, Wetter, Trachtquellen
        pheromon_aktualisieren(spiel.pheromonfelder, delta);
        alle_bienen_aktualisieren(&spiel, delta);
        stock_aktualisieren(&spiel, delta);

        if (spiel.ansicht == ANSICHT_STOCK)
            bienenstock_aktualisieren(&spiel, delta);
        else
            wiese_aktualisieren(&spiel, delta);

        // --- Zeichnen ---
        BeginDrawing();
        if (spiel.ansicht == ANSICHT_STOCK)
            bienenstock_zeichnen(&spiel);
        else
            wiese_zeichnen(&spiel);

        // Pheromon-Overlay (über der Szene, unter HUD)
        if (spiel.pheromon_debug) {
            pheromon_zeichnen(&spiel.pheromonfelder[PHEROMON_ALARM],   PHEROMON_ALARM);
            pheromon_zeichnen(&spiel.pheromonfelder[PHEROMON_SPUR],    PHEROMON_SPUR);
            pheromon_zeichnen(&spiel.pheromonfelder[PHEROMON_NASONOV], PHEROMON_NASONOV);
            DrawText("[P] Pheromon-Debug AN", 10, FENSTER_HOEHE - 44, 13,
                     (Color){255, 255, 100, 200});
            DrawText("LMB=Alarm  S=Spur", 10, FENSTER_HOEHE - 28, 13,
                     (Color){200, 200, 200, 180});
        }

        EndDrawing();
    }

    bienenstock_aufraeumen(&spiel);
    wiese_aufraeumen(&spiel);
    CloseWindow();
    return 0;
}
