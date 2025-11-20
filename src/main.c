#include <genesis.h>
#include "../inc/wrestler.h"
#include "../inc/ai.h"
#include "../res/resources.h"

// Include Test Runner Header
#ifdef TEST_BUILD
#include "../inc/test_runner.h"
#endif

// Global entities
Wrestler player1;
Wrestler player2;
    AIController aiPlayer2;

u16 p1_last_input = 0;
u16 p2_last_input = 0;

// Sound ID constant
#define SFX_HIT_ID 64

// Timing Constants
#define GRAPPLE_SYNC_POINT 10
#define GRAPPLE_WINDOW_SIZE 15 // 15 frames to input (~250ms)

// Handle Striking Logic (Hitbox vs Hurtbox)
void handleStrikes() {
    bool p1Hits = FALSE;
    bool p2Hits = FALSE;

    // 1. Detection Phase
    // Check P1 Attacking P2
    Box hit1 = getHitbox(&player1);
    if (hit1.w > 0) { 
        Box hurt2 = getWrestlerBox(&player2);
        if (checkCollision(hit1, hurt2)) {
             // Only hit if P2 is not already stunned/thrown
             if (player2.state != STATE_STUNNED && player2.state != STATE_THROWN && player2.state != STATE_GRAPPLE_INIT) {
                 p1Hits = TRUE;
             }
        }
    }

    // Check P2 Attacking P1
    Box hit2 = getHitbox(&player2);
    if (hit2.w > 0) {
        Box hurt1 = getWrestlerBox(&player1);
        if (checkCollision(hit2, hurt1)) {
             if (player1.state != STATE_STUNNED && player1.state != STATE_THROWN && player1.state != STATE_GRAPPLE_INIT) {
                 p2Hits = TRUE;
             }
        }
    }

    // 2. Resolution Phase
    if (p1Hits) {
         applyDamage(&player2, 5, 30); // 5 dmg, 30 frame stun
         XGM_startPlayPCM(SFX_HIT_ID, 1, SOUND_PCM_CH2);
         VDP_drawText("HIT!", 10, 4);
    }

    if (p2Hits) {
         applyDamage(&player1, 5, 30);
         // If both hit, the sound might just retrigger, which is acceptable for now
         XGM_startPlayPCM(SFX_HIT_ID, 1, SOUND_PCM_CH2);
         VDP_drawText("HIT!", 20, 4);
    }
}

