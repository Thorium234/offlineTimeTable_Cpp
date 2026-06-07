#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

BUILD_DIR="build_cmake"
BINARY="$BUILD_DIR/timetableGen"
TEST_BINARY="$BUILD_DIR/test_runner"

# ── Help ─────────────────────────────────────────────────────────────────────
usage() {
    cat <<EOF
Usage: ./run.sh [OPTION]

Options:
  --help        Show this help and exit
  --debug       Build and run in Debug mode (ASan+UBSan enabled)
  --test        Build and run unit tests
  --check       Build with warnings-as-errors, run tests
  --cli         Force CLI mode (no GUI, implies --debug)
  --clean       Remove build artifacts then exit

Environment:
  FLASK_PORT    Port for the web server (default: 5000)
  CMAKE_GENERATOR   CMake generator (default: Unix Makefiles)
EOF
    exit 0
}

# ── Flags ────────────────────────────────────────────────────────────────────
BUILD_TYPE="Release"
CMAKE_ARGS=()
RUN_TESTS=0
RUN_CLI=0

for arg in "$@"; do
    case "$arg" in
        --help) usage ;;
        --debug) BUILD_TYPE="Debug" ;;
        --test) RUN_TESTS=1 ;;
        --check) BUILD_TYPE="Debug"; RUN_TESTS=1 ;;
        --cli) BUILD_TYPE="Debug"; RUN_CLI=1 ;;
        --clean)
            rm -rf "$BUILD_DIR" *.o test_runner timetableGen
            echo "[run.sh] Cleaned."
            exit 0
            ;;
        *) echo "Unknown option: $arg"; usage ;;
    esac
done

# ── Dependency Check ─────────────────────────────────────────────────────────
check_deps() {
    local missing=()
    command -v cmake    >/dev/null 2>&1 || missing+=("cmake")
    command -v g++      >/dev/null 2>&1 || missing+=("g++")
    command -v pkg-config >/dev/null 2>&1 || missing+=("pkg-config")
    pkg-config --exists Qt5Widgets 2>/dev/null || missing+=("qtbase5-dev")
    pkg-config --exists Qt5Sql     2>/dev/null || missing+=("libqt5sql5-sqlite")

    if [ ${#missing[@]} -gt 0 ]; then
        echo "[run.sh] Missing dependencies: ${missing[*]}"
        echo "[run.sh] Install with: sudo apt install cmake g++ pkg-config qtbase5-dev libqt5sql5-sqlite"
        exit 1
    fi
}
check_deps

# ── Build ────────────────────────────────────────────────────────────────────
echo "[run.sh] Configuring ${BUILD_TYPE} build..."
mkdir -p "$BUILD_DIR"
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE" ${CMAKE_ARGS+"${CMAKE_ARGS[@]}"}

echo "[run.sh] Building..."
cmake --build "$BUILD_DIR" -j "$(nproc)"

# ── Run Tests ────────────────────────────────────────────────────────────────
if [ "$RUN_TESTS" -eq 1 ]; then
    if [ -x "$TEST_BINARY" ]; then
        echo "[run.sh] Running tests..."
        "$TEST_BINARY"
    else
        echo "[run.sh] Test binary not found; skipping tests."
    fi
    # In --check mode, stop after tests
    if [ "$RUN_CLI" -eq 0 ]; then
        exit 0
    fi
fi

# ── Run Application ──────────────────────────────────────────────────────────
if [ ! -x "$BINARY" ]; then
    echo "[run.sh] Binary not found at $BINARY"
    exit 1
fi

# Prepend system lib path to avoid Anaconda's old libstdc++
export LD_LIBRARY_PATH="/usr/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

# Special Qt5 path (for CI / extracted debs)
if [ -d /tmp/qt5/usr/lib/x86_64-linux-gnu ]; then
    export LD_LIBRARY_PATH="/tmp/qt5/usr/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    export QT_PLUGIN_PATH="/tmp/qt5/usr/lib/x86_64-linux-gnu/qt5/plugins${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}"
fi

echo "[run.sh] Launching timetable generator..."
if [ "$RUN_CLI" -eq 1 ]; then
    exec "$BINARY" --cli
else
    exec "$BINARY"
fi
