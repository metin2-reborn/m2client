# Development

## Recipes

| Recipe            | Does                                                                |
|-------------------|---------------------------------------------------------------------|
| `build`           | configure if needed, then build                                     |
| `test`            | build, then run the unit tests                                      |
| `run`             | run the client from the game directory                              |
| `run-sandbox`     | run the client in the overlay sandbox with `HOME=run/home`          |
| `run-auto SCRIPT` | run in the sandbox with an automation script                        |
| `smoke`, `login`, `world` | the automation smoke tests                                  |
| `grn-dump`        | load every Granny file in the game data and summarize failures       |
| `format`          | format the sources with clang-format                                |
| `clean`           | remove the build and everything the client wrote under `run/`       |

`run/` is scratch output of the recipes and is git-ignored.

## The automation harness

Debug and RelWithDebInfo builds (`M2_DEV_TOOLS`) include an automation harness that drives
the client from a script or a socket: mouse and keyboard input, screenshots, Python execution and
frame stepping. Release and MinSizeRel builds contain none of it.

```sh
just smoke                       # start, reach the login screen, quit
just login                       # log-in screen checks
just world                       # load a map
just run-auto scripts/smoke.m2a  # any automation script
just send 'screenshot shot.png'  # one command to a running client (needs socat)
```

Scripts, commands and their results are logged to `run/automation.log`. The harness is how a
change to rendering or the interface is verified: a targeted `.m2a` script in the sandbox, then a
look at its screenshots, instead of touching the game directory or the desktop.

## Tools

The build also produces `build/bin/eter_unpack`, which extracts an original client's `pack/*.eix`
+ `*.epk` archives: `eter_unpack <out dir> <pack dir or .eix files>` writes one folder per pack,
decodes the plain, compressed and key-encrypted entries and lists the Panama and hybrid-crypt
entries (their keys only ever came from the login server) in `<out dir>/unsupported.txt`.

`scripts/python_stubs.py` writes `.pyi` stubs for the `metin2.*` modules the game scripts call
(functions with their arguments from the `PyArg_ParseTuple` formats, return types, constants) into
`../m2game/typings/`, where m2game's `pyrightconfig.json` finds them; run it after changing an
export table and commit the result in m2game.

`scripts/package_release.py` lays out a release folder; see [building.md](building.md).

## Tests

Each file in `tests/` is its own executable registered with CTest; the tests inside it are selected
by name on the command line (`tests/check_main.cpp`):

```sh
ctest --test-dir build -R net_tests --output-on-failure
build/tests/net_tests <test_name> [<test_name>...]
```

Tests use the small framework in `tests/check.h` (`TEST(name)`); a new test file is added with
`m2_add_test(...)` in `tests/CMakeLists.txt`.

## Code conventions

Code style is defined by `.clang-format` and `.clang-tidy` in the repository root. Names follow the
standard library: `snake_case` for namespaces, types, functions and variables, a trailing `_` for private
and protected data members, `UPPER_CASE` for constants and enumerators. `clang-tidy` checks this.

- Sources live under `src/metin2/`, folders match namespaces under `metin2`, headers are included
  as `<metin2/...>`, every folder is an inline namespace. Everything is UTF-8.
- Format with `just format`. The files in `src/metin2/dev` keep their own style;
  `.clang-format-ignore` excludes them.
- Don't add trace or debug switches to the client. Tracing is added for one investigation and
  removed again; `.claude/skills/trace/SKILL.md` describes how.
- Keep headers self-contained, with `#pragma once`, and include what you use.
- Platform code stays behind `_WIN32` in the few files that have it (`core/mapped_file`,
  `core/error`, `core/random`, `config/state_dir`, `dev/automation`); everything else uses
  `std::filesystem` and the helpers in `core/`.
- Read game assets through `get_file_manager()` (`files/eter_file_manager.h`), never by path: asset
  names are case-insensitive and may live in the zip or the loose-file folder.
- The Python modules live under the root package `metin2` (`metin2.app`, `metin2.graphics.text`,
  `metin2.trade.shop`, ...); register a new one with `init_python_module` (`script/python_module.h`),
  which creates the parent packages. Export names (the strings in the module tables) are
  `snake_case`; the game scripts call them, so an export and its call sites in `m2game/data` change
  together.
- Wire structures in `client/protocol/packet.h` are packed and checked with `static_assert`; their
  sizes must match the server's.
- Comments explain what the code cannot: pitfalls, ordering, lifetimes, protocol facts.
- Build in Debug (`-Werror`), run `just test`, and run the smoke tests before sending a change.
  CI builds Debug with Clang on Debian 13 and runs CTest, and builds on Windows with clang-cl.
