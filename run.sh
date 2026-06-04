#!/bin/bash
set -e

# Set Qt plugin path so the SQLite driver is found
export QT_PLUGIN_PATH="/nix/store/jkjspflrnava83j3b0ij8dvax3dhywjh-qt-full-5.15.17/lib/qt-5.15.17/plugins"

# Build the project if needed
if [ ! -f build_cmake/timetableGen ]; then
    echo "Building timetableGen..."
    mkdir -p build_cmake
    cd build_cmake
    cmake ..
    make -j$(nproc)
    cd ..
fi

# Run in CLI mode (no display server available in Replit)
./build_cmake/timetableGen --cli
