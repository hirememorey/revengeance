#ifndef _WRESTLER_H_
#define _WRESTLER_H_

#include <genesis.h>

// FIRST PRINCIPLE: Finite State Machine
// A wrestler can only be in one state at a time.
typedef enum {
    STATE_IDLE,
    STATE_WALKING,
    STATE_RUNNING,
    
    // Combat States
    STATE_ATTACK_LIGHT, // Quick strikes
    STATE_ATTACK_HEAVY, // Slower, stronger strikes
    
    // The Duel
    STATE_GRAPPLE_INIT, // The "Reach" / Timing Window
    STATE_GRAPPLING,    // Locked up (Result of winning the timing)
    
    // Move States (Execution & Victim)
    STATE_SUPLEX_EXECUTE,
    STATE_SUPLEX_VICTIM,
    STATE_PILEDRIVER_EXECUTE,
    STATE_PILEDRIVER_VICTIM,

    // Damage States
    STATE_THROWN,       // Being tossed
    STATE_GROUNDED,     // Lying on mat (can mash to rise)
    STATE_SELLING,      // Holding button to rest (fast regen)
    STATE_STUNNED,      // Standing but groggy (vulnerable)
    STATE_PINNED        // Shoulders down
} WrestlerState;

// FIRST PRINCIPLE: Entity Structure
// We use fixed-point math (fix32) for position because the Genesis
// has no Floating Point Unit (FPU). Floats are too slow.
typedef struct {
    // Physics
    fix32 x;
    fix32 y;
    fix32 velX;
    fix32 velY;
    
    // The Momentum Economy (Resources)
    // We use s16 for these to allow simple arithmetic. 
    // Max values ~100 for readability.
    s16 stamina;        // Energy. Depletes on moves/running. Regens on rest.
    s16 heat;           // Momentum. Gains on successful moves.
    s16 bodyDamage;     // Long-term wear. Reduces max speed/stamina.
    s16 stunValue;      // Current "dizziness". If high, you are vulnerable.

    // State Management
    WrestlerState state;
    u16 stateTimer;     // Frames spent in current state (useful for animation/stun)
    
    // Hardware
    Sprite* sprite;     // Pointer to hardware sprite
    bool facingRight;
    
    // Input Buffer for Timing
    // We capture the input pressed *during* the specific window
    u16 bufferedInput;  
} Wrestler;

// Collision Box - Using SGDK's built-in Box struct
// typedef struct {
//     s16 x;
//     s16 y;
//     s16 w;
//     s16 h;
// } Box;

// Function Prototypes
void initWrestler(Wrestler* w, fix32 startX, fix32 startY);
void updateWrestler(Wrestler* w, u16 input);
void drawWrestler(Wrestler* w);

// Collision helpers
Box getWrestlerBox(Wrestler* w);
Box getHitbox(Wrestler* w);
bool checkCollision(Box a, Box b);

// Interaction helpers
void applyDamage(Wrestler* victim, s16 damage, s16 stun);

#endif // _WRESTLER_H_
