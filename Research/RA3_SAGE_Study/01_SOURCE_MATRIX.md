#01. Source Matrix
## A. Official Generals / Zero Hour source code
Source: https://github.com/electronicarts/CnC_Generals_Zero_Hour
Status:

- official Electronic Arts repository;
- published under GPL-3.0 with additional EA terms;
- archived February 27, 2025;
- contains the `Generals` and `GeneralsMD` trees;
- is not a modern ready-made SDK: the original build is designed for Visual Studio 6 and depends on a number of old or proprietary libraries.
Utility:
- GameClient / GameLogic / GameNetwork / Common boundaries;
- object model and modular behavior;
- commands, synchronization frames and network transport;- AI, pathfinding, weapons, armor, scripting, victory conditions;
- saving game state and data infrastructure.
Limitation:
- the code cannot be copied into a closed commercial Unreal module;
- it is permissible to study ideas, boundaries of subsystems, invariants and algorithmic Requirements, after which write an independent implementation.
## B. Official C&C Modding Support
Source: https://github.com/electronicarts/CnC_Modding_Support
RA3: https://github.com/electronicarts/CnC_Modding_Support/tree/main/Red%20Alert%203

Status:

- official EA repository;
- archived August 15, 2025;
- for RA3 contains `Libraries`, `Schemas`, `Shaders`, `Xml`, `RA3Music.h`;
- RA3 does not contain complete C++ code.
Utility:
- formal XSD schemas;
- structure of game objects and their inheritance;
- Economy, dependencies and technological levels;
- commands, abilities, upgrades, AI and multiplayer settings;
- visual states, FX and shader feature taxonomy;
- campaign/skirmish scripts structure.
Limitation:
- this is not a free library of ready-made data for a commercial game;
- values, names, Factions, links to art and balance cannot be mechanically transferred to the product.
## C. Official EA Modding Rules
Source: https://www.ea.com/games/command-and-conquer/command-and-conquer-remastered/news/modding-faq
Critical points for the project:
- EA lists complete published GPL code only for Red Alert, Tiberian Dawn, Renegade, Generals and Zero Hour;
- mods with C&C materials must be free and non-commercial;
- you cannot create the impression of an official connection with EA;
- the use of trademarks, logos and music is limited;
- for a public mod, a disclaimer is required stating that EA does not support it.
## D. RA3 Mod SDK and art/campaign packs
Secondary directory: https://www.cnclabs.com/downloads/redalert3/modding-and-mapping.aspx
Utility:
- Mod SDK v3;
- World Builder;
- campaign source files;
- W3X/TGA/XML/3ds Max source packs;
- practical understanding of the old content pipeline.
Trust Status:
- use as navigation and mirror;
- check legal conclusions based on EA materials;
- binary downloads should not be included in the production repository without separate verification of origin.
## E. Active community fork GeneralsGameCode
Source: https://github.com/TheSuperHackers/GeneralsGameCode
Utility:
- modern Navigation using the old code;
- ported to Visual Studio 2022/C++20;
- discussions of determinism, replays, desync and cross-platform mathematics;
- useful for finding known problems in the original architecture.
Limitation:
- it is not a source of truth about original behavior;
- community fork changes must be verified with the official EA snapshot;
- GPL code is still not transferred to the closed Unreal runtime.