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

    w->state = STATE_IDLE;
    w->stateTimer = 0;
    w->facingRight = TRUE;
    w->bufferedInput = 0;
    
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
            // We use a global timer or the wrestler's state timer if it resets?
            // Let's use SYS_getCounter() or just a simple modulo on a global tick.
            // Actually, w->stateTimer isn't reliable for walking if we don't reset it.
            // Let's use the low bits of x/y position or a simple counter.
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
            SPR_setFrame(w->sprite, FRAME_GROUNDED);
            break;

        case STATE_ATTACK_HEAVY:
            // Reuse Grapple frame for throw animation for now
            SPR_setFrame(w->sprite, FRAME_GRAPPLE);
            break;
            
        default:
            SPR_setFrame(w->sprite, FRAME_IDLE);
            break;
    }
}

void updateWrestler(Wrestler* w, u16 input) {
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
            // Input buffering logic will be handled in main or a specialized function
            // For now, just track timer
            w->stateTimer++;
            break;

        case STATE_GRAPPLING:
            w->velX = FIX32(0);
            w->velY = FIX32(0);
            // Logic moved to resolveGrapple in main for now
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
            // Assuming BUTTON_C is mapped to sell for 3-button controller compatibility
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

        case STATE_ATTACK_HEAVY:
            // Throwing someone
             w->stateTimer++;
            if (w->stateTimer > 20) {
                w->state = STATE_IDLE;
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

bool checkCollision(Box a, Box b) {
    return (a.x < b.x + b.w &&
            a.x + a.w > b.x &&
            a.y < b.y + b.h &&
            a.y + a.h > b.y);
}
