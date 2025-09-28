## Tuna Chess Engine

**Tuna** is a high-performance chess engine written in C++20, designed with a focus on maintainability, correctness, and simplicity while achieving competitive playing strength. The engine implements modern chess programming techniques including bitboard representation, advanced search algorithms, and sophisticated evaluation functions.

Built from scratch using modern C++ practices, Tuna features a clean, modular architecture that prioritizes code clarity and maintainability. The engine supports the Universal Chess Interface (UCI) protocol and has been tested against established engines in tournament play, achieving a rating of **2505.9 ELO** in blitz time controls (10s+0.1s).

This project demonstrates expertise in:
- **Systems Programming**: Low-level bit manipulation, memory management, and performance optimization
- **Algorithm Design**: Implementation of alpha-beta search, transposition tables, and move generation algorithms
- **Software Architecture**: Clean separation of concerns with modular design patterns
- **Performance Engineering**: Bitboard techniques, move ordering heuristics, and search optimizations

### Features
#### Move generation
  - Evasions, captures, and quiet moves generator
    - Quiet queen promotions are considered with captures
  - Fancy PEXT bitboards for sliding attacks
  - Generates pseudo-legal moves
    - Moves are only checked for legality when played
  - Move sorting
    - Captures and queen promotions sorted by MVV-LVA
    - Two killer move slots for quiet moves
    - Quiet moves sorted by butterfly history heuristic
      - Bonus for quiet move that failed high
      - Malus for all previous quiet moves
#### Search
  - Transposition table
    - Depth-preferred replacement strategy
  - Iterative deepening
  - Aspiration windows
  - Alpha-beta search
  - Quiescence search
    - Only checks for evasions, captures, and quiet queen promotions
  - Principal variation search
  - Check extensions
  - Reverse futility pruning
  - Null move pruning
  - Late move reductions
  - Late move pruning
#### Evaluation
  - Tuned PeSTO's Evaluation Function piece-square tables
  - Tapered evaluation
  - Incremental update of piece-square table material
#### Time management
  - Run-time branching factor estimation to decide whether to search next depth

#### Tournament results (10s+0.1s) (ELO anchored to CCRL Blitz ratings 20250503)
|   #   |   PLAYER              |   RATING   |   POINTS   |   PLAYED   |   (%)     |
|-------|-----------------------|------------|------------|------------|-----------|
|   1   |   blunder 7.6.0       |   2618.0   |   1180.0   |   2000     |   59.0%   |
|   2   |   leorik 2.0.2        |   2536.0   |   1161.5   |   2000     |   58.1%   |
| **3** | **tuna 1.0.0**        | **2505.9** | **4343.5** | **8000**   | **54.3%** |
|   4   |   shenyu 2.0.1        |   2420.0   |    804.5   |   2000     |   40.2%   |
|   5   |   admete 1.3.0        |   2316.0   |    510.5   |   2000     |   25.5%   |
