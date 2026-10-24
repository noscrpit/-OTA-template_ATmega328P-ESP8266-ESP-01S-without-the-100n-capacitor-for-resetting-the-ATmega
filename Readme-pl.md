# Szablon OTA: ATmega328P + ESP8266 (ESP-01S)

**NIe jest wymagany kondensator 100n podłączany do pinu GPIO ESP do resetu ategi!**

```
Dwa małe projekty na zakładkach.  
  
Dają bezpzewodowe wgrywanie programu do ATmegi  przez ESP i aktualizację samego ESP.  
Nie ma w nich nic poza tym.
```

# Ważne!!!

```
Skompilowane projekty są dla kwarcu 14,31818Mhz i baud 447443 .  
Jeśli używasz innego kwarcu MUSISZ! pozmienić baud i kwarc w Minicore oraz baud w plikach: 1\_atmega.ino i 1\_esp.ino  
-----------------------------------------------------------------------------  
  
1\_atmega/1\_atmega.ino  (główna), Twój kod -\> 6\_atmega\_ota.ino (NIE ZMIENIAJ)  
1\_esp/1\_esp.ino        (główna), Twój kod -\> 7\_esp\_ota-esp.ino (NIE ZMIENIAJ)  
narzedzia/         wgraj\_usbasp.sh, wgraj progam do atmegi jeśli nie masz innych progrmów do wgrywania  
  
W katalogu MiniCore, są pliki zmodyfikowane dla pakietu MiniCore:   
boards.txt       - dodana obsługa kwarcu  14.31818 MHz  i baud: 111861, 447443  
platform.txt     - dodane generowanie pliku .bin bez bootloadera  
programmers.txt  - dodana obsłuch programatora CH341A - TYLKO DLA KWARCU \>= 8MHZ !!!!  
  
UŻYWASZ INNY KWARC NIŻ 14,3 Mhz?  
- pozmieniaj baud i kwarc w Minicore oraz baud w plikach: 1\_atmega.ino i 1\_esp.ino
```

Folder projektu musi mieć nazwę taką jak plik główny (1\_atmega, 1\_esp), jak w Twoim projekcie.

## 1. Wymagania

- Arduino IDE z płytką **MiniCore 3.1.3** (ATmega328, Bootloader: **Yes (UART0)**, Clock i Variant jak w Twoim projekcie) oraz z płytką ESP8266.

- Biblioteka **WiFiManager** (Menedżer bibliotek) dla ESP.

- USBasp i avrdude (do pierwszego wgrania ATmegi). python3 tylko, jeśli użyjesz opcjonalnego patchowania

- ATmega i ESP: TX ESP → RX ATmegi, RX ESP → TX ATmegi, wspólna masa, poziomy 3,3 V / 5 V jak dotąd.

- Schemat podłaczenia do atmegi  (kicad, pdf)w katalogu **schemat&pic**

## 2. Jednorazowa konfiguracja/UPDATE  **MiniCore 3.1.3**

WEJDŹ DO KATALOGU MiniCore I WYKONAJ: sh /update\_mc.sh

## 3. Pierwsze uruchomienie (raz, przez kable)

**ATmega (USBasp):**

