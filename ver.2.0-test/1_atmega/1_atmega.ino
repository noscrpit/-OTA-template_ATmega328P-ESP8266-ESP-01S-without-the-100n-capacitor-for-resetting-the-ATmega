// =========================================================================
//  SZABLON OTA - ATmega328P (MiniCore + bootloader urboot)   ZAKŁADKA GŁÓWNA: 1_atmega.ino
//  Zakładka: 6_atmega_ota.ino  (wejście w bootloader na komendę "!OTA_START!" z ESP)
//  Tu dopisujesz własny program. Trzy rzeczy są OBOWIĄZKOWE (oznaczone "OBOWIĄZKOWE").
// =========================================================================
#include <Arduino.h>
#include <avr/wdt.h>

#define UART_BAUD 447443            // MUSI być identyczne jak UART_BAUD w ESP (1_esp.ino)

const uint8_t PIN_LED = 13;         // przykład: dioda (PB5)

// Prototypy z zakładki OTA
void uruchomProcedureOTA();         // 6_atmega_ota.ino

// OBOWIĄZKOWE: hak wywoływany tuż przed wejściem w bootloader (przerwania i I2C jeszcze działają).
// Tu wyłącz wyjścia, zapisz dane do EEPROM, pokaż komunikat na wyświetlaczu itp.
void otaPrzygotuj() {
  digitalWrite(PIN_LED, LOW);
  // np. zgaś paski LED, wyświetl "AKTUALIZACJA" na OLED-zie
}

// OBOWIĄZKOWE: odbiór komend z ESP. Linia zakończona znakiem nowej linii.
void sprawdzKomendyZESP() {
  if (Serial.available() > 0) {
    String r = Serial.readStringUntil('\n');
    r.trim();
    if (r == "!OTA_START!") {
      uruchomProcedureOTA();        // NIE WRACA (skok do bootloadera); wraca tylko gdy bootloader nieznany
    }
    // else if (r.startsWith("MOJA_KOMENDA")) { ... }   // <-- własne komendy z ESP
  }
}

void setup() {
  MCUSR = 0;                        // OBOWIĄZKOWE: po wgraniu bootloader zostawia włączony watchdog -
  wdt_disable();                    //              wyłącz go na samym początku
  Serial.begin(UART_BAUD);
  pinMode(PIN_LED, OUTPUT);
  // TUTAJ własna inicjalizacja
}

void loop() {
  sprawdzKomendyZESP();             // OBOWIĄZKOWE: wywołuj często (bez długich delay() w pętli)

  // --- PRZYKŁAD: nieblokujące miganie diodą (zastąp własnym kodem) ---
  static uint32_t t = 0;
  if (millis() - t > 500) { t = millis(); digitalWrite(PIN_LED, !digitalRead(PIN_LED)); }
}
