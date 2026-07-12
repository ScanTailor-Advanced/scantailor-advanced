#!/usr/bin/env bash
# Build ScanTailor Advanced and create a fakesigned .ipa for iOS.
# Usage: ./build-ios.sh [build_dir]
# Requires: Qt 6.x with iOS target, Xcode, ldid (brew install ldid)
#
# The resulting .ipa is fakesigned and can be installed via:
#   - Sideloadly (sideloadly.io)
#   - AltStore    (altstore.io)
#   - TrollStore  (if device is supported)
#   - ideviceinstaller after re-signing with a real cert
#
# Environment variables (override defaults):
#   QT_IOS_DIR     Path to Qt iOS installation  (e.g. ~/Qt/6.12.0/ios)
#   QT_MACOS_DIR   Path to Qt macOS installation (e.g. ~/Qt/6.12.0/macos)
#   BUNDLE_ID      App bundle identifier          (e.g. com.yourname.scantailor)
#   DEVICE_ID      Target device UDID for install (optional, requires real cert)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

BUILD_DIR="${1:-build-ios}"
mkdir -p "$BUILD_DIR"
BUILD_DIR="$(cd "$BUILD_DIR" && pwd)"

# ── Check for ldid ─────────────────────────────────────────────────────────────

if ! command -v ldid >/dev/null 2>&1; then
  echo "Error: ldid not found. Install with: brew install ldid" >&2
  exit 1
fi

# ── Generate icons ─────────────────────────────────────────────────────────────

if [[ -f "${SCRIPT_DIR}/generate-icons.sh" ]]; then
  echo "Generating icons..."
  bash "${SCRIPT_DIR}/generate-icons.sh"
else
  echo "Warning: generate-icons.sh not found, skipping icon generation." >&2
fi

# ── Locate Qt ──────────────────────────────────────────────────────────────────

if [[ -z "$QT_IOS_DIR" ]]; then
  QT_IOS_DIR=$(find ~/Qt -name "Qt6Config.cmake" 2>/dev/null \
    | grep "/ios/" | head -1 | sed 's|/lib/cmake/Qt6/Qt6Config.cmake||')
fi
if [[ -z "$QT_MACOS_DIR" ]]; then
  QT_MACOS_DIR=$(find ~/Qt -name "Qt6Config.cmake" 2>/dev/null \
    | grep "/macos/" | head -1 | sed 's|/lib/cmake/Qt6/Qt6Config.cmake||')
fi

if [[ -z "$QT_IOS_DIR" || ! -d "$QT_IOS_DIR" ]]; then
  echo "Error: Could not find Qt iOS installation." >&2
  echo "Install via: aqt install-qt mac ios <version> ios --outputdir ~/Qt" >&2
  echo "Or set QT_IOS_DIR=/path/to/Qt/6.x.x/ios" >&2
  exit 1
fi
if [[ -z "$QT_MACOS_DIR" || ! -d "$QT_MACOS_DIR" ]]; then
  echo "Error: Could not find Qt macOS installation (needed for host tools)." >&2
  echo "Install via: aqt install-qt mac desktop <version> clang_64 --outputdir ~/Qt" >&2
  echo "Or set QT_MACOS_DIR=/path/to/Qt/6.x.x/macos" >&2
  exit 1
fi

echo "Qt iOS:   $QT_IOS_DIR"
echo "Qt macOS: $QT_MACOS_DIR"

# ── Extract version ────────────────────────────────────────────────────────────

VERSION=$(sed -n 's/^#define VERSION "\([^"]*\)".*/\1/p' version.h.in)
if [[ -z "$VERSION" ]]; then
  echo "Error: could not read VERSION from version.h.in" >&2
  exit 1
fi

BUNDLE_ID="${BUNDLE_ID:-com.$(id -un).scantailor}"
echo "Bundle ID: $BUNDLE_ID"
echo "Building ScanTailor Advanced ${VERSION} for iOS (fakesigned)"

# ── Configure ─────────────────────────────────────────────────────────────────

cd "$BUILD_DIR"
cmake -G Xcode \
  -DQt6_DIR="${QT_IOS_DIR}/lib/cmake/Qt6" \
  -DQT_HOST_PATH="${QT_MACOS_DIR}" \
  -DCMAKE_PREFIX_PATH="${QT_IOS_DIR};${QT_MACOS_DIR}" \
  -DCMAKE_TOOLCHAIN_FILE="${QT_IOS_DIR}/lib/cmake/Qt6/qt.toolchain.cmake" \
  -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH \
  -DBUNDLE_ID="${BUNDLE_ID}" \
  "$SCRIPT_DIR"

# ── Build (no real signing) ────────────────────────────────────────────────────

xcodebuild \
  -project "ScanTailor Advanced.xcodeproj" \
  -scheme "scantailor-advanced" \
  -destination "generic/platform=iOS" \
  -configuration Release \
  CODE_SIGNING_ALLOWED=NO \
  CODE_SIGNING_REQUIRED=NO \
  CODE_SIGN_IDENTITY="" \
  build

# ── Find built .app ────────────────────────────────────────────────────────────

APP=$(find "$BUILD_DIR" -name "scantailor-advanced.app" \
  -not -path "*/simulator/*" | head -1)

if [[ -z "$APP" ]]; then
  echo "Error: could not find built .app" >&2
  exit 1
fi

# ── Fakesign with ldid ─────────────────────────────────────────────────────────

echo "Fakesigning with ldid..."
ENTITLEMENTS="${SCRIPT_DIR}/ios/scantailor.entitlements"
if [[ -f "$ENTITLEMENTS" ]]; then
  ldid -S"$ENTITLEMENTS" "$APP/scantailor-advanced"
else
  ldid -S "$APP/scantailor-advanced"
fi
# Sign any embedded frameworks/dylibs too
find "$APP" -name "*.dylib" -o -name "*.framework" | while read -r lib; do
  ldid -S "$lib" 2>/dev/null || true
done

# ── Package as IPA ────────────────────────────────────────────────────────────

IPA_DIR="/tmp/scantailor-ipa-$$"
mkdir -p "${IPA_DIR}/Payload"
cp -r "$APP" "${IPA_DIR}/Payload/"

IPA_PATH="${BUILD_DIR}/scantailor-advanced_${VERSION}_ios.ipa"
cd "$IPA_DIR"
zip -r "$IPA_PATH" Payload/
rm -rf "$IPA_DIR"

echo ""
echo "Done: $IPA_PATH"
echo ""
echo "Install options:"
echo "  Sideloadly:      drag the .ipa onto sideloadly.io app"
echo "  AltStore:        use AltServer to sideload"
echo "  TrollStore:      if your device is supported"