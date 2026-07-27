#!/bin/bash
cd "$(dirname "$0")"

# System cmake (3.22) is too old for this project; use the pipx one
CMAKE="${CMAKE:-$HOME/.local/bin/cmake}"
[[ -x "$CMAKE" ]] || CMAKE=cmake

TARGET="lenny-raylib-template"
DEBUG_MODE="${1:-}"

# Force a relink even when nothing changed, so the POST_BUILD resource
# copy always runs
rm -f "build/$TARGET/$TARGET"

TMUX_DEBUG_SESSION="lenny-gdbgui-debug"
TMUX_RUN_SESSION="lenny-game-run"

if [[ "$DEBUG_MODE" == "raddbg" ]]; then
    "$CMAKE" -B build -DCMAKE_BUILD_TYPE=Debug && "$CMAKE" --build build || exit 1

    RADDBG="$PWD/vendor/raddebugger/build/raddbg"
    if [[ ! -x "$RADDBG" ]]; then
        echo "raddbg not built yet; building it now (one-time)..."
        git submodule update --init vendor/raddebugger
        (cd vendor/raddebugger && ./build.sh raddbg release) || exit 1
    fi

    # cwd must be the binary dir so the game finds resources/
    cd "build/$TARGET" && exec "$RADDBG" "./$TARGET"
elif [[ "$DEBUG_MODE" == "gdb" ]]; then
    "$CMAKE" -B build -DCMAKE_BUILD_TYPE=Debug && "$CMAKE" --build build || exit 1
    gdb ./build/"$TARGET"/"$TARGET"
elif [[ "$DEBUG_MODE" == "gdbgui" ]]; then
    "$CMAKE" -B build -DCMAKE_BUILD_TYPE=Debug && "$CMAKE" --build build || exit 1

    for pid in $(pgrep -f "python.*gdbgui" 2>/dev/null); do
        kill "$pid" 2>/dev/null || true
    done
    sleep 0.3

    tmux kill-session -t "$TMUX_DEBUG_SESSION" 2>/dev/null || true
    sleep 0.2
    tmux new-session -d -s "$TMUX_DEBUG_SESSION" -c "build/$TARGET" "gdbgui ./$TARGET"

    echo "gdbgui started in tmux session '$TMUX_DEBUG_SESSION'"
    echo "Open http://127.0.0.1:5000 in your browser"
    exit 0
else
    "$CMAKE" -B build && "$CMAKE" --build build || exit 1

    # A graphics driver update can break GLX until the next reboot (context
    # creation fails BadValue and the game segfaults on launch). Fall back to
    # Mesa's software renderer so the game still runs; once GLX is healthy
    # again this check passes and the GPU is used as normal.
    GL_ENV=""
    if command -v glxinfo >/dev/null 2>&1 && ! glxinfo -B >/dev/null 2>&1; then
        echo "WARNING: GLX is broken (reboot pending?) - running on software GL"
        GL_ENV="__GLX_VENDOR_LIBRARY_NAME=mesa LIBGL_ALWAYS_SOFTWARE=1"
    fi

    tmux kill-session -t "$TMUX_RUN_SESSION" 2>/dev/null || true
    sleep 0.2
    # cwd must be the binary dir so the game finds resources/
    tmux new-session -d -s "$TMUX_RUN_SESSION" -c "$PWD/build/$TARGET" "$GL_ENV ./$TARGET"

    echo "Running in tmux session '$TMUX_RUN_SESSION'"
    exit 0
fi
