#!/bin/bash
# Wgrywanie ATmegi przez USBasp (z kasowaniem flasha, EEPROM zostaje).
# Użycie:  ./wgraj_usbasp.sh sciezka/do/1_atmega.ino.with_bootloader.hex
set -e
[ -z "$1" ] && { echo "Podaj plik .with_bootloader.hex"; exit 1; }
avrdude -c usbasp -p m328p -e -U flash:w:"$1":i
