#include <genesis.h>
#include "../inc/wrestler.h"
#include "../res/resources.h"

// Include Test Runner Header
#ifdef TEST_BUILD
#include "../inc/test_runner.h"
#endif

// Global entities
Wrestler player1;
Wrestler player2;

u16 p1_last_input = 0;
u16 p2_last_input = 0;

// Sound ID constant
#define SFX_HIT_ID 64

// Timing Constants
#define GRAPPLE_SYNC_POINT 10
#define GRAPPLE_WINDOW_SIZE 15 // 15 frames to input (~250ms)

void handleCollisions(u16 p1_input, u16 p2_input) {
    // If either player is busy, no new collision logic
    if (player1.state != STATE_IDLE && player1.state != STATE_WALKING && player1.state != STATE_RUNNING) return;
    if (player2.state != STATE_IDLE && player2.state != STATE_WALKING && player2.state != STATE_RUNNING) return;

    Box b1 = getWrestlerBox(&player1);
    Box b2 = getWrestlerBox(&player2);
    
    if (checkCollision(b1, b2)) {
        // Transition to STATE_GRAPPLE_INIT (The Duel)
        // Position them properly facing each other
        if (player1.x < player2.x) {
            player1.facingRight = TRUE;
            player2.facingRight = FALSE;
            player2.x = fix32Add(player1.x, FIX32(16));
        } else {
            player1.facingRight = FALSE;
            player2.facingRight = TRUE;
            player2.x = fix32Sub(player1.x, FIX32(16));
        }

        player1.state = STATE_GRAPPLE_INIT;
        player2.state = STATE_GRAPPLE_INIT;
        player1.stateTimer = 0;
        player2.stateTimer = 0;
        player1.bufferedInput = 0;
        player2.bufferedInput = 0;

        return;
    }
}

// Resolve the RPS Logic + Stamina Check
void resolveGrappleResult() {
    // For now, simple winner check based on button hierarchy
    // Hierarchy: A (Light) > C (Heavy) > B (Medium) > A ...
    // Input: 0 if nothing pressed.
    // BUTTON_A = 0x0040, BUTTON_B = 0x0010, BUTTON_C = 0x0020
    
    u16 p1_in = player1.bufferedInput;
    u16 p2_in = player2.bufferedInput;
    
    Wrestler* winner = NULL;
    Wrestler* loser = NULL;

    // Default winner logic (placeholder for full RPS)
    // If P1 pressed anything and P2 didn't, P1 wins
    if (p1_in && !p2_in) {
        winner = &player1;
        loser = &player2;
    } else if (!p1_in && p2_in) {
        winner = &player2;
        loser = &player1;
    } else if (p1_in && p2_in) {
        // Clash! For now, coin flip or P1 advantage
        winner = &player1;
        loser = &player2;
    } else {
        // Both missed window -> Reset to neutral
        player1.state = STATE_IDLE;
        player2.state = STATE_IDLE;
        // Push apart slightly
        if(player1.x < player2.x) {
             player1.x = fix32Sub(player1.x, FIX32(10));
             player2.x = fix32Add(player2.x, FIX32(10));
        } else {
             player1.x = fix32Add(player1.x, FIX32(10));
             player2.x = fix32Sub(player2.x, FIX32(10));
        }
        return;
    }
    
    // STAMINA CHECK (The Soul)
    // If winner used Heavy (C), check loser stamina
    // BUTTON_C is usually mapped to 0x0020
    bool isHeavy = (winner == &player1) ? (p1_in & BUTTON_C) : (p2_in & BUTTON_C);
    
    if (isHeavy) {
        if (loser->stamina > 40) {
            // REVERSAL!
            VDP_drawText("REVERSED!", 12, 4);
            XGM_startPlayPCM(SFX_HIT_ID, 1, SOUND_PCM_CH2);
            
            // Swap roles
            Wrestler* temp = winner;
            winner = loser;
            loser = temp;
            
            // Attacker loses Heat for being cocky
            loser->heat -= 20; 
        }
    }

    // Apply Win
    winner->state = STATE_ATTACK_HEAVY; // Doing the move
    loser->state = STATE_THROWN;        // Taking the move
    
    // Physics impulse
    if (winner->x < loser->x) {
        loser->velX = FIX32(2);
        loser->velY = FIX32(-3);
    } else {
        loser->velX = FIX32(-2);
        loser->velY = FIX32(-3);
    }
    
    winner->heat += 15;
    loser->stamina -= 15;
    loser->bodyDamage += 5;

    XGM_startPlayPCM(SFX_HIT_ID, 1, SOUND_PCM_CH2);
}

