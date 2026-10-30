# Running

## The game directory

The client needs the game directory: `data/` (the bootstrap in `data/system`, the `data/ui` pack
with the game scripts, the UI package and the locale data, the other archives) and `lib/`, the
Python 2.7 standard library as `.pyc` files, which is the only standard library the embedded
Python ever sees (no `PYTHONHOME`, no `site`). An installation is the executable, `data/` and
`lib/`, nothing else.

`just run` starts the client from the game directory as a normal process, with your own
`~/.metin2`. The game directory defaults to an `m2game` checkout next to this repository; set
`METIN2_CLIENT_DIR` to use another one:

```sh
just run
METIN2_CLIENT_DIR=/path/to/client-data just run
```

`just run-sandbox` starts it in a bubblewrap sandbox instead: the game directory is mounted as an
overlay, so nothing can modify it, and `HOME` is `run/home` (a windowed
`run/home/.metin2/metin2.toml` is written on first use). The automation recipes use the sandbox.
The server address is part of the game scripts' server list.

## ~/.metin2

| Path                       | Contents                                                          |
|----------------------------|-------------------------------------------------------------------|
| `metin2.toml`              | settings (display, graphics, sound, interface, login, log level)  |
| `interface.dat`            | window positions and UI state                                     |
| `log/client.log`           | log of the current run (the previous five are kept as `.1`-`.5`)  |
| `log/syserr.log`           | warnings and errors with file and line, Python tracebacks         |
| `log/crash.log`            | stack trace of a crash                                            |
| `mark/`                    | guild marks downloaded from the server                            |
| `screenshots/`             | screenshots                                                       |

On Windows the directory is `%APPDATA%\metin2`.

## metin2.toml

A minimal file:

```toml
[display]
width = 1280
height = 720             # the window size; windowed = false is a borderless full screen on the
windowed = true          # current monitor at the desktop's resolution (no mode switch, no chrome)
vsync = true
fps_limit = 0            # 0: no cap beyond vsync; e.g. 144
gamma = 3                # 1 (darker) to 5 (brighter), 3 is neutral

[graphics]
antialias = true         # FXAA on the 3D scene
shadow_level = 3
terrain_detail = 1       # relief shading from the terrain textures: 0 off, 1 low, 2 high
grass_density = 1        # grass on grass and field tiles: 0 off to 3

[interface]
font_scale = 1.0         # text size factor
font_antialias = true    # false gives the original 1-bit glyphs
ui_scale = 0             # interface magnification 1-3; 0 picks it from the window height
                         # (text is rasterized at the magnified size, not enlarged; on a scaled
                         # desktop the window is high-DPI and the automatic choice sees real pixels)
chat_timestamps = false  # prefix spoken chat lines with [HH:MM]

[keys]                   # optional: rebind game actions (names as in DIK_*, e.g. "F", "NUMPAD1", "GRAVE")
inventory = "I"
pick_up = "Z"

[sound]
music_volume = 0.5
voice_volume = 1.0

[log]
level = "info"           # "debug", "info", "warn" or "error"
```

Rebindable actions and their original keys: `attack` (SPACE), `move_up`/`move_down`/`move_left`/
`move_right` (arrows) and their `_alt` twins (W/S/A/D), `camera_left`/`camera_right` (Q/E),
`camera_zoom_in`/`camera_zoom_out` (R/F), `camera_pitch_up`/`camera_pitch_down` (T/G), `pick_up`
(Z) and `pick_up_alt` (`), `sprint` (LSHIFT, held), `character` (C), `skills` (V), `quests` (N),
`inventory` (I), `dragon_soul` (O), `map` (M), `chat_log` (L), `mount` (J), `help` (H), `emotions`
(B), `debug_info` (F12, the fps and position overlay, hidden when the game starts). Two actions bound to one key both fire.

A key with a wrong type or an unknown key is reported in `syserr.log` with its line and keeps its
default; the rest of the file still applies. `python_system::load_config` in
`src/metin2/client/app/python_system.cpp` reads every option.
