#!/usr/bin/env bash
#
# prepare_content.sh — Copy and/or regenerate ASCII art content for the DSi build
#
# Usage:
#   ./tools/prepare_content.sh              # Copy existing 80-char art from outputs/
#   ./tools/prepare_content.sh --regen 32   # Regenerate art at 32-char width (fits DSi screen)
#
# Run from the dsi/ directory:
#   bash tools/prepare_content.sh
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DSI_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
ROOT_DIR="$(cd "$DSI_DIR/.." && pwd)"
ART_DIR="$DSI_DIR/nitrofiles/art"
OUTPUTS_DIR="$ROOT_DIR/outputs"
IMGS_DIR="$ROOT_DIR/imgs"
ASCIIFY="$ROOT_DIR/asciify.py"

# Parse arguments
REGEN_SIZE=""
if [[ "${1:-}" == "--regen" && -n "${2:-}" ]]; then
    REGEN_SIZE="$2"
    echo "Will regenerate ASCII art at width $REGEN_SIZE"
fi

mkdir -p "$ART_DIR"

if [[ -n "$REGEN_SIZE" ]]; then
    echo "Regenerating ASCII art at --size $REGEN_SIZE ..."
    echo ""

    # Map of image file -> output name -> conceal code
    # Adjust these to match your content
    declare -A ITEMS=(
        ["boot.png"]="2SHOO|2 SHOO"
        ["bread.jpg"]="BIGJO|BIG JO"
        ["chefhat.webp"]="CHEFHTN|CHEF HTN"
        ["dumpling.png"]="DMPL93|DMPL 93"
        ["hammock.jpg"]="HMCK25|HMCK 25"
        ["pickle.png"]="PCKL57|PCKL 57"
        ["tent.webp"]="TNT404|TNT 404"
    )

    for img in "${!ITEMS[@]}"; do
        IFS='|' read -r name code <<< "${ITEMS[$img]}"
        img_path="$IMGS_DIR/$img"
        out_path="$ART_DIR/${name}.txt"

        if [[ ! -f "$img_path" ]]; then
            echo "  SKIP: $img_path not found"
            continue
        fi

        echo "  $img -> ${name}.txt (code: \"$code\")"
        python3 "$ASCIIFY" "$img_path" \
            --size "$REGEN_SIZE" \
            --conceal "$code" \
            --difficulty hard \
            --save-txt "$out_path"
    done

    echo ""
    echo "Done! Art regenerated at width $REGEN_SIZE in $ART_DIR"
else
    echo "Copying existing art from $OUTPUTS_DIR ..."

    if [[ ! -d "$OUTPUTS_DIR" ]]; then
        echo "ERROR: $OUTPUTS_DIR not found. Generate art first with asciify.py."
        exit 1
    fi

    count=0
    for f in "$OUTPUTS_DIR"/*.txt; do
        cp "$f" "$ART_DIR/"
        count=$((count + 1))
        echo "  Copied: $(basename "$f")"
    done

    echo ""
    echo "Done! Copied $count art files to $ART_DIR"
fi

echo ""
echo "Next steps:"
echo "  1. Edit letter files in: $DSI_DIR/nitrofiles/letters/"
echo "  2. Update manifest:      $DSI_DIR/nitrofiles/manifest.txt"
echo "  3. Build:                cd $DSI_DIR && make"
