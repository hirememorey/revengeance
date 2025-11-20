#include "../inc/wrestler.h"
#include "../res/resources.h"

// Constants for "Game Feel"
#define WALK_SPEED      FIX32(1.5)
#define GRAPPLE_MAX_TIME 180

// Wrestler Size
#define WRESTLER_WIDTH  24 
#define WRESTLER_HEIGHT 32

// Frame Constants
#define FRAME_IDLE      0
#define FRAME_WALK_1    1
#define FRAME_WALK_2    2
#define FRAME_GRAPPLE   3
#define FRAME_THROWN    4
#define FRAME_GROUNDED  5

void initWrestler(Wrestler* w, fix32 startX, fix32 startY) {
    w->x = startX;
    w->y = startY;
    w->velX = FIX32(0);
    w->velY = FIX32(0);
    
    // Economy Init
    w->stamina = 100;
    w->heat = 0;
    w->bodyDamage = 0;
    w->stunValue = 0;
    w->mashCount = 0;

    w->state = STATE_IDLE;
    w->stateTimer = 0;
    w->facingRight = TRUE;
    w->bufferedInput = 0;
    w->lastInput = 0;
    
    // Initialize Hardware Sprite
    w->sprite = SPR_addSprite(&wrestler_sprite, fix32ToInt(w->x), fix32ToInt(w->y), TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
    // We are using Manual Frame control, so stop auto-animation
    SPR_setAnim(w->sprite, 0); 
}

void updateAnimation(Wrestler* w) {
    // Simple animation logic
    
    // Set facing (H-Flip)
    SPR_setHFlip(w->sprite, !w->facingRight);

    switch(w->state) {
        case STATE_IDLE:
            SPR_setFrame(w->sprite, FRAME_IDLE);
            break;
            
        case STATE_WALKING:
        case STATE_RUNNING:
            // Toggle between Walk 1 and Walk 2 every 10 frames
            {
                u32 tick = getTick();
                if ((tick >> 4) & 1) {
                    SPR_setFrame(w->sprite, FRAME_WALK_1);
                } else {
                    SPR_setFrame(w->sprite, FRAME_WALK_2);
                }
            }
            break;
        
        case STATE_GRAPPLE_INIT:
            // Use Grapple frame for the "Reach"
            SPR_setFrame(w->sprite, FRAME_GRAPPLE);
            break;
            
        case STATE_GRAPPLING:
            SPR_setFrame(w->sprite, FRAME_GRAPPLE);
            break;
            
        case STATE_THROWN:
            SPR_setFrame(w->sprite, FRAME_THROWN);
            break;
            
        case STATE_GROUNDED:
        case STATE_SELLING:
        case STATE_PINNED: // Lying down
            SPR_setFrame(w->sprite, FRAME_GROUNDED);
            break;
            
        case STATE_PINNING: // On top
            SPR_setFrame(w->sprite, FRAME_GRAPPLE); // Use grapple frame (kneeling-ish) for now
            break;

        case STATE_ATTACK_HEAVY:
            // Reuse Grapple frame for throw animation for now
            SPR_setFrame(w->sprite, FRAME_GRAPPLE);
            break;

        case STATE_ATTACK_LIGHT:
            // Use Walk 2 (arm out?) or Grapple for now
            SPR_setFrame(w->sprite, FRAME_GRAPPLE);
            break;

        case STATE_SUPLEX_EXECUTE:
        case STATE_SUPLEX_VICTIM:
        case STATE_PILEDRIVER_EXECUTE:
        case STATE_PILEDRIVER_VICTIM:
             // For now, reuse Grapple/Thrown frames until we add new art
             if (w->state == STATE_SUPLEX_EXECUTE || w->state == STATE_PILEDRIVER_EXECUTE) {
                 SPR_setFrame(w->sprite, FRAME_GRAPPLE);
             } else {
                 SPR_setFrame(w->sprite, FRAME_THROWN);
             }
             break;
             
        case STATE_WIN:
            SPR_setFrame(w->sprite, FRAME_GRAPPLE); // Arms up?
            break;
        case STATE_LOSE:
            SPR_setFrame(w->sprite, FRAME_GROUNDED);
            break;
            
        default:
            SPR_setFrame(w->sprite, FRAME_IDLE);
            break;
    }
}

void updateWrestler(Wrestler* w, u16 input) {
    // Detect pressed buttons (Rising Edge)
    u16 pressed = input & ~w->lastInput;
    w->lastInput = input;

    // 1. STATE MANAGEMENT
    switch(w->state) {
        case STATE_IDLE:
        case STATE_WALKING:
        case STATE_RUNNING:
            if (input & BUTTON_RIGHT) {
                w->velX = WALK_SPEED;
                w->facingRight = TRUE;
                w->state = STATE_WALKING;
            } else if (input & BUTTON_LEFT) {
                w->velX = -WALK_SPEED;
                w->facingRight = FALSE;
                w->state = STATE_WALKING;
            } else {
                w->velX = FIX32(0);
            }

            if (input & BUTTON_UP) {
                w->velY = -WALK_SPEED;
                w->state = STATE_WALKING;
            } else if (input & BUTTON_DOWN) {
                w->velY = WALK_SPEED;
                w->state = STATE_WALKING;
            } else {
                w->velY = FIX32(0);
            }

            if (w->velX == 0 && w->velY == 0) {
                w->state = STATE_IDLE;
            }
            break;

        case STATE_GRAPPLE_INIT:
            w->velX = FIX32(0);
            w->velY = FIX32(0);
            w->stateTimer++;
            break;

        case STATE_GRAPPLING:
            w->velX = FIX32(0);
            w->velY = FIX32(0);
            break;

        case STATE_THROWN:
            w->stateTimer++;
            if (w->stateTimer > 30) {
                w->state = STATE_GROUNDED;
                w->stateTimer = 0;
            }
            break;

        case STATE_GROUNDED:
            w->velX = FIX32(0);
            w->velY = FIX32(0);
            
            // Check for Active Sell (Holding C/Z)
            if (input & BUTTON_C) {
                w->state = STATE_SELLING;
                break;
            }

            w->stateTimer++;
            if (w->stateTimer > 60) {
                w->state = STATE_IDLE;
                w->stamina += 5; // Small recover on stand up
                if(w->stamina > 100) w->stamina = 100;
            }
            break;

        case STATE_SELLING:
            w->velX = FIX32(0);
            w->velY = FIX32(0);
            
            // Regenerate Stamina fast!
            w->stamina += 2; // +2 per frame = full bar in ~1 sec
            if (w->stamina > 100) w->stamina = 100;

            // If button released, go back to grounded -> idle flow
            if (!(input & BUTTON_C)) {
                w->state = STATE_GROUNDED;
                // Keep current timer so they don't get stuck forever
            }
            break;
            
        case STATE_PINNING:
            w->velX = FIX32(0);
            w->velY = FIX32(0);
            w->stateTimer++;
            // Exit/Win logic handled in main or via opponent kickout
            break;
            
        case STATE_PINNED:
            w->velX = FIX32(0);
            w->velY = FIX32(0);
            w->stateTimer++; // Used for Count (1-2-3)
            
            // Kickout Mechanic: Mashing (A, B, or C)
            if (pressed & (BUTTON_A | BUTTON_B | BUTTON_C)) {
                w->mashCount -= 5; // Reduce required mash
            }
            
            if (w->mashCount <= 0) {
                // KICKOUT!
                w->state = STATE_GROUNDED;
                w->stateTimer = 0;
                w->stamina += 10; // Adrenaline boost
            }
            break;

        case STATE_WIN:
        case STATE_LOSE:
            w->velX = FIX32(0);
            w->velY = FIX32(0);
            break;

        case STATE_ATTACK_HEAVY:
             w->stateTimer++;
            if (w->stateTimer > 20) {
                w->state = STATE_IDLE;
            }
            break;

        case STATE_SUPLEX_EXECUTE:
            w->velX = FIX32(0);
            w->velY = FIX32(0);
            w->stateTimer++;
            if (w->stateTimer > 60) {
                w->state = STATE_IDLE;
            }
            break;

        case STATE_SUPLEX_VICTIM:
            w->velX = FIX32(0);
            w->velY = FIX32(0);
            w->stateTimer++;
            if (w->stateTimer > 60) {
                w->state = STATE_GROUNDED; // Hard knock down
                w->stateTimer = 0;
            }
            break;

        case STATE_PILEDRIVER_EXECUTE:
            w->velX = FIX32(0);
            w->velY = FIX32(0);
            w->stateTimer++;
            if (w->stateTimer > 90) {
                w->state = STATE_IDLE;
            }
            break;

        case STATE_PILEDRIVER_VICTIM:
            w->velX = FIX32(0);
            w->velY = FIX32(0);
            w->stateTimer++;
            if (w->stateTimer > 90) {
                w->state = STATE_GROUNDED;
                w->stateTimer = 0;
            }
            break;

        case STATE_ATTACK_LIGHT:
            w->velX = FIX32(0);
            w->velY = FIX32(0);
            w->stateTimer++;
            if (w->stateTimer > 15) {
                w->state = STATE_IDLE;
            }
            break;
            
        case STATE_STUNNED:
             w->velX = FIX32(0);
             w->velY = FIX32(0);
             w->stateTimer++;
             if (w->stateTimer > w->stunValue) {
                 w->state = STATE_IDLE;
                 w->stunValue = 0;
             }
             break;

        default:
            break;
    }

    // 2. PHYSICS
    w->x = fix32Add(w->x, w->velX);
    w->y = fix32Add(w->y, w->velY);

    // 3. BOUNDARY
    if (w->x < FIX32(0)) w->x = FIX32(0);
    if (w->x > FIX32(288)) w->x = FIX32(288);
    if (w->y < FIX32(0)) w->y = FIX32(0);
    if (w->y > FIX32(192)) w->y = FIX32(192);
    
    // 4. UPDATE ANIMATION FRAME
    updateAnimation(w);
}

void drawWrestler(Wrestler* w) {
    SPR_setPosition(w->sprite, fix32ToInt(w->x), fix32ToInt(w->y));
}

Box getWrestlerBox(Wrestler* w) {
    Box b;
    b.x = fix32ToInt(w->x) + 4;
    b.y = fix32ToInt(w->y);
    b.w = WRESTLER_WIDTH;
    b.h = WRESTLER_HEIGHT;
    return b;
}

Box getHitbox(Wrestler* w) {
    Box b = {0, 0, 0, 0};
    
    // Only generating hitboxes in attack states
    if (w->state == STATE_ATTACK_LIGHT) {
        // Active Frames 4-8
        if (w->stateTimer >= 4 && w->stateTimer <= 8) {
            b.y = fix32ToInt(w->y) + 8;
            b.w = 16;
            b.h = 8;
            
            if (w->facingRight) {
                b.x = fix32ToInt(w->x) + 20;
            } else {
                b.x = fix32ToInt(w->x) - 10;
            }
        }
    }
    return b;
}

bool checkCollision(Box a, Box b) {
    return (a.x < b.x + b.w &&
            a.x + a.w > b.x &&
            a.y < b.y + b.h &&
            a.y + a.h > b.y);
}

void applyDamage(Wrestler* victim, s16 damage, s16 stun) {
    victim->stamina -= damage;
    victim->stunValue = stun;
    victim->state = STATE_STUNNED;
    victim->stateTimer = 0;
    
    // Clamp stamina
    if (victim->stamina < 0) victim->stamina = 0;
}
