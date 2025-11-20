# Guiding Vision: "Squared Circle: Genesis"

## The Thesis
Wrestling is not a fight; it is a dramatic performance of a fight. Therefore, the game mechanics must reward **Pacing, Psychology, and Rhythm**, not just reflex speed.

## 1. The Core Loop: The "Momentum Economy"
We have removed the traditional Health Bar. Instead, the game is driven by three variables:

*   **Stamina (Invisible/Debug)**: Short-term energy. Depletes when running or executing moves.
*   **Heat (Visible)**: Momentum. Gains on successful moves, taunts, or "2.9" kickouts.
*   **Body Damage (Hidden)**: Long-term wear on specific limbs.

**Implemented Mechanics**:
*   **Stamina Regen**: Standing still regenerates slowly.
*   **Active Selling**: Holding 'C' while `STATE_GROUNDED` regenerates Stamina at **2x speed**. This forces a strategic choice: "Stay down to heal" vs "Get up to fight."

## 2. The "Duel": Timing > Mashing
We reject button mashing. Grappling is a "High Noon" duel.

**The Protocol**:
1.  **Collision**: Wrestlers enter `STATE_GRAPPLE_INIT` (The Reach).
2.  **The Tell**: A visual "Spark" (!) appears at Frame 10.
3.  **The Input**:
    *   **Early**: Punished (Stun).
    *   **Perfect**: Input is buffered.
    *   **Late**: Whiff.

## 3. The Hierarchy (Rock-Paper-Scissors)
*   **Light (A)**: Interrupts Heavy.
*   **Medium (B)**: Safe, reliable.
*   **Heavy (C)**: High Damage, High Risk.
    *   *The Soul Rule*: If you attempt a Heavy Move on an opponent with >40% Stamina, they will **Auto-Reverse** you. You must "work the body" first.

## Technical Pillars
To achieve this vision on the Sega Genesis:
1.  **Deterministic FSM**: No "floating" logic. Every frame belongs to a specific State.
2.  **Resource-Driven**: Every action has a Stamina cost.
3.  **Verified Logic**: All mechanics are backed by the `src/test_runner.c` test suite to ensure the math aligns with the vision.
