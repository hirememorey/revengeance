#include "../inc/ai.h"

// Helper for random range
u16 randomRange(u16 min, u16 max) {
    return (random() % (max - min + 1)) + min;
}

void initAI(AIController* ai, Wrestler* self, Wrestler* opponent, AIDifficulty diff) {
    ai->self = self;
    ai->opponent = opponent;
    ai->difficulty = diff;
    ai->currentState = AI_THINKING;
    ai->reactionTimer = 0;
    ai->desiredInput = 0;
}

u16 updateAI(AIController* ai) {
    ai->desiredInput = 0; // Reset input
    
    // If AI is busy (stunned, executing move, etc), do nothing but maybe mash
    if (ai->self->state != STATE_IDLE && 
        ai->self->state != STATE_WALKING && 
        ai->self->state != STATE_RUNNING &&
        ai->self->state != STATE_GRAPPLE_INIT && 
        ai->self->state != STATE_PINNED) {
        return 0;
    }

    // Mashing Logic (Kickout)
    if (ai->self->state == STATE_PINNED) {
        // Mash rate depends on difficulty
        // Easy: Every 10 frames, Hard: Every 4 frames
        u16 mask = (ai->difficulty == AI_HARD) ? 3 : (ai->difficulty == AI_MEDIUM ? 7 : 15);
        
        if ((getTick() & mask) == 0) {
            ai->desiredInput = BUTTON_A; // Mash A
        }
        return ai->desiredInput;
    }

    // Grapple Logic (The Duel)
    if (ai->self->state == STATE_GRAPPLE_INIT) {
        // Wait for sync point + reaction time
        // Sync point is 10.
        // Perfect timing is 10.
        // Reaction delay: Easy=15, Medium=5, Hard=0 (Perfect)
        
        u16 targetFrame = 10;
        u16 reaction = 0;
        
        switch(ai->difficulty) {
            case AI_EASY: reaction = 15; break;
            case AI_MEDIUM: reaction = 5; break;
            case AI_HARD: reaction = 0; break;
        }
        
        // If we are in the window (10 to 25)
        if (ai->self->stateTimer >= (targetFrame + reaction)) {
            // Choose move based on Stamina
            // >40 Stamina: Use Heavy (C)
            // <40 Stamina: Use Medium (B) to be safe
            // <20 Stamina: Use Light (A)
            
            // Only press ONCE (Pulse)
            if (ai->desiredInput == 0) {
                if (ai->self->stamina > 40) ai->desiredInput = BUTTON_C;
                else if (ai->self->stamina > 20) ai->desiredInput = BUTTON_B;
                else ai->desiredInput = BUTTON_A;
            }
        }
        return ai->desiredInput;
    }

    // Neutral Game (Movement & Strikes)
    
    // Distance check
    int dist = fix32ToInt(ai->opponent->x) - fix32ToInt(ai->self->x);
    int distY = fix32ToInt(ai->opponent->y) - fix32ToInt(ai->self->y);
    bool closeX = (dist > -24 && dist < 24);
    bool closeY = (distY > -10 && distY < 10);
    
    if (closeX && closeY) {
        // Attack Range
        if (ai->reactionTimer == 0) {
            // 30% chance to attack, 70% chance to wait/reposition
            if (randomRange(0, 100) < 30) {
                ai->desiredInput = BUTTON_A; // Light Strike
                ai->reactionTimer = 20; // Cooldown
            }
        } else {
            ai->reactionTimer--;
        }
    } else {
        // Approach
        if (dist > 0) ai->desiredInput |= BUTTON_RIGHT;
        else ai->desiredInput |= BUTTON_LEFT;
        
        if (distY > 0) ai->desiredInput |= BUTTON_DOWN;
        else ai->desiredInput |= BUTTON_UP;
    }

    return ai->desiredInput;
}

