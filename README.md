# Metin2 Client

[![CI](https://github.com/metin2-reborn/m2client/actions/workflows/ci.yml/badge.svg)](https://github.com/metin2-reborn/m2client/actions/workflows/ci.yml)
![C++23](https://img.shields.io/badge/C%2B%2B-23-00599C?logo=cplusplus)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows-FCC624)
[![Last commit](https://img.shields.io/github/last-commit/metin2-reborn/m2client)](https://github.com/metin2-reborn/m2client/commits)
[![Issues](https://img.shields.io/github/issues/metin2-reborn/m2client)](https://github.com/metin2-reborn/m2client/issues)

The game client of Metin2 Reborn: the classic Metin2 client, ported from its 2000s Windows and
DirectX 8 code base to Linux and Windows, and cleaned up to the standard of the rest of the project.

It is part of a family of repositories:

| Repository   | What it is                                                     |
|--------------|----------------------------------------------------------------|
| `m2client`   | this client                                                    |
| `m2game`     | the game data: scripts, interface, maps, models, sounds        |
| `m2server`   | the game server                                                |
| `m2launcher` | the launcher and patcher players start the game with           |
| `m2web`      | the account website and its API                                |

## What it does

- Renders the original world, characters and effects through its own renderer on OpenGL.
- Runs the original game logic, written in Python, from the `m2game` data.
- Plays music and sounds through OpenAL.
- Talks to `m2server` over an encrypted connection.
- Keeps its settings, logs and screenshots in `~/.metin2` (`%APPDATA%\metin2` on Windows) and
  never writes into the game data.

## Getting started

You need a Linux machine with Clang, CMake, Ninja and [just](https://github.com/casey/just), and an
`m2game` checkout next to this repository.

```sh
just build   # fetches the dependencies, builds build/bin/client
just run     # starts the client from ../m2game
```

The exact packages, the Windows build and the release packaging are in
[docs/building.md](docs/building.md).

## Documentation

| Document                                   | Covers                                                              |
|--------------------------------------------|---------------------------------------------------------------------|
| [docs/building.md](docs/building.md)       | requirements, building on Linux and Windows, release packaging      |
| [docs/running.md](docs/running.md)         | the game directory, `~/.metin2`, every setting of `metin2.toml`     |
| [docs/development.md](docs/development.md) | the `just` recipes, the automation harness, tools, code conventions |

## Contributing

Pull requests are welcome. Please read [docs/development.md](docs/development.md) first: it has the
code style, the build-and-test checklist and the rules the code base follows.

## Third-party software

Licenses of bundled and fetched dependencies are in [`vendor/licenses`](vendor/licenses).

## Credits

Thanks to Mali for the Visual Studio 2022 client sources this port started from:
<https://metin2.dev/topic/22115-compile-client-with-visual-studio-2022-src/>
