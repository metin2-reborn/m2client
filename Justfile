# Metin2 Linux port — build/run helpers.
# `just build` configures + builds (RelWithDebInfo), `just run` launches the client inside a
# bubblewrap sandbox whose game directory is an overlay (writes land in run/overlay/upper).

set shell := ["bash", "-euo", "pipefail", "-c"]

root   := justfile_directory()
# SANITIZE=1 builds with AddressSanitizer and UndefinedBehaviorSanitizer into build-sanitize/.
build  := root / if env("SANITIZE", "") != "" { "build-sanitize" } else { "build" }
# Game data: the m2game checkout next to this repo unless METIN2_CLIENT_DIR points elsewhere.
client := env("METIN2_CLIENT_DIR", clean(root / ".." / "m2game"))
pyhome := build / "python27-install"

default: build

# Clang unless the environment picks another compiler.
export CC := env("CC", "clang")
export CXX := env("CXX", "clang++")

# Configure the build tree (Ninja; BUILD_TYPE defaults to RelWithDebInfo, Debug adds -Werror).
configure:
    cmake -S "{{root}}" -B "{{build}}" -G Ninja -DCMAKE_BUILD_TYPE="${BUILD_TYPE:-RelWithDebInfo}" \
        -DM2_SANITIZE=$([ -n "${SANITIZE:-}" ] && echo ON || echo OFF)

# Configure if needed, then build everything.
build:
    @[ -f "{{build}}/build.ninja" ] || just configure
    cmake --build "{{build}}"

# Remove build outputs and everything the client wrote under run/.
clean:
    rm -rf "{{build}}" "{{root}}/run/overlay/work"
    [ -d "{{root}}/run/overlay/upper" ] && find "{{root}}/run/overlay/upper" -mindepth 1 -delete || true
    rm -f "{{root}}/run/"*.png "{{root}}/run/automation.log" "{{root}}/run/automation.sock" "{{root}}/run/"*.log

# Record a SHA-256 manifest of the game directory (taken once before the first run).
checksum:
    (cd "{{client}}" && find . -path ./.git -prune -o -type f -print0 | sort -z | xargs -0 sha256sum) > "{{root}}/run/gamedir.sha256"
    wc -l "{{root}}/run/gamedir.sha256"

# Verify the game directory is unchanged against the recorded manifest (.git/ is not part of it).
verify-gamedir:
    (cd "{{client}}" && sha256sum --quiet -c "{{root}}/run/gamedir.sha256")
    diff <(sed 's/^[0-9a-f]*  //' "{{root}}/run/gamedir.sha256") <(cd "{{client}}" && find . -path ./.git -prune -o -type f -print | sort) \
        || { echo "files were added to or removed from the game directory (> added, < removed)" >&2; exit 1; }
    @echo "game directory unchanged"

# Make sure the overlay directories exist and seed a windowed ~/.metin2/metin2.toml (HOME is run/home) on first use.
seed-config:
    mkdir -p "{{root}}/run/overlay/upper" "{{root}}/run/overlay/work" "{{root}}/run/home/.metin2"
    @if [ ! -f "{{root}}/run/home/.metin2/metin2.toml" ]; then \
        printf '[display]\nwidth = 1280\nheight = 720\nbpp = 32\nfrequency = 60\nwindowed = true\ngamma = 3\nsoftware_cursor = true\n\n[graphics]\nvisibility = 3\nobject_culling = true\nshadow_level = 3\n\n[sound]\nmusic_volume = 0.5\nvoice_volume = 1\n' > "{{root}}/run/home/.metin2/metin2.toml"; \
        echo "seeded run/home/.metin2/metin2.toml (windowed 1280x720)"; fi

# Build if needed, then run the client in the sandbox. Extra arguments go to the client.
run *ARGS: build seed-config
    @command -v bwrap >/dev/null || { echo "bubblewrap (bwrap) is required to protect the game directory" >&2; exit 1; }
    bwrap --dev-bind / / --tmpfs /tmp \
        --overlay-src "{{client}}" --overlay "{{root}}/run/overlay/upper" "{{root}}/run/overlay/work" "{{client}}" \
        --bind "{{root}}/run" "{{root}}/run" \
        --chdir "{{client}}" \
        --setenv PYTHONHOME "{{pyhome}}" \
        --setenv HOME "{{root}}/run/home" \
        --setenv METIN2_RUN_DIR "{{root}}/run" \
        --setenv METIN2_AUTOMATION_LOG "{{root}}/run/automation.log" \
        --setenv METIN2_AUTOMATION_SOCKET "{{root}}/run/automation.sock" \
        --setenv METIN2_SCREENSHOT_DIR "{{root}}/run" \
        --setenv ASAN_OPTIONS "log_path={{root}}/run/sanitizer:detect_leaks=0" \
        --setenv UBSAN_OPTIONS "log_path={{root}}/run/sanitizer:print_stacktrace=1" \
        -- "{{build}}/bin/client" {{ARGS}}

# Run with the automation harness driven by a script file (path relative to the repo).
run-auto SCRIPT *ARGS: build seed-config
    METIN2_AUTOMATION=1 METIN2_AUTOMATION_SCRIPT="{{root}}/{{SCRIPT}}" just run {{ARGS}}

# Send one command line to a running client's automation socket (needs socat).
send CMD:
    printf '%s\n' "{{CMD}}" | socat - UNIX-CONNECT:"{{root}}/run/automation.sock"

smoke: (run-auto "scripts/smoke.m2a")
login: (run-auto "scripts/login_screen.m2a")
world: (run-auto "scripts/world_test.m2a")

# Build, then run the unit tests.
test: build
    ctest --test-dir "{{build}}" --output-on-failure

# Format the sources with clang-format (.clang-format-ignore keeps platform/ and dev/ in their own style).
format:
    git ls-files --cached --others --exclude-standard 'src/*.h' 'src/*.cpp' 'tests/*.h' 'tests/*.cpp' 'tools/*.cpp' \
        | while read -r f; do [ -f "$f" ] && echo "$f"; done | xargs clang-format -i

# Load every .gr2 in the system archive through the Granny runtime and summarize formats/failures.
grn-dump *ARGS: build seed-config
    bwrap --dev-bind / / --tmpfs /tmp \
        --overlay-src "{{client}}" --overlay "{{root}}/run/overlay/upper" "{{root}}/run/overlay/work" "{{client}}" \
        --bind "{{root}}/run" "{{root}}/run" --chdir "{{client}}" \
        -- "{{build}}/bin/grn_dump" {{ARGS}} | tee "{{root}}/run/grn_dump.log"
