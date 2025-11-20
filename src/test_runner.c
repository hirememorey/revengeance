#include "../inc/test_runner.h"
#include "../inc/wrestler.h"

// Access to globals from main.c
extern Wrestler player1;
extern Wrestler player2;
extern void handleCollisions(u16 p1_input, u16 p2_input);
extern void handleStrikes();
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
    
    // Assert: Player 1 (Winner) should be executing Suplex (A -> Suplex)
    // Because P1 input A and P2 input nothing.
    if (player1.state != STATE_SUPLEX_EXECUTE) return FALSE; // Winner state
    if (player2.state != STATE_SUPLEX_VICTIM) return FALSE;  // Loser state
    
    return TRUE;
}

// TEST 4: Hitbox Collision (Strike)
bool test_hitbox_collision() {
    reset_gamestate();

    // Setup: Player 1 close to Player 2
    // P1 at 100, P2 at 120. (Hitbox x offset +20, width 16 -> reaches 136)
    // Hurtbox x = 120+4 = 124.
    // 100 + 20 (offset) = 120 start. Width 16 -> 136 end.
    // P2 Hurtbox starts at 124. Collision!
    player1.x = FIX32(100);
    player2.x = FIX32(120); 
    
    // 1. Verify No Collision on Inactive Frame
    player1.state = STATE_ATTACK_LIGHT;
    player1.stateTimer = 2; // Too early (starts at 4)
    
    handleStrikes();
    
    if (player2.state != STATE_IDLE) return FALSE; // Should not be hit yet

    // 2. Verify Collision on Active Frame
    player1.stateTimer = 5; // Active!
    
    handleStrikes();
    
    // Assert: P2 should be Stunned
    if (player2.state != STATE_STUNNED) return FALSE;
    if (player2.stamina >= 100) return FALSE; // Should have taken damage
    
    return TRUE;
}

// TEST 5: Hitbox Whiff (Out of Range)
bool test_hitbox_whiff() {
    reset_gamestate();
    
    // Setup: Players far apart
    player1.x = FIX32(10);
    player2.x = FIX32(200);
    
    player1.state = STATE_ATTACK_LIGHT;
    player1.stateTimer = 5; // Active frame
    
    handleStrikes();
    
    // Assert: P2 should NOT be stunned
    if (player2.state != STATE_IDLE) return FALSE;
    
    return TRUE;
}

// TEST 6: Trade (Simultaneous Hits)
bool test_simultaneous_hits() {
    reset_gamestate();
    
    // Setup: Close range, both attacking
    player1.x = FIX32(100);
    player2.x = FIX32(120);
    player2.facingRight = FALSE; // Facing P1
    
    // Both active frames
    player1.state = STATE_ATTACK_LIGHT;
    player1.stateTimer = 5;
    
    player2.state = STATE_ATTACK_LIGHT;
    player2.stateTimer = 5;
    
    handleStrikes();
    
    // Assert: BOTH should be stunned (Double KO Logic)
    if (player1.state != STATE_STUNNED) return FALSE;
    if (player2.state != STATE_STUNNED) return FALSE;
    
    return TRUE;
}

// TEST 7: Strike Interruption (Startup Vulnerability)
bool test_strike_interruption() {
    reset_gamestate();
    
    // Setup: Close range
    player1.x = FIX32(100);
    player2.x = FIX32(120);
    player2.facingRight = FALSE;

    // P1 is active (hitting)
    player1.state = STATE_ATTACK_LIGHT;
    player1.stateTimer = 5;
    
    // P2 is in startup (vulnerable)
    player2.state = STATE_ATTACK_LIGHT;
    player2.stateTimer = 2; // Not active yet
    
    handleStrikes();
    
    // Assert: P1 should be fine (Winner)
    if (player1.state != STATE_ATTACK_LIGHT) return FALSE;
    
    // Assert: P2 should be Stunned (Interrupted)
    if (player2.state != STATE_STUNNED) return FALSE;
    
    return TRUE;
}

// TEST 8: Facing Direction (Back Turned)
bool test_facing_direction() {
    reset_gamestate();
    
    // Setup: P1 at 100, P2 at 80 (Behind P1)
    player1.x = FIX32(100);
    player2.x = FIX32(80);
    
    // P1 Facing Right (Away from P2)
    player1.facingRight = TRUE;
    
    // Action: P1 Attacks
    player1.state = STATE_ATTACK_LIGHT;
    player1.stateTimer = 5; // Active
    
    handleStrikes();
    
    // Assert: P2 should NOT be hit because P1 is punching the other way
    // Hitbox for Right Facing: x + 20.
    // P1 x=100 -> Hitbox x=120.
    // P2 x=80 -> Hurtbox x=80. No overlap.
    if (player2.state != STATE_IDLE) return FALSE;
    
    return TRUE;
}

