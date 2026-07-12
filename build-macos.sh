#!/usr/bin/env bash
# Build ScanTailor Advanced and create an ad-hoc signed .dmg for macOS.
# Usage: ./build-macos.sh [build_dir]
# Requires: Qt 6.x (macOS), CMake, Xcode Command Line Tools
#
# The app is signed with an ad-hoc signature (-) so it runs on your own Mac
# without a developer certificate. Other users will need to run:
#   xattr -cr /Applications/ScanTailor\ Advanced.app
# to remove the quarantine attribute after copying from the DMG.
#
# Environment variables (override defaults):
#   QT_MACOS_DIR   Path to Qt macOS installation (e.g. ~/Qt/6.12.0/macos)
#                  Falls back to Homebrew Qt if not set.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

BUILD_DIR="${1:-build-macos}"
mkdir -p "$BUILD_DIR"
BUILD_DIR="$(cd "$BUILD_DIR" && pwd)"

# ── Generate icons ─────────────────────────────────────────────────────────────

if [[ -f "${SCRIPT_DIR}/generate-icons.sh" ]]; then
  echo "Generating icons..."
  bash "${SCRIPT_DIR}/generate-icons.sh"
else
  echo "Warning: generate-icons.sh not found, skipping icon generation." >&2
fi

# ── Locate Qt ──────────────────────────────────────────────────────────────────

if [[ -z "$QT_MACOS_DIR" ]]; then
  QT_MACOS_DIR=$(find ~/Qt -name "Qt6Config.cmake" 2>/dev/null \
    | grep "/macos/" | head -1 | sed 's|/lib/cmake/Qt6/Qt6Config.cmake||')
fi
if [[ -z "$QT_MACOS_DIR" || ! -d "$QT_MACOS_DIR" ]]; then
  if [[ -d "/opt/homebrew/opt/qt" ]]; then
    QT_MACOS_DIR="/opt/homebrew/opt/qt"
  elif [[ -d "/usr/local/opt/qt" ]]; then
    QT_MACOS_DIR="/usr/local/opt/qt"
  fi
fi
if [[ -z "$QT_MACOS_DIR" || ! -d "$QT_MACOS_DIR" ]]; then
  echo "Error: Could not find Qt macOS installation." >&2
  echo "Install via Homebrew: brew install qt" >&2
  echo "Or set QT_MACOS_DIR=/path/to/Qt/6.x.x/macos" >&2
  exit 1
fi
echo "Qt macOS: $QT_MACOS_DIR"

# ── Locate dependencies ────────────────────────────────────────────────────────

for pkg in jpeg libpng libtiff boost; do
  if ! brew list "$pkg" &>/dev/null; then
    echo "Installing missing dependency: $pkg"
    brew install "$pkg"
  fi
done

# ── Extract version ────────────────────────────────────────────────────────────

VERSION=$(sed -n 's/^#define VERSION "\([^"]*\)".*/\1/p' version.h.in)
if [[ -z "$VERSION" ]]; then
  echo "Error: could not read VERSION from version.h.in" >&2
  exit 1
fi
echo "Building ScanTailor Advanced ${VERSION} for macOS"

# ── Configure ─────────────────────────────────────────────────────────────────

cd "$BUILD_DIR"
cmake -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="${QT_MACOS_DIR}" \
  -DCMAKE_INSTALL_PREFIX=/usr/local \
  "$SCRIPT_DIR"

# ── Build ──────────────────────────────────────────────────────────────────────

make -j"$(sysctl -n hw.logicalcpu)"

# ── Bundle Qt frameworks ───────────────────────────────────────────────────────

APP_PATH="${BUILD_DIR}/ScanTailor Advanced.app"

MACDEPLOYQT="${QT_MACOS_DIR}/bin/macdeployqt"
if [[ ! -x "$MACDEPLOYQT" ]]; then
  MACDEPLOYQT=$(find /opt/homebrew /usr/local -name macdeployqt 2>/dev/null | head -1)
fi
if [[ -x "$MACDEPLOYQT" ]]; then
  "$MACDEPLOYQT" "$APP_PATH" -verbose=1
else
  echo "Warning: macdeployqt not found, skipping Qt framework bundling." >&2
fi

# ── Ad-hoc sign (no developer account needed) ─────────────────────────────────

echo "Ad-hoc signing..."
ENTITLEMENTS="${SCRIPT_DIR}/macos/scantailor.entitlements"
if [[ -f "$ENTITLEMENTS" ]]; then
  codesign --force --deep --sign - --entitlements "$ENTITLEMENTS" "$APP_PATH"
else
  codesign --force --deep --sign - "$APP_PATH"
fi

# ── Create DMG ────────────────────────────────────────────────────────────────

DMG_PATH="${SCRIPT_DIR}/scantailor-advanced_${VERSION}_macos.dmg"
ICNS="${SCRIPT_DIR}/src/resources/appicon.icns"

if command -v create-dmg >/dev/null 2>&1; then
  create-dmg \
    --volname "ScanTailor Advanced ${VERSION}" \
    $([[ -f "$ICNS" ]] && echo "--volicon \"$ICNS\"") \
    --window-pos 200 120 \
    --window-size 600 400 \
    --icon-size 100 \
    --icon "scantailor-advanced.app" 175 190 \
    --hide-extension "scantailor-advanced.app" \
    --app-drop-link 425 190 \
    "$DMG_PATH" \
    "$APP_PATH"
else
  hdiutil create -volname "ScanTailor Advanced" \
    -srcfolder "$APP_PATH" \
    -ov -format UDZO \
    "$DMG_PATH"
fi

echo ""
echo "Done: $DMG_PATH"
echo ""
echo "Note: users installing from the DMG should run:"
echo "  xattr -cr /Applications/ScanTailor\\ Advanced.app"