void processGrappleLogic() {
    if (player1.state != STATE_GRAPPLE_INIT) return;
    
    // 1. The TELL (Visual/Audio Cue)
    if (player1.stateTimer == GRAPPLE_SYNC_POINT) {
        VDP_drawText("!", 19, 10); // The visual "Spark"
        // Play a light click or sync sound here
    }
    
    // 2. The Window
    // Close window after duration
    if (player1.stateTimer > (GRAPPLE_SYNC_POINT + GRAPPLE_WINDOW_SIZE)) {
         VDP_clearText(19, 10, 1);
         resolveGrappleResult();
         return;
    }

    // 3. Buffer Inputs (Only look for first press in window)
    // Note: We are reading current inputs from main loop, but we need 'pressed' this frame?
    // For simplicity, we'll read the global 'pressed' vars passed or read joypad directly.
    // But here we are in a function. Let's assume we process this AFTER updateWrestler 
    // and we need to know what was pressed.
    
    // We'll rely on the main loop to pass input or store it in wrestler.
    // Actually, let's refactor main loop slightly to pass input to this function 
    // OR store "just pressed" in the wrestler struct for this frame.
}

int main() {
    SPR_init();
    
    XGM_setPCM(SFX_HIT_ID, sfx_hit, sizeof(sfx_hit));
    
    VDP_setPalette(PAL1, ring_bg.palette->data);
    int ind = TILE_USERINDEX;
    VDP_drawImageEx(BG_B, &ring_bg, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, ind), 0, 0, FALSE, TRUE);
    ind += ring_bg.tileset->numTile; 

    PAL_setPalette(PAL0, wrestler_sprite.palette->data, DMA);

    JOY_init();
    
    initWrestler(&player1, FIX32(100), FIX32(120)); 
    initWrestler(&player2, FIX32(200), FIX32(120));
    player2.facingRight = FALSE;

    // === TEST HARNESS ENTRY POINT ===
    #ifdef TEST_BUILD
    VDP_drawText("=== TEST MODE ===", 10, 1);
    bool success = run_all_tests();
    if (success) {
        VDP_drawText("ALL TESTS PASSED", 10, 20);
        VDP_setBackgroundColor(8); // Green-ish
    } else {
        VDP_drawText("TESTS FAILED", 10, 20);
        VDP_setBackgroundColor(2); // Red
    }
    while(1) {
        SYS_doVBlankProcess();
    }
    #endif
    // ================================

    char debugStr[40];

    while(1) {
        u16 p1_current = JOY_readJoypad(JOY_1);
        u16 p2_current = JOY_readJoypad(JOY_2);

        u16 p1_pressed = p1_current & ~p1_last_input;
        u16 p2_pressed = p2_current & ~p2_last_input;
        
        p1_last_input = p1_current;
        p2_last_input = p2_current;

        // Input Buffering for Grapple Phase
        if (player1.state == STATE_GRAPPLE_INIT) {
            if (player1.stateTimer >= GRAPPLE_SYNC_POINT) {
                // Only buffer A/B/C
                if (p1_pressed & (BUTTON_A | BUTTON_B | BUTTON_C)) {
                    player1.bufferedInput = p1_pressed;
                }
                if (p2_pressed & (BUTTON_A | BUTTON_B | BUTTON_C)) {
                    player2.bufferedInput = p2_pressed;
                }
            } else {
                // Pressed TOO EARLY (False Start)
                if (p1_pressed & (BUTTON_A | BUTTON_B | BUTTON_C)) {
                    player1.state = STATE_STUNNED; // Punishment
                    // Ideally give advantage to P2 immediately
                }
            }
        }

        // Pass 'current' for movement, 'pressed' is handled by buffering logic above
        updateWrestler(&player1, p1_current);
        updateWrestler(&player2, p2_current); 

        handleCollisions(p1_current, p2_current);
        
        // Run the duel logic
        processGrappleLogic();

        // HUD / Debug
        sprintf(debugStr, "P1 STM:%3d P2 STM:%3d", player1.stamina, player2.stamina);
        VDP_drawText(debugStr, 1, 1);
        
        if(player1.state == STATE_SELLING) VDP_drawText("P1 SELLING", 1, 2);
        else VDP_clearText(1, 2, 10);

        drawWrestler(&player1);
        drawWrestler(&player2);
        
        SPR_update();
        SYS_doVBlankProcess();
    }
    return (0);
}