// TEST 9: Move Transition (Suplex)
bool test_move_suplex() {
    reset_gamestate();
    
    // Setup: Force Collision into Grapple
    player1.x = FIX32(100);
    player2.x = FIX32(100);
    
    // Manually set state to Grapple Init Sync Point
    player1.state = STATE_GRAPPLE_INIT;
    player1.stateTimer = 26; // > 10 + 15 (Expired Window)
    
    // P1 presses B (Medium) -> Suplex
    player1.bufferedInput = BUTTON_B;
    player2.bufferedInput = 0;
    
    processGrappleLogic();
    
    // Assert: P1 Executing Suplex, P2 is Victim
    if (player1.state != STATE_SUPLEX_EXECUTE) return FALSE;
    if (player2.state != STATE_SUPLEX_VICTIM) return FALSE;

    // Assert: Resources (Win: +15 Heat, Lose: -15 Stamina, +5 Damage)
    if (player1.heat != 15) return FALSE;
    if (player2.stamina != 85) return FALSE;
    if (player2.bodyDamage != 5) return FALSE;
    
    return TRUE;
}

// TEST 10: Move Transition (Piledriver)
bool test_move_piledriver() {
    reset_gamestate();
    
    // Setup: P2 MUST have low stamina (<40) to avoid Auto-Reversal (Soul Rule)
    player2.stamina = 30;

    player1.state = STATE_GRAPPLE_INIT;
    player1.stateTimer = 26;
    
    // P1 presses C (Heavy) -> Piledriver
    player1.bufferedInput = BUTTON_C;
    player2.bufferedInput = 0;
    
    processGrappleLogic();
    
    // Assert: P1 Executing Piledriver, P2 is Victim
    if (player1.state != STATE_PILEDRIVER_EXECUTE) return FALSE;
    if (player2.state != STATE_PILEDRIVER_VICTIM) return FALSE;

    // Assert: Resources (Win: +25 Heat, Lose: -25 Stamina, +10 Damage)
    // P2 Stamina start 30 - 25 = 5
    if (player1.heat != 25) return FALSE;
    if (player2.stamina != 5) return FALSE;
    if (player2.bodyDamage != 10) return FALSE;
    
    return TRUE;
}

// TEST 11: Move Duration (Exit State)
bool test_move_duration() {
    reset_gamestate();
    
    // Setup: P1 executing Suplex
    player1.state = STATE_SUPLEX_EXECUTE;
    player1.stateTimer = 0;
    
    // Forward 59 frames (Should still be executing)
    for(int i=0; i<59; i++) {
        updateWrestler(&player1, 0);
    }
    if (player1.state != STATE_SUPLEX_EXECUTE) return FALSE;
    
    // Frame 60 -> 61 (Exit)
    updateWrestler(&player1, 0); // 60
    updateWrestler(&player1, 0); // 61 -> Transition
    
    if (player1.state != STATE_IDLE) return FALSE;
    
    return TRUE;
}

// TEST 12: Input Lockout (Cannot Move During Animation)
bool test_input_lockout() {
    reset_gamestate();
    
    player1.state = STATE_SUPLEX_EXECUTE;
    player1.x = FIX32(100);
    
    // Try to move right
    updateWrestler(&player1, BUTTON_RIGHT);
    
    // Assert: Position should NOT change
    if (player1.x != FIX32(100)) return FALSE;
    if (player1.velX != FIX32(0)) return FALSE;
    
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
    
    if (!test_hitbox_collision()) {
        VDP_drawText("FAIL: Hitbox Check", 10, 12);
        return FALSE;
    }

    if (!test_hitbox_whiff()) {
        VDP_drawText("FAIL: Hitbox Whiff", 10, 12);
        return FALSE;
    }

    if (!test_simultaneous_hits()) {
        VDP_drawText("FAIL: Trade Check", 10, 12);
        return FALSE;
    }
    
    if (!test_strike_interruption()) {
        VDP_drawText("FAIL: Interruption", 10, 12);
        return FALSE;
    }
    
    if (!test_facing_direction()) {
        VDP_drawText("FAIL: Facing Dir", 10, 12);
        return FALSE;
    }
    
    if (!test_move_suplex()) {
        VDP_drawText("FAIL: Suplex Logic", 10, 12);
        return FALSE;
    }

    if (!test_move_piledriver()) {
        VDP_drawText("FAIL: Piledriver Logic", 10, 12);
        return FALSE;
    }

    if (!test_move_duration()) {
        VDP_drawText("FAIL: Move Duration", 10, 12);
        return FALSE;
    }

    if (!test_input_lockout()) {
        VDP_drawText("FAIL: Input Lockout", 10, 12);
        return FALSE;
    }
    
    return TRUE;
}

