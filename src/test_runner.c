#include "../inc/test_runner.h"
#include "../inc/wrestler.h"

// Access to globals from main.c
extern Wrestler player1;
extern Wrestler player2;
extern void handleCollisions(u16 p1_input, u16 p2_input);
extern void processGrappleLogic();
extern void updateWrestler(Wrestler* w, u16 input);

// Helper to reset state between tests
void reset_gamestate() {
    initWrestler(&player1, FIX32(100), FIX32(120));
    initWrestler(&player2, FIX32(200), FIX32(120));
    player2.facingRight = FALSE;
}

// TEST 1: Momentum Economy Initialization
bool test_momentum_init() {
    reset_gamestate();
    if (player1.stamina != 100) return FALSE;
    if (player1.heat != 0) return FALSE;
    return TRUE;
}

// TEST 2: Selling Mechanic (Regen)
bool test_selling_mechanic() {
    reset_gamestate();
    
    // Setup: Player 1 is Grounded with low stamina
    player1.state = STATE_GROUNDED;
    player1.stamina = 10;
    
    // Action: Hold C (Sell) for 10 frames
    for(int i=0; i<10; i++) {
        // Simulate update loop
        updateWrestler(&player1, BUTTON_C);
    }
    
    // Assert: State should be SELLING
    if (player1.state != STATE_SELLING) return FALSE;
    
    // Assert: Stamina should have increased by 18 (9 effective frames * 2)
    // 10 (start) + 18 = 28. 
    // Note: The first frame consumes the transition from GROUNDED to SELLING, 
    // so no regen happens on frame 0.
    if (player1.stamina != 28) return FALSE;
    
    return TRUE;
}

// TEST 3: Grapple Timing Logic (The Duel)
bool test_grapple_timing() {
    reset_gamestate();
    
    // Setup: Force collision
    player1.x = FIX32(100);
    player2.x = FIX32(100); // Overlap
    
    // Trigger collision logic
    handleCollisions(0, 0);
    
    // Assert: Should be in GRAPPLE_INIT
    if (player1.state != STATE_GRAPPLE_INIT) return FALSE;
    
    // Fast forward to Sync Point (Frame 10)
    player1.stateTimer = 10; 
    
    // Simulate Player 1 pressing 'A' exactly on time
    player1.bufferedInput = BUTTON_A;
    player2.bufferedInput = 0;
    
    // Run Logic
    processGrappleLogic(); // The function might rely on timer logic inside
    
    // Actually, processGrappleLogic relies on timer > 10+15 to resolve.
    // So let's fast forward past window.
    player1.stateTimer = 30; 
    processGrappleLogic();
    
    // Assert: Player 1 (Winner) should be attacking, P2 thrown
    // Because P1 input A and P2 input nothing.
    if (player1.state != STATE_ATTACK_HEAVY) return FALSE; // Winner state
    if (player2.state != STATE_THROWN) return FALSE;       // Loser state
    
    return TRUE;
}

bool run_all_tests() {
    VDP_drawText("Running Tests...", 10, 10);
    
    if (!test_momentum_init()) {
        VDP_drawText("FAIL: Momentum Init", 10, 12);
        return FALSE;
    }
    
    if (!test_selling_mechanic()) {
        VDP_drawText("FAIL: Selling Mech", 10, 12);
        return FALSE;
    }

    if (!test_grapple_timing()) {
        VDP_drawText("FAIL: Grapple Timing", 10, 12);
        return FALSE;
    }
    
    return TRUE;
}

