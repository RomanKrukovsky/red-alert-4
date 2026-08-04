# Build System & Automated Test Audit (`BUILD_AND_TEST_AUDIT.md`)

**Audit Date**: August 4, 2026  
**Total Automated Tests**: 378  
**Pass Rate**: 100% (378 passed, 0 failed)  
**Execution Time**: 5.616 seconds  

---

## 1. Dual Build System Architecture

The project maintains two parallel build pipelines:

```
                                  +-----------------------+
                                  |   C++ SOURCE CODE     |
                                  |   (Source/ Directory) |
                                  +-----------------------+
                                        /           \
                                       /             \
                                      v               v
                +---------------------------+   +---------------------------+
                |   UNREAL BUILD TOOL (UBT) |   |  CMAKE HEADLESS BUILD     |
                |   Unreal Engine Editor    |   |  Tools/HeadlessBuild/     |
                |   Target.cs & Build.cs    |   |  UnrealStub / C++ Static  |
                +---------------------------+   +---------------------------+
                              |                               |
                              v                               v
                      [Unreal Editor]                 [4 Test Executables]
                      [Packaged Game]                 378/378 PASS (5.6s)
```

---

## 2. Test Executable Breakdown

| Executable | Source Directory | Test Count | Pass Rate | Execution Time | Purpose |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `RA4Tests` | `Source/RA4Tests` | 258 | 100% | 5.616s | Sim engine, Lockstep, Bible content, Save/Restore, CommandBus, Campaign. |
| `RA4AITests` | `Source/RA4AI` | 46 | 100% | 4.826s | AICommander strategy loop, squad formations, difficulty profiles, fog compliance. |
| `RA4InputTests` | `Source/RA4Input` | 51 | 100% | 0.004s | WASD camera controls, marquee selection, control groups, attack-move commands. |
| `RA4PresentationTests` | `Source/RA4Presentation` | 23 | 100% | 0.010s | HUD ViewModel snapshot builder, resource bar deltas, selection portraits. |
| **TOTAL** | | **378** | **100%** | **5.616s** | Complete C++ Headless Verification |

---

## 3. Key Test Cases & Verification Highlights

### High-Stress Simulation Test
- `ProvingGround.HeadlessStressScenario500Entities`: Simulates 500 active units and structures across 1000 ticks in headless mode. Passes deterministically in 433 milliseconds.

### Lockstep & Desync Catching
- `Lockstep.DesyncIsCaughtOnTheTickItHappens`: Deliberately injects a divergent random seed into one client tick frame; verifies that `LockstepSession` immediately halts execution and reports desync on exact tick index.

### AI Match Completion
- `AI.FiveSkirmishScenariosFinishWithAWinner`: Runs 5 complete 1v1 skirmish matches to victory/defeat completion deterministically.

---

## 4. CI Workflow Integration (`.github/workflows/ci.yml`)

The GitHub Actions workflow executes:
1. `cmake -B build/hb -S Tools/HeadlessBuild`
2. `cmake --build build/hb -j8`
3. `./build/hb/RA4Tests` (executed from repo root)
4. `./build/hb/RA4AITests`
5. `./build/hb/RA4InputTests`
6. `./build/hb/RA4PresentationTests`
7. `cd ra4-ui && npm ci && npm run build`

---

## 5. Build Command Reference

- **Run All Headless Tests (Local Terminal)**:
  ```bash
  ./build/hb/RA4Tests && ./build/hb/RA4AITests && ./build/hb/RA4InputTests && ./build/hb/RA4PresentationTests
  ```
- **Rebuild Headless Suite from Scratch**:
  ```bash
  cmake -B build/hb -S Tools/HeadlessBuild && cmake --build build/hb -j8
  ```
