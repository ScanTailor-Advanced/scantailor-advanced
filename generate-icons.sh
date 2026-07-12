#!/usr/bin/env bash
# Generate iOS and macOS app icons from a source image.
# Prefers src/resources/appicon-ios.png, falls back to appicon.svg.
# Usage: ./generate-icons.sh
# Requires: sips (macOS built-in) for PNG source, or librsvg/Inkscape for SVG

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PNG="${SCRIPT_DIR}/src/resources/appicon-ios.png"
SVG="${SCRIPT_DIR}/src/resources/appicon.svg"

# ── Pick source and renderer ───────────────────────────────────────────────────

if [[ -f "$PNG" ]]; then
  SOURCE="$PNG"
  # sips is built into macOS — no install needed
  render() {
    sips -z "$1" "$1" --out "$2" "$SOURCE" >/dev/null 2>&1
  }
  echo "Source: $SOURCE (PNG)"
elif [[ -f "$SVG" ]]; then
  SOURCE="$SVG"
  if command -v rsvg-convert >/dev/null 2>&1; then
    render() { rsvg-convert -w "$1" -h "$1" -o "$2" "$SOURCE"; }
  elif command -v inkscape >/dev/null 2>&1; then
    render() { inkscape --export-type=png --export-width="$1" --export-filename="$2" "$SOURCE" 2>/dev/null; }
  else
    echo "Error: no SVG renderer found. Install: brew install librsvg" >&2
    exit 1
  fi
  echo "Source: $SOURCE (SVG)"
else
  echo "Error: no source icon found." >&2
  echo "Place a PNG at: src/resources/appicon-ios.png" >&2
  echo "Or an SVG at:   src/resources/appicon.svg" >&2
  exit 1
fi

# ══════════════════════════════════════════════════════════════════════════════
# iOS — AppIcon.appiconset
# ══════════════════════════════════════════════════════════════════════════════

IOS_DIR="${SCRIPT_DIR}/build-ios/AppIcon.appiconset"
mkdir -p "$IOS_DIR"

echo "Generating iOS icons..."

for size in 20 29 40 58 60 76 80 87 120 152 167 180 1024; do
  out="${IOS_DIR}/icon_${size}.png"
  echo "  ${size}x${size} → icon_${size}.png"
  render "$size" "$out"
done

cat > "${IOS_DIR}/Contents.json" << 'JSON'
{
  "images": [
    { "idiom": "universal", "platform": "ios", "size": "1024x1024", "scale": "1x", "filename": "icon_1024.png" },
    { "idiom": "iphone",  "size": "20x20",    "scale": "2x", "filename": "icon_40.png"  },
    { "idiom": "iphone",  "size": "20x20",    "scale": "3x", "filename": "icon_60.png"  },
    { "idiom": "iphone",  "size": "29x29",    "scale": "2x", "filename": "icon_58.png"  },
    { "idiom": "iphone",  "size": "29x29",    "scale": "3x", "filename": "icon_87.png"  },
    { "idiom": "iphone",  "size": "40x40",    "scale": "2x", "filename": "icon_80.png"  },
    { "idiom": "iphone",  "size": "40x40",    "scale": "3x", "filename": "icon_120.png" },
    { "idiom": "iphone",  "size": "60x60",    "scale": "2x", "filename": "icon_120.png" },
    { "idiom": "iphone",  "size": "60x60",    "scale": "3x", "filename": "icon_180.png" },
    { "idiom": "ipad",    "size": "20x20",    "scale": "1x", "filename": "icon_20.png"  },
    { "idiom": "ipad",    "size": "20x20",    "scale": "2x", "filename": "icon_40.png"  },
    { "idiom": "ipad",    "size": "29x29",    "scale": "1x", "filename": "icon_29.png"  },
    { "idiom": "ipad",    "size": "29x29",    "scale": "2x", "filename": "icon_58.png"  },
    { "idiom": "ipad",    "size": "40x40",    "scale": "1x", "filename": "icon_40.png"  },
    { "idiom": "ipad",    "size": "40x40",    "scale": "2x", "filename": "icon_80.png"  },
    { "idiom": "ipad",    "size": "76x76",    "scale": "1x", "filename": "icon_76.png"  },
    { "idiom": "ipad",    "size": "76x76",    "scale": "2x", "filename": "icon_152.png" },
    { "idiom": "ipad",    "size": "83.5x83.5","scale": "2x", "filename": "icon_167.png" }
  ],
  "info": { "author": "xcode", "version": 1 }
}
JSON

echo "iOS icons → $IOS_DIR"

# ══════════════════════════════════════════════════════════════════════════════
# macOS — .icns via iconutil
# ══════════════════════════════════════════════════════════════════════════════

ICONSET="${SCRIPT_DIR}/src/resources/appicon.iconset"
ICNS="${SCRIPT_DIR}/src/resources/appicon.icns"
mkdir -p "$ICONSET"

echo "Generating macOS icons..."

declare -A MACOS=(
  ["icon_16x16.png"]=16
  ["icon_16x16@2x.png"]=32
  ["icon_32x32.png"]=32
  ["icon_32x32@2x.png"]=64
  ["icon_128x128.png"]=128
  ["icon_128x128@2x.png"]=256
  ["icon_256x256.png"]=256
  ["icon_256x256@2x.png"]=512
  ["icon_512x512.png"]=512
  ["icon_512x512@2x.png"]=1024
)

for name in "${!MACOS[@]}"; do
  size="${MACOS[$name]}"
  echo "  ${size}x${size} → $name"
  render "$size" "${ICONSET}/${name}"
done

iconutil -c icns "$ICONSET" -o "$ICNS"
rm -rf "$ICONSET"

echo "macOS .icns → $ICNS"
echo ""
echo "Done."
