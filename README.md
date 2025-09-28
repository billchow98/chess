## Tuna Chess Engine

**Tuna** is a high-performance chess engine written in C++20, achieving **2505.9 ELO** rating in tournament play. Built from scratch with modern C++ practices, it features bitboard representation, alpha-beta search, and sophisticated evaluation functions.

### Key Features
- **Advanced Search**: Alpha-beta with transposition tables, quiescence search, and pruning techniques
- **Bitboard Engine**: PEXT bitboards for sliding piece attacks and efficient move generation
- **Automated Testing**: SPRT statistical validation and tournament management system
- **Modern Tooling**: Pixi dependency management, CMake/Ninja build system, automated formatting

### Tournament Results (10s+0.1s)
| Rank | Engine | Rating | Points | Played | Win % |
|------|--------|--------|--------|--------|-------|
| 1 | blunder 7.6.0 | 2618.0 | 1180.0 | 2000 | 59.0% |
| 2 | leorik 2.0.2 | 2536.0 | 1161.5 | 2000 | 58.1% |
| **3** | **tuna 1.0.0** | **2505.9** | **4343.5** | **8000** | **54.3%** |
| 4 | shenyu 2.0.1 | 2420.0 | 804.5 | 2000 | 40.2% |
| 5 | admete 1.3.0 | 2316.0 | 510.5 | 2000 | 25.5% |

### Quick Start
```bash
pixi run build    # Build the engine
pixi run start    # Run the engine
pixi run test     # Run test suite
```

### Technical Skills Demonstrated
- **Systems Programming**: Low-level bit manipulation, memory management, performance optimization
- **Algorithm Design**: Search algorithms, move generation, evaluation functions
- **DevOps**: Automated testing, tournament management, CI/CD pipeline
- **Software Architecture**: Clean modular design with UCI protocol support