1. Wypal bootloader (Narzędzia → Wypal Bootloader'a), jeśli układ go jeszcze nie ma. Fusebity zostają jak są (np. HFUSE 0xD7).

2. Skompiluj projekt 1\_atmega, kliknij **Szkic → Eksportuj skompilowany plik binarny (Ctrl+Alt+S)**.

3. Wgraj plik 1\_atmega.ino.with\_bootloader.hex przez: narzedzia/wgraj\_usbasp.sh sciezka/1\_atmega.ino.with\_bootloader.hex (to zwykłe avrdude -c usbasp -p m328p -e -U flash:w:PLIK:i). Patchowanie pliku **nie jest potrzebne**: ATmega wchodzi do bootloadera skokiem z programu, a przy pierwszym wgraniu przez OTA ESP sam ustawia wektory.


**ESP (kabel USB-UART, pierwszy raz):**

1. **Ważna uwaga:** Najepszym wyjściem dla przyszłych projektów jest wymiana pamięci 1MB na 4Mb. Jeśli masz jakąś płytę główną od PC - poszukaj kości biosu - mozesz mieć taka pamięc i nie wiedzieć. Wykasuj ją przed wutowaniem jeśli chcesz.

2. W ustawieniach płytki wybierz flash **1MB lub 4 MB z systemem plików - zależy co masz na pokładzie** i układ z partycją OTA.

3. Wgraj projekt 1\_esp.

4. Po starcie ESP otwiera sieć **ESP\_Konfiguracja**: połącz się, wybierz swoje WiFi i hasło. ESP restartuje się i dostaje adres IP z routera.

## 4. Każde następne wgranie ATmegi: bez kabli

1. Zmień kod w 1\_atmega.ino, rozdziel/dodaj projekt na nowe zakadki, skompiluj, kliknij **Szkic → Eksportuj skompilowany plik binarny**.

2. W przeglądarce otwórz http://IP\_ESP/upload\_atmega\_page.

3. Wybierz plik **1\_atmega.ino.bin** (bez bootloadera), kliknij „Wgraj do ATmegi” i poczekaj kilkanaście sekund. Ma się pojawić „OK! Wgrano … B.”.

4. Na stronie głównej ESP (http://IP\_ESP/) jest też kafelek „Zaktualizuj ESP (.bin)” (/upload\_esp\_page): wgrywa plik .bin samego ESP. ESP można też aktualizować z Arduino IDE przez sieć (nazwa hosta ESP-OTA).

## 5. Jak użyć szablonu w innym programie

**ATmega** (kopiujesz 6\_atmega\_ota.ino bez zmian i dopisujesz w swoim pliku głównym cztery rzeczy z 1\_atmega.ino):

1. \#define UART\_BAUD … i Serial.begin(UART\_BAUD); w setup().

2. Na samym początku setup(): MCUSR = 0; wdt\_disable(); (po wgraniu bootloader może zostawić włączony watchdog).

3. Funkcja void otaPrzygotuj(): wygaś wyjścia, zapisz dane, pokaż komunikat (przerwania i I2C jeszcze działają).

4. Funkcja sprawdzKomendyZESP() wywoływana często w loop(); w niej if (r == "!OTA\_START!") uruchomProcedureOTA();. Nie używaj długich delay() w pętli ani nie blokuj odbioru UART. Jeśli używasz watchdoga, karm go w pętli.

**ESP** (kopiujesz zakładki 6 i 7 bez zmian i dopisujesz w pliku głównym): \#define OTA\_ATMEGA, \#define OTA\_HOSTNAME, obiekt ESP8266WebServer server(80);, prototypy trzech funkcji, w setup() wywołania setupAtmegaOTA(); setupEsp(); server.begin(); oraz w loop() obslugaArduinoOTA(); server.handleClient();.

**Zasada na całe życie projektu:** każdy program wgrywany przez OTA musi zawierać obsługę !OTA\_START! i zakładkę 6\_atmega\_ota.ino. Jeśli ją zgubisz, kolejną aktualizację zrobisz tylko przez USBasp.

## 6. Zmiana prędkości UART albo kwarcu

- Prędkość: zmień UART\_BAUD w obu projektach na tę samą wartość. Bootloader ma autobaud, więc dostosuje się sam. Domyślne 447443 jest sprawdzone z kwarcem 14,31818 MHz. Inne prędkości (np. 111861) działają w kodzie, ale ich OTA trzeba sprawdzić.

- Kwarc: wybierz właściwy w menu MiniCore. Bootloader urboot nie zależy od częstotliwości (autobaud), a kod OTA nie ma zapisanych częstotliwości.

- Inny bootloader (np. Optiboot, inna wersja urboota): ATmega nie skoczy, odeśle OTA\_NAK, a strona to pokaże.

## 7. Komunikaty błędów strony OTA

| Komunikat | Co zrobić |
| - | - |
| „ATmega nie odpowiedziała OTA\_ACK” | Brak sprawdzKomendyZESP() w pętli albo !OTA\_START! w kodzie; różne UART\_BAUD; złe okablowanie TX/RX. |
| „OTA\_NAK … inny bootloader” | W ATmedze nie ma urboota z MiniCore 3.1.3 (inna wersja). Sygnatura w 6\_atmega\_ota.ino wymaga dopasowania. |
| „… bootloader nie odpowiada na sync. Odebrano N B …” | ATmega odpowiedziała ACK, ale bootloader nie odpowiada. Sprawdź: ten sam UART\_BAUD w obu projektach, zasilanie i masę, okablowanie TX/RX. W linii ACK powinno być P=03 i R=40 (bootloader urboot 8.0). Jeśli tak jest i problem się powtarza, wgraj ponownie program przez USBasp i spróbuj OTA jeszcze raz. |
| „To jest plik HEX” | Wgraj plik .bin, nie .hex. |
| „Plik za duży … with\_bootloader” | Wgrywasz plik .with\_bootloader.\*. Użyj zwykłego 1\_atmega.ino.bin. |
| „Błąd zapisu strony pod adresem …” | Przerwa w UART w trakcie. Sprawdź zasilanie, masę i kable, i spróbuj ponownie. |


## 8. Ograniczenia

- ESP nie porównuje wgranego programu z plikiem po zapisie.

- OTA przez ESP wymaga, żeby w ATmedze działał program z obsługą !OTA\_START!.

- Jeśli po nieudanym wgraniu ATmega nie startuje, odzyskasz ją przez USBasp (wgraj\_usbasp.sh).

- Plik OTA musi pochodzić z tego samego bootloadera urboot 8.0 (MiniCore 3.1.3), inaczej ATmega odeśle OTA\_NAK.

...

# Wgrywanie do Atmegi przyciskiem „Prześlij” w Arduino IDE (przez sieć)

```
Po jednorazowej instalacji (nie trzeba zmieniać nic w kodzie ESP ani ATmegi.  
  
1. Jeśli zrobiłes j/w to:  
  
2. Uruchom Arduino IDE. Wymagane: ESP w tej samej sieci, w ESP działa zakładka \`7\_esp\_ota-esp.ino\`   
    (to ona ogłasza ESP w sieci, nazwa \`ESP-OTA\`).  
      
3. Wybierz płytkę MiniCore ATmega328 (jak zawsze), a w \*\*Narzędzia → Port\*\*   
   wybierz pozycję sieciową ESP (np. „ESP-OTA at 192.168.x.x”).  
  
4. Kliknij \*\*Prześlij\*\*. IDE kompiluje, wysyła plik \`.bin\` (bez bootloadera) do ESP, a ESP programuje ATmegę.   
   W oknie wyniku zobaczysz „OK! Wgrano … B.”.  
     
   - Przy porcie szeregowym (USB, programator) Prześlij działa jak dotąd.   
   Wybór płytki ESP z tym samym portem sieciowym aktualizuje ESP.  
     
   - Gdy ESP nie ma na liście portów: IDE wykrywa go przez mDNS (Linux: usługa Avahi,   
      firewall nie może blokować UDP 5353).   
        
      Awaryjnie wgraj ręcznie: \`python3 …/3.1.3/tools/ota\_atmega.py IP\_ESP plik.bin\`.  
      - Windows: w liniach \`tools.atmegaota…\` zamień \`python3\` na \`python\`.  
           
   - Inna płytka niż \`328\` w MiniCore: dodaj taką samą linię \`ID.upload.tool.network=atmegaota\`   
      pod odpowiednią linią \`ID.upload.tool=\` w \`boards.txt\`.  
  
5. Na stronie głównej ESP (\`http://IP\_ESP/\`) jest też kafelek „Zaktualizuj ESP (.bin)” (\`/upload\_esp\_page\`):   
    wgrywa plik \`.bin\` samego ESP.   
    ESP można też aktualizować z Arduino IDE przez sieć (nazwa hosta \`ESP-OTA\`).
```

