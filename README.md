# Gödel's Sky

*A fork of Endless Sky with meaningful consequences*

Explore other star systems. Earn money by trading, carrying passengers, or completing missions. Use your earnings to buy a better ship or to upgrade the weapons and engines on your current one. Blow up pirates. Take sides in a civil war. Or leave human space behind and hope to find some friendly aliens whose culture is more civilized than your own...

**But beware: in Gödel's Sky, every action has consequences.**

<img width="800" height="464" alt="image" src="https://github.com/user-attachments/assets/e0223f58-8550-448d-867f-4be906bffe49" />

------

Gödel's Sky is a sandbox-style space exploration game forked from [Endless Sky](https://github.com/endless-sky/endless-sky). It maintains all the original gameplay while adding deep consequence systems that make the universe feel more alive and responsive to your choices.

## New Features in Gödel's Sky

### Action Memory System
Governments now remember your past actions. Destroyed ships, killed crew, and damaged property are tracked over time. Pattern detection identifies whether you're acting as a trader, pirate, bounty hunter, protector, warmonger, or saboteur - and governments react accordingly.

### NPC Encounter Memory
NPCs remember their encounters with you. Assist a merchant captain multiple times, and they'll recognize you as a friend. Attack someone and escape - they'll remember your face. This creates a web of relationships that makes the universe feel more personal.

### Witness System
Crimes now require witnesses to affect your reputation. Commit piracy in an empty system with no witnesses, and no one will know. But if patrol ships observe your actions, expect consequences. You can even attempt to eliminate witnesses before they report - if you're willing to dig yourself deeper.

### Reputation Decay & Recovery
Your reputation with governments naturally drifts toward neutral over time. Positive reputation slowly decays without ongoing good deeds, while negative reputation can slowly recover - unless you've committed unforgivable atrocities. Different governments have different memory lengths and forgiveness policies.

## Installing the game

Gödel's Sky is currently available as source code only. See [Building from source](#building-from-source) below.

For the original Endless Sky, official releases are available from [GitHub](https://github.com/endless-sky/endless-sky/releases/latest), on [Steam](https://store.steampowered.com/app/404410/Endless_Sky/), on [GOG](https://gog.com/game/endless_sky), and on [Flathub](https://flathub.org/apps/details/io.github.endless_sky.endless_sky).

## System Requirements

Gödel's Sky has the same minimal system requirements as Endless Sky:

|Hardware | Minimum | Recommended |
|---|----:|----:|
|RAM | 750 MB | 2 GB |
|Graphics | OpenGL 2.0* | OpenGL 3.0 |
|Screen Resolution | 1024x768 | 1920x1080 |
|Storage Free | 400 MB | 1.5 GB |

\* For OpenGL 2 devices, [custom shaders](https://github.com/endless-sky/endless-sky/wiki/CreatingPlugins#shaders) are needed.

|Operating System | Minimum Version |
|---|---|
|Linux | Any modern distribution (equivalent of Ubuntu 20.04) |
|MacOS | 10.7 |
|Windows | XP (5.1) |

## Building from source

Development is done using [CMake](https://cmake.org) to compile the project. Most popular IDEs are supported through their respective CMake integration.

For full installation instructions, consult the [Build Instructions](docs/readme-developer.md) readme.

Quick start:
```bash
# Configure
cmake --preset linux  # or macos, clang-cl, mingw

# Build
cmake --build --preset linux-debug

# Run tests
ctest --preset linux-test

# Run game
./build/linux/Debug/endless-sky
```

## Contributing

Contributions are welcome! Gödel's Sky particularly welcomes:
- Enhancements to the consequence systems
- New government-specific reaction behaviors
- Mission content that leverages the new memory systems
- Bug fixes and performance improvements

Please review the [contribution guidelines](docs/CONTRIBUTING.md) before submitting.

## Relationship to Endless Sky

Gödel's Sky is a fork of Endless Sky. We are grateful to the Endless Sky community for creating such an excellent foundation. Changes made in Gödel's Sky may be offered back to the upstream project where appropriate.

For the original Endless Sky project, see:
- [Endless Sky GitHub](https://github.com/endless-sky/endless-sky)
- [Endless Sky Wiki](https://github.com/endless-sky/endless-sky/wiki)
- [Endless Sky Discord](https://discord.gg/ZeuASSx)

## Licensing

Gödel's Sky, like Endless Sky, is a free, open source game. The [source code](https://github.com/geeknik/endless-sky/) is available under the GPL v3 license, and all the artwork is either public domain or released under a variety of Creative Commons (and similarly permissive) licenses. (To determine the copyright status of any of the artwork, consult the [copyright file](https://github.com/endless-sky/endless-sky/blob/master/copyright).)

## Why "Gödel's Sky"?

The name references Kurt Gödel, the mathematician and logician famous for his incompleteness theorems. Just as Gödel showed that within any consistent system, there are truths that cannot be proven within that system, Gödel's Sky aims to create a universe where the consequences of your actions extend beyond what any simple reputation system can capture - a world of emergent complexity arising from interconnected memory systems.
