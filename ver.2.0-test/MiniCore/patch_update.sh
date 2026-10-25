#!/bin/bash
# Dodaje do MiniCore wgrywanie przez sieć (ESP) - przycisk "Prześlij" w Arduino IDE.
# Skrypt: robi kopie *.ota.bak, kopiuje tools/ota_atmega.py, dopisuje narzędzie do platform.txt
# i linię "328.upload.tool.network=atmegaota" do boards.txt. Można uruchamiać wielokrotnie.

# Adds network uploading (ESP) to MiniCore—the "Upload" button in the Arduino IDE.
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

VER="3.1.3"
if [ "`basename "$DEST"`" != "$VER" ] ; then echo " Sorry! - Bad version of MiniCore - expected "$VER" . I'm pausing the operation ... "; exit 1; fi


    for plik in "boards.txt" "platform.txt" "programmers.txt" ; do
        if [ -f "$DEST/$plik" ]; then
            echo "$plik - exists, OK."
        else
            echo  "File not found ->  "$DEST/$plik""
            exit 1
        fi
    done

echo ""
echo "Ctrl C    <- to stop"
echo "----------------------------------------"
printf "Start of installation in: "; for i in {10..1}; do printf "%d " "$i"; sleep 1; done; echo ""

mkdir -vp "$DEST/tools"
cp -vf --backup=numbered ota_atmega.py "$DEST/tools/ota_atmega.py"; chmod +x "$DEST/tools/ota_atmega.py"
cp -vf --backup=numbered  minicore_add_boards.py programmers.diff  platform.diff  "$DEST"


 cd "$DEST"

 echo "
 Patching file: programmers.txt - platform.txt
 --------------------------------"
patch -p1 <  programmers.diff programmers.txt && patch -p1 <  platform.diff platform.txt

# add ota
        if grep -q "tools.atmegaota.upload.pattern" "platform.txt"; then
              echo "platform.txt: mod already added"
        else cat "$HERE/platform_ota_dodatek.txt" >> "platform.txt"
              echo "platform.txt: The atmegaota tool has been added"
        fi

if grep -q "^328.upload.tool.network=" "boards.txt"; then
echo "boards.txt: mod already added"
else
  grep -q "^328.upload.tool=" "boards.txt"
  sed -i 's/^328\.upload\.tool=.*$/&\n328.upload.tool.network=atmegaota/' "boards.txt"
  echo "boards.txt: 328.upload.tool.network  was added"
  fi

 echo "
 Create, check and patching boards.txt
 --------------------------------"
 python3 minicore_add_boards.py boards.txt && patch -p1 --dry-run < minicore_3.1.3_328p_14M31818_25M.diff  && patch -p1 < minicore_3.1.3_328p_14M31818_25M.diff || { echo "Checking the boards patch—overlay error!" ;  exit 1; } && echo "... done"

echo "
All done - ota and new boards added. Start or close and restart the Arduino IDE.
"
exit 0
