#ifndef _AI_H_
#define _AI_H_

#include <genesis.h>
#include "wrestler.h"

// AI Difficulty Levels
typedef enum {
    AI_EASY,
    AI_MEDIUM,
    AI_HARD
} AIDifficulty;

// AI State (Internal thought process)
typedef enum {
    AI_THINKING,
    AI_APPROACHING,
    AI_FLEEING,
    AI_ATTACKING,
    AI_WAITING_FOR_GRAPPLE
} AIState;

typedef struct {
    Wrestler* self;
    Wrestler* opponent;
    AIDifficulty difficulty;
    AIState currentState;
    u16 reactionTimer; // Delays action to simulate human reaction time
    u16 desiredInput;  // The input the AI wants to press this frame
} AIController;

void initAI(AIController* ai, Wrestler* self, Wrestler* opponent, AIDifficulty diff);
u16 updateAI(AIController* ai);

#endif // _AI_H_

