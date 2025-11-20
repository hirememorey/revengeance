# Revengeance: Sega Genesis Wrestling Game

## Project Overview
**Codename**: "Squared Circle: Genesis"
**Platform**: Sega Genesis / Mega Drive (Motorola 68000)
**Engine**: SGDK (Sega Genesis Development Kit)

Revengeance is a wrestling simulation that prioritizes **psychology, pacing, and rhythm** over arcade reflexes. It is built on a custom Finite State Machine (FSM) engine that enforces wrestling rules through gameplay mechanics.

## First Principles Architecture
This project avoids "spaghetti code" by adhering to strict architectural principles:
1.  **Finite State Machine (FSM)**: Entities (`Wrestler`) are always in a discrete state (e.g., `STATE_IDLE`, `STATE_GRAPPLING`, `STATE_SELLING`).
2.  **Fixed-Point Math**: All physics use `fix32` to accommodate the Genesis hardware (no FPU).
3.  **Separation of Concerns**: 
    *   `inc/wrestler.h`: Data structures and constants.
    *   `src/wrestler.c`: State transitions and entity logic.
    *   `src/main.c`: The Game Loop, Input Handling, and Rendering.
    *   `src/test_runner.c`: Headless simulation logic for verification.

## Building the Project
We use **Docker** to ensure a consistent build environment without polluting your local machine.

### Prerequisites
*   [Docker Desktop](https://www.docker.com/products/docker-desktop/)

### Build Commands
The `build.sh` script handles all interactions with the Docker container.

**1. Build the Game ROM**
```bash
./build.sh
```
Output: `out/rom.bin` (Playable in BlastEm, Kega Fusion, RetroArch)

**2. Run the Test Suite**
```bash
./build.sh test
```
Output: `out/rom.bin` (Booting this ROM runs the automated test harness)

## The Test Harness
We maintain a **Headless Integration Test System** to verify game mechanics without manual playtesting.

*   **How it works**: The build script compiles with `-DTEST_BUILD`. `main.c` detects this flag and intercepts the boot process to run `src/test_runner.c`.
*   **Visual Feedback**:
    *   **Green Screen**: All tests passed.
    *   **Red Screen**: A test failed (Error message displayed).
*   **Current Tests**:
    1.  **Momentum Init**: Verifies correct starting stats.
    2.  **Selling Mechanic**: Verifies stamina regeneration math over time (handling FSM transition delays).
    3.  **Grapple Timing**: Verifies the input buffer window and "Spark" timing logic.

## Roadmap & Mechanics
*   [x] **Core Engine**: 60FPS loop, Sprite rendering, Input handling.
*   [x] **Momentum Economy**: Stamina, Heat, and Body Damage variables.
*   [x] **The "Duel"**: Timing-based grapple initiation (Rock-Paper-Scissors foundation).
*   [x] **Active Selling**: Hold 'C' on the ground to regenerate stamina.
*   [x] **Hitboxes**: AABB Collision for strikes.
*   [x] **Grapple Moves**: Implementing the actual throws (Suplex, Piledriver).
*   [ ] **Pin System**: Implementing the 1-2-3 count.
*   [ ] **AI**: Basic CPU opponent logic.
