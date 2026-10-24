# OTA Template: ATmega328P + ESP8266 (ESP-01S)

**A 100nF capacitor connected to the ESP GPIO pin for ATmega reset is NOT required!**

```
Two small projects across tabs.  
  
They provide wireless program flashing to the ATmega via ESP and updating of the ESP itself.  
There is nothing more inside them.
```

# Important!!!

```
Compiled projects are configured for a 14.31818MHz crystal and a 447443 baud rate.  
If you are using a different crystal, you MUST! change the baud rate and crystal in MiniCore as well as the baud rate in the following files: 1_atmega.ino and 1_esp.ino  
-----------------------------------------------------------------------------  
  
1_atmega/1_atmega.ino  (main), Your code -> 6_atmega_ota.ino (DO NOT CHANGE)  
1_esp/1_esp.ino        (main), Your code -> 7_esp_ota-esp.ino (DO NOT CHANGE)  
tools/                 wgraj_usbasp.sh, flash the program to the ATmega if you do not have other flashing software  
  
In the MiniCore directory, there are modified files for the MiniCore package:   
boards.txt       - added support for 14.31818 MHz crystal and baud rates: 111861, 447443  
platform.txt     - added generation of .bin file without bootloader  
programmers.txt  - added support for the CH341A programmer - ONLY FOR CRYSTAL >= 8MHZ !!!!  
  
USING A CRYSTAL OTHER THAN 14.3 MHz?  
- change the baud rate and crystal in MiniCore as well as the baud rate in the files: 1_atmega.ino and 1_esp.ino
```

The project folder must have the same name as the main file (1_atmega, 1_esp), just like in your project.

## 1. Requirements

- Arduino IDE with the **MiniCore 3.1.3** board package (ATmega328, Bootloader: **Yes (UART0)**, Clock and Variant according to your project) and the ESP8266 board package.

- **WiFiManager** library (Library Manager) for the ESP.

- USBasp and avrdude (for the initial flashing of the ATmega). python3 is only needed if you use optional patching.

- ATmega and ESP connection: TX ESP → RX ATmega, RX ESP → TX ATmega, common ground, 3.3V / 5V level matching as usual.

- Connection diagram for the ATmega (kicad, pdf) is located in the **schemat&pic** directory.

## 2. One-time Setup/UPDATE of **MiniCore 3.1.3**

ENTER THE MiniCore DIRECTORY AND RUN: sh /update_mc.sh

## 3. Initial Run (once, via cables)

**ATmega (USBasp):**

1. Burn the bootloader (Tools → Burn Bootloader) if the chip does not have it yet. Fuse bits remain as they are (e.g., HFUSE 0xD7).

2. Compile the 1_atmega project, click **Sketch → Export Compiled Binary (Ctrl+Alt+S)**.

3. Flash the 1_atmega.ino.with_bootloader.hex file using: tools/wgraj_usbasp.sh path/to/1_atmega.ino.with_bootloader.hex (this is a standard avrdude -c usbasp -p m328p -e -U flash:w:FILE:i command). File patching **is not needed**: ATmega enters the bootloader by jumping from the program, and during the first OTA upload, the ESP handles the vectors itself.


**ESP (USB-UART cable, first time):**

1. **Important note:** The best option for future projects is replacing the 1MB flash memory with a 4MB one. If you have any old PC motherboard lying around, look for the BIOS chip — you might have such memory without even knowing it. Erase it before desoldering if you wish.

2. In the board settings, select flash size **1MB or 4MB with a file system - depending on what you have on board** and a scheme with an OTA partition.

3. Flash the 1_esp project.

4. After startup, the ESP creates an open network named **ESP_Konfiguracja**: connect to it, select your WiFi, and enter the password. The ESP will restart and receive an IP address from the router.

## 4. Every Subsequent ATmega Flashing: Wireless

1. Modify the code in 1_atmega.ino, separate/add project code into new tabs, compile, and click **Sketch → Export Compiled Binary**.

2. Open http://ESP_IP/upload_atmega_page in your web browser.

3. Choose the **1_atmega.ino.bin** file (the one without bootloader), click "Upload to ATmega" and wait a dozen seconds or so. A confirmation saying "OK! Wgrano … B." (OK! Uploaded ... B.) should appear.

