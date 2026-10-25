// =========================================================================
//   ZAKŁADKA: 6_atmega_ota.ino  (ATmega328P, MiniCore 3.x = bootloader URBOOT)   - BEZ ZMIAN
//
//   Po komendzie "!OTA_START!" z ESP:
//     1. sprawdza, czy w pamięci jest dokładnie ten bootloader, który znamy,
//     2. wywołuje otaPrzygotuj() (hak z 1_atmega.ino),
//     3. odsyła "OTA_ACK ..." (albo "OTA_NAK", gdy bootloader inny - wtedy NIE skacze),
//     4. przywraca stan jak po resecie (przerwania wyłączone, stos, UART, timery),
//     5. skacze do bootloadera W MIEJSCE PO TEŚCIE FLAGI EXTRF.
//
//   DLACZEGO TAK: urboot po resecie wchodzi w tryb wgrywania tylko przy fladze EXTRF (reset z pinu
//   RESET); programowo jej nie da się ustawić. Pierwsze instrukcje bootloadera (od START):
//       +0x00 clr r1 | +0x02 in r2,MCUSR | +0x04 out MCUSR,r1 | +0x06 ldi r24,0 | +0x08 rcall (wyłącz WDT)
//       +0x0A sbrs r2,1  <- test EXTRF | +0x0C rjmp -> aplikacja | +0x0E ldi r24,0x0E  <- tryb wgrywania
//   Skaczemy pod START+0x0E po odtworzeniu stanu z resetu sprzętowego. Adres START liczymy z tabeli
//   na szczycie flasha (liczba stron bootloadera), a kod jest porównywany z sygnaturą przed skokiem.
// =========================================================================
#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>

void otaPrzygotuj();                // hak zdefiniowany w 1_atmega.ino

#define BL_ENTRY_OFFSET 0x0E        // bajtów od początku bootloadera

static void p8(uint8_t b) {         // bajt jako 2 znaki hex
  if (b < 16) Serial.print('0');
  Serial.print(b, HEX);
}

void uruchomProcedureOTA() {
  // Tabela na szczycie flasha urboota (od góry): wersja, możliwości, rjmp (2 B), numer wektora VBL, liczba stron
  const uint8_t ver   = pgm_read_byte_near(FLASHEND);
  const uint8_t cap   = pgm_read_byte_near(FLASHEND - 1);
  const uint8_t vect  = pgm_read_byte_near(FLASHEND - 4);
  const uint8_t pages = pgm_read_byte_near(FLASHEND - 5);

  // Sprawdzenie sygnatury bootloadera (inaczej NIE skaczemy)
  bool sygnaturaOK = (pages >= 1 && pages <= 16);
  uint16_t start = 0;
  if (sygnaturaOK) {
    start = (uint16_t)(0x8000UL - (uint32_t)pages * 128UL);
    sygnaturaOK =
      pgm_read_word_near(start + 0x00) == 0x2411 &&    // clr r1
      pgm_read_word_near(start + 0x02) == 0xB624 &&    // in r2, MCUSR
      pgm_read_word_near(start + 0x0A) == 0xFE21 &&    // sbrs r2, 1
      pgm_read_word_near(start + 0x0C) == 0xC0EB &&    // rjmp do aplikacji
      pgm_read_word_near(start + 0x0E) == 0xE08E;      // ldi r24, 0x0E
  }
  if (!sygnaturaOK) {
    Serial.println(F("OTA_NAK"));
    Serial.flush();
    return;                                            // aplikacja działa dalej
  }

  otaPrzygotuj();                                      // hak projektu (wygaś wyjścia, OLED itp.)

  // Odpowiedź dla ESP: wektor 0, wektor VBL, numer wektora, strony, cap, wersja (hex)
  Serial.print(F("OTA_ACK V0="));
  for (uint8_t i = 0; i < 4; i++) p8(pgm_read_byte_near(i));
  Serial.print(F(" VN="));
  for (uint8_t i = 0; i < 4; i++) p8(pgm_read_byte_near((uint16_t)vect * 4 + i));
  Serial.print(F(" V="));  p8(vect);
  Serial.print(F(" P="));  p8(pages);
  Serial.print(F(" C="));  p8(cap);
  Serial.print(F(" R="));  p8(ver);
  Serial.println();
  Serial.flush();

  // Stan jak po resecie sprzętowym
  eeprom_busy_wait();
  cli();
  wdt_reset();
  wdt_disable();
  TIMSK0 = 0; TIMSK1 = 0; TIMSK2 = 0;
  TCCR0A = 0; TCCR0B = 0;
  TCCR1A = 0; TCCR1B = 0;
  TCCR2A = 0; TCCR2B = 0;
  TWCR = 0;
  ADCSRA = 0;
  SPCR = 0;
  UCSR0B = 0;                                          // UART wyłączony
  UCSR0A = 0;                                          // bez U2X (jak po resecie)
  UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);                  // 8N1 (domyślne po resecie)
  UBRR0H = 0; UBRR0L = 0;

  const uint16_t wejscieSlowa = (uint16_t)(start + BL_ENTRY_OFFSET) >> 1;   // adres słowa dla IJMP

  asm volatile(
    "cli                    \n\t"
    "clr  r1                \n\t"                      // rejestr zerowy
    "out  %[sreg], r1       \n\t"                      // SREG = 0
    "ldi  r24, %[sph_v]     \n\t"
    "out  %[sph], r24       \n\t"                      // SP = RAMEND
    "ldi  r24, %[spl_v]     \n\t"
    "out  %[spl], r24       \n\t"
    "ldi  r24, %[flag]      \n\t"
    "mov  r2, r24           \n\t"                      // r2 = "flagi resetu" (EXTRF)
    "ijmp                   \n\t"                      // Z = adres wejścia (słowa)
    :
    : [sreg]  "I" (_SFR_IO_ADDR(SREG)),
      [sph]   "I" (_SFR_IO_ADDR(SPH)),
      [spl]   "I" (_SFR_IO_ADDR(SPL)),
      [sph_v] "M" (RAMEND >> 8),
      [spl_v] "M" (RAMEND & 0xFF),
      [flag]  "M" (_BV(EXTRF)),
      [adr]   "z" (wejscieSlowa)
    : "r24"
  );
  while (1) {}
}
