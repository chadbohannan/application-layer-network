#!/usr/bin/env zsh
set -e

SCRIPT_DIR="${0:a:h}"
BUILD_DIR="$SCRIPT_DIR/build"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "Running qmake6..."
qmake6 "$SCRIPT_DIR/aln-qt.pro"

echo "Building..."
make -j$(nproc)

echo "Launching aln-wrench-qt..."
exec env QT_SCALE_FACTOR=0.75 "$BUILD_DIR/aln-wrench-qt/aln-wrench-qt"