4. On the main ESP page (http://ESP_IP/) there is also a tile named "Zaktualizuj ESP (.bin)" (Update ESP (.bin)) (/upload_esp_page): it uploads the .bin file for the ESP itself. The ESP can also be updated over the network from the Arduino IDE (mDNS hostname ESP-OTA).

## 5. How to Use the Template in a Different Program

**ATmega** (copy 6_atmega_ota.ino without changes and add four things from 1_atmega.ino into your main file):

1. \#define UART_BAUD … and Serial.begin(UART_BAUD); inside setup().

2. At the very beginning of setup(): MCUSR = 0; wdt_disable(); (after flashing, the bootloader might leave the watchdog enabled).

3. The function void otaPrzygotuj(): turn off outputs, save data, show a message (interrupts and I2C are still active at this point).

4. The function sprawdzKomendyZESP() called frequently inside loop(); inside it, if (r == "!OTA_START!") uruchomProcedureOTA();. Do not use long delay() calls inside the loop or block UART reception. If you use a watchdog, feed it inside the loop.

**ESP** (copy tabs 6 and 7 without changes and add the following to your main file): \#define OTA_ATMEGA, \#define OTA_HOSTNAME, the object ESP8266WebServer server(80);, prototypes for the three functions, inside setup() call setupAtmegaOTA(); setupEsp(); server.begin(); and inside loop() include obslugaArduinoOTA(); server.handleClient();.

**A rule for the lifetime of the project:** every program uploaded via OTA must include support for !OTA_START! and the 6_atmega_ota.ino tab. If you lose it, the next update will only be possible via USBasp.

## 6. Changing UART Speed or Crystal Frequency

- Speed: change UART_BAUD in both projects to the exact same value. The bootloader features autobaud, so it will adapt automatically. The default 447443 is tested and proven with a 14.31818 MHz crystal. Other speeds (e.g., 111861) work in the code, but their OTA functionality needs verification.

- Crystal: select the correct one in the MiniCore menu. The urboot bootloader does not depend on frequency (due to autobaud), and the OTA code does not have hardcoded frequencies.

- Different bootloader (e.g., Optiboot, another version of urboot): ATmega will not execute the jump, it will return OTA_NAK instead, and the web page will display this.

## 7. OTA Page Error Messages

| Message | What to do |
| - | - |
| "ATmega nie odpowiedziała OTA_ACK" (ATmega did not respond OTA_ACK) | Missing sprawdzKomendyZESP() in the loop or no !OTA_START! handling in code; mismatched UART_BAUD; incorrect TX/RX wiring. |
| "OTA_NAK … inny bootloader" (OTA_NAK ... different bootloader) | The ATmega does not contain the urboot bootloader from MiniCore 3.1.3 (different version). The signature in 6_atmega_ota.ino requires matching adjustments. |
| "… bootloader nie odpowiada na sync. Odebrano N B …" (... bootloader does not respond to sync. Received N B ...) | ATmega responded with ACK, but the bootloader is not responding. Check: identical UART_BAUD in both projects, power supply and common ground, TX/RX wiring. The ACK line should show P=03 and R=40 (urboot 8.0 bootloader). If this matches and the problem persists, re-flash the program via USBasp and try OTA again. |
| "To jest plik HEX" (This is a HEX file) | Upload the .bin file instead of the .hex file. |
| "Plik za duży … with_bootloader" (File too large ... with_bootloader) | You are attempting to upload a .with_bootloader.* file. Use the regular 1_atmega.ino.bin instead. |
| "Błąd zapisu strony pod adresem …" (Page write error at address ...) | UART connection was interrupted mid-process. Check power supply, ground, and cables, then try again. |


## 8. Limitations

- ESP does not verify the flashed program against the file after writing is complete.

- OTA via ESP requires a running program on the ATmega that handles the !OTA_START! command.

- If the ATmega fails to start after an unsuccessful flash, you can recover it using USBasp (wgraj_usbasp.sh).

- The OTA file must target the same urboot 8.0 bootloader (MiniCore 3.1.3), otherwise the ATmega will send an OTA_NAK response.

...

# Flashing the ATmega via the "Upload" button in Arduino IDE (over the network)

```
Following a one-time configuration (no modifications are needed inside the ESP or ATmega code).  
  
1. If you followed the steps above:  
  
2. Launch Arduino IDE. Requirements: ESP must be in the same network, and the `7_esp_ota-esp.ino` tab   
    must be active in the ESP (this tab handles advertising the ESP on the network under the name `ESP-OTA`).  
      
3. Select the MiniCore ATmega328 board (as usual), and under **Tools → Port**   
   select the network entry for the ESP (e.g., "ESP-OTA at 192.168.x.x").  
  
4. Click **Upload**. The IDE compiles, transmits the `.bin` file (without bootloader) to the ESP, and the ESP programs the ATmega.   
   The output console will display "OK! Wgrano … B." (OK! Uploaded ... B.).  
     
   - When using a serial interface (USB, hardware programmer), the Upload button functions as usual.   
   Selecting the ESP board using that same network port updates the ESP itself.  
     
   - If the ESP is missing from the port list: The IDE discovers it via mDNS (Linux: Avahi service,   
      firewall must not block UDP port 5353).   
        
      As a backup option, flash manually via terminal: `python3 …/3.1.3/tools/ota_atmega.py ESP_IP file.bin`.  
      - Windows: replace `python3` with `python` inside the lines starting with `tools.atmegaota…`.  
           
   - Using a board other than `328` within MiniCore: append an identical line `ID.upload.tool.network=atmegaota`   
      right below the corresponding `ID.upload.tool=` entry inside `boards.txt`.  
  
5. On the main ESP page (`http://ESP_IP/`) there is also a tile named "Zaktualizuj ESP (.bin)" (`/upload_esp_page`):   
    it uploads the `.bin` file for the ESP itself.   
    The ESP can also be updated over the network from the Arduino IDE (mDNS hostname `ESP-OTA`).
```