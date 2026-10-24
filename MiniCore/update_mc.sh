#!/bin/bash
# Dodaje do MiniCore wgrywanie przez sieć (ESP) - przycisk "Prześlij" w Arduino IDE.
# Użycie:  ./update_mc.sh /ścieżka_do_katalogu/.arduino15/packages/MiniCore/hardware/avr/3.1.3
# Skrypt: robi kopie *.ota.bak, kopiuje tools/ota_atmega.py, dopisuje narzędzie do platform.txt
# i linię "328.upload.tool.network=atmegaota" do boards.txt. Można uruchamiać wielokrotnie.

# Adds network uploading (ESP) to MiniCore—the "Upload" button in the Arduino IDE.
# Usage:  ./update_mc.sh /path_to_directory/.arduino15/packages/MiniCore/hardware/avr/3.1.3
# Script: creates *.ota.bak backup files, copies tools/ota_atmega.py, adds the tool to platforms.txt
# and adds the line "328.upload.tool.network=atmegaota" to boards.txt. Can be run multiple times.

# clear
set -e

DEST="${1:?Specify the MiniCore folder (containing the boards.txt, platforms.txt, and programmers.txt files)}"
HERE="$(dirname "$(readlink -f "$0")")"

echo "
Specified_directory=$DEST
Current_directory=$HERE
Current_short= "`basename $HERE` "
"

# VER="3.1.3"
# if [ "`basename "$DEST"`" != "$VER" ]; then echo " Sorry! - Bad version of MiniCore - expected $VER. I'm pausing the operation ... "; exit 1; fi

for plik in "platform.txt" "boards.txt" "programmers.txt"; do
    if [ ! -f "$DEST/$plik" ]; then
        echo "
        File missing: $DEST/$plik
        Is this definitely a good catalog? — Check it out!

        "
        exit 1
    fi
done
echo "Ctrl C    <- to stop"
echo "----------------------------------------"
printf "Start of installation in: "; for i in {10..1}; do printf "%d " "$i"; sleep 1; done; echo ""

mkdir -vp "$DEST/tools"
cp -vf --backup=numbered "ota_atmega.py" "$DEST/tools/ota_atmega.py"
chmod +x "$DEST/tools/ota_atmega.py"
cp -vf --backup=numbered boards.txt platform.txt  programmers.txt  $DEST
if grep -q "tools.atmegaota.upload.pattern" "$DEST/platform.txt"; then echo "platform.txt: mod already added"; else cat "$HERE/platform_ota_dodatek.txt" >> "$DEST/platform.txt"; echo "platform.txt: The atmegaota tool has been added"; fi
if grep -q "^328.upload.tool.network=" "$DEST/boards.txt"; then echo "boards.txt: mod already added"; else
  grep -q "^328.upload.tool=" "$DEST/boards.txt" || { echo "boards.txt: line 328 is missing.upload.tool="; exit 1; }
  sed -i 's/^328\.upload\.tool=.*$/&\n328.upload.tool.network=atmegaota/' "$DEST/boards.txt"; echo "boards.txt: 328.upload.tool.network  was added"; fi
echo "All done. Start or close and restart the Arduino IDE."
exit 0