void handleCollisions(u16 p1_input, u16 p2_input) {
    // If either player is busy, no new collision logic
    // Exception: We allow collisions if moving, but NOT if attacking (unless we want attacks to stuff grapples?)
    // Let's say if you are attacking, you cannot be grappled easily (or maybe you can?)
    // For now: If attacking, ignore grapple collision
    if (player1.state == STATE_ATTACK_LIGHT || player2.state == STATE_ATTACK_LIGHT) return;

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
    
    // Determine Move based on Button
    if (isHeavy) {
        winner->state = STATE_PILEDRIVER_EXECUTE;
        loser->state = STATE_PILEDRIVER_VICTIM;
        
        winner->heat += 25;
        loser->stamina -= 25;
        loser->bodyDamage += 10;
    } else {
        // Medium (B) or Light (A) -> Suplex (for now)
        // Ideally A could be a simple irish whip or body slam, B suplex
        winner->state = STATE_SUPLEX_EXECUTE;
        loser->state = STATE_SUPLEX_VICTIM;
        
        winner->heat += 15;
        loser->stamina -= 15;
        loser->bodyDamage += 5;
    }
    
    winner->stateTimer = 0;
    loser->stateTimer = 0;
    
    // Reset Physics
    winner->velX = FIX32(0); winner->velY = FIX32(0);
    loser->velX = FIX32(0); loser->velY = FIX32(0);

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
    
    // Initialize AI
    initAI(&aiPlayer2, &player2, &player1, AI_MEDIUM);

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
        
        // AI Update
        u16 p2_current = updateAI(&aiPlayer2);
        // Allow P2 Controller override for debug/2P mode if needed (Press Start?)
        if (JOY_readJoypad(JOY_2) & BUTTON_START) {
             p2_current = JOY_readJoypad(JOY_2);
        }

        u16 p1_pressed = p1_current & ~p1_last_input;
        u16 p2_pressed = p2_current & ~p2_last_input;
        
        p1_last_input = p1_current;
        p2_last_input = p2_current;

        // 0. Neutral Input Handling (Strikes)
        // Only if IDLE/WALKING
        if ((player1.state == STATE_IDLE || player1.state == STATE_WALKING) && (p1_pressed & BUTTON_A)) {
            player1.state = STATE_ATTACK_LIGHT;
            player1.stateTimer = 0;
        }
        if ((player2.state == STATE_IDLE || player2.state == STATE_WALKING) && (p2_pressed & BUTTON_A)) {
            player2.state = STATE_ATTACK_LIGHT;
            player2.stateTimer = 0;
        }
        
        // 0.1 Pin Initiation
        // P1 attempts pin on P2
        if ((player1.state == STATE_IDLE || player1.state == STATE_WALKING) && 
            (player2.state == STATE_GROUNDED || player2.state == STATE_SELLING)) {
            
            int dist = fix32ToInt(player1.x) - fix32ToInt(player2.x);
            if(dist < 0) dist = -dist;

            if ((p1_pressed & BUTTON_A) && dist < 24) {
                player1.state = STATE_PINNING;
                player2.state = STATE_PINNED;
                player1.stateTimer = 0;
                player2.stateTimer = 0;
                
                // Calculate Mash Difficulty
                // Base 30 + Damage. Less Stamina = Harder.
                // mashCount = 30 + (100 - stamina) + damage/2
                player2.mashCount = 30 + (100 - player2.stamina) + (player2.bodyDamage >> 1);
                
                VDP_drawText("PIN!", 15, 10);
            }
        }

        // P2 attempts pin on P1
        if ((player2.state == STATE_IDLE || player2.state == STATE_WALKING) && 
            (player1.state == STATE_GROUNDED || player1.state == STATE_SELLING)) {
            
            int dist = fix32ToInt(player2.x) - fix32ToInt(player1.x);
            if(dist < 0) dist = -dist;

            if ((p2_pressed & BUTTON_A) && dist < 24) {
                player2.state = STATE_PINNING;
                player1.state = STATE_PINNED;
                player2.stateTimer = 0;
                player1.stateTimer = 0;
                
                player1.mashCount = 30 + (100 - player1.stamina) + (player1.bodyDamage >> 1);
                
                VDP_drawText("PIN!", 15, 10);
            }
        }

        // 0.2 Pin Resolution (Kickout or Win)
        if (player1.state == STATE_PINNING) {
            // Check if P2 broke out (State changed to GROUNDED in updateWrestler)
            if (player2.state == STATE_GROUNDED) {
                player1.state = STATE_STUNNED; // Pushed off
                player1.stateTimer = 0;
                player1.stunValue = 20; // Brief stun
                VDP_drawText("KICKOUT!", 15, 10);
                VDP_clearText(15, 12, 10); // Clear count
            } else {
                // Count Logic
                // 60 frames = 1 sec.
                if (player1.stateTimer == 60) VDP_drawText("ONE!  ", 15, 12);
                if (player1.stateTimer == 120) VDP_drawText("TWO!  ", 15, 12);
                if (player1.stateTimer == 180) {
                    VDP_drawText("THREE!", 15, 12);
                    player1.state = STATE_WIN;
                    player2.state = STATE_LOSE;
                    VDP_drawText("WINNER: P1", 12, 14);
                }
            }
        }
        
        if (player2.state == STATE_PINNING) {
            if (player1.state == STATE_GROUNDED) {
                player2.state = STATE_STUNNED; 
                player2.stateTimer = 0;
                player2.stunValue = 20;
                VDP_drawText("KICKOUT!", 15, 10);
                VDP_clearText(15, 12, 10);
            } else {
                if (player2.stateTimer == 60) VDP_drawText("ONE!  ", 15, 12);
                if (player2.stateTimer == 120) VDP_drawText("TWO!  ", 15, 12);
                if (player2.stateTimer == 180) {
                    VDP_drawText("THREE!", 15, 12);
                    player2.state = STATE_WIN;
                    player1.state = STATE_LOSE;
                    VDP_drawText("WINNER: P2", 12, 14);
                }
            }
        }

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
        
        // Clear Pin Text if no longer pinning
        if (player1.state != STATE_PINNING && player2.state != STATE_PINNING && 
            player1.state != STATE_WIN && player2.state != STATE_WIN && 
            player1.stateTimer == 0 && player2.stateTimer == 0) { // HACK: Check timers to avoid clearing immediately on kickout frame
             // Actually, simpler: if state transitioned, clear after some time. 
             // For now, let's leave it or clear it when state is IDLE.
        }
        
        if (player1.state == STATE_IDLE && player2.state == STATE_IDLE) {
             // Reset text occasionally? 
             // VDP_clearText(15, 10, 10);
             // VDP_clearText(15, 12, 10);
        }

        handleCollisions(p1_current, p2_current);
        
        handleStrikes();

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
