
Wersja 3.1.3 MiniCore, przekopiuj pliki do:

.arduino15/packages/MiniCore/hardware/avr/3.1.3/

 z zachowaniem  kopii oryginałów  - inne wersje MiniCore - dopasuj w plikach
----------------------------------------------------------------------------------------------

boards.txt              - dodana obsługa kwarcu 14,31818Mhz i baud 111861, 447443
platform.txt            - dodane budowanie .bin bez bootloadera
programmers.txt      - dodana obsługa CH341A   - tylko dla kwarcu >= 8 Mhz !!!
ota_atmega.py        - wsparcie ota dla atmegi przez esp

Read.txt - ten plik :)

=================== en -------------------

MiniCore version 3.1.3: Copy the files to:

.arduino15/packages/MiniCore/hardware/avr/3.1.3/

while keeping copies of the originals - for other MiniCore versions, adjust the files accordingly
------------------------ ----------------------------------------------------------------------

boards.txt              - Added support for 14.31818 MHz crystal and baud rates 111,861 and 447,443
platform.txt            - Added .bin build without bootloader
programmers.txt      - Added support for CH341A   - only for crystals >= 8 MHz !!!
ota_atmega.py        - OTA support for Atmega via ESP

Read.txt - this file :)

Have fun!
