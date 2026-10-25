// =========================================================================
//   ZAKŁADKA: 6_esp_ota-atmega.ino  (projekt ESP8266)
//   Wgrywanie .bin do ATmegi328P przez UART, bootloader URBOOT (MiniCore 3.x),
//   protokół URPROTOCOL (nie STK500!). Bez pinu RESET:
//     ESP -> "!OTA_START!"  ->  ATmega odsyła "OTA_ACK ..." i resetuje się
//     z flagą EXTRF -> urboot czeka na sync (autobaud, ok. 1 s) -> zapis stron.
// =========================================================================
#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

#ifdef OTA_ATMEGA

extern ESP8266WebServer server;

// ------------------------------ KONFIGURACJA -----------------------------
#define PAGE_SIZE       128       // ATmega328P: strona flash = 128 B
#define OTA_FILE        "/atmega.bin"

// Urprotocol
#define UR_GET_SYNC     0x30
#define UR_EOP          0x20
#define UR_PROG_PAGE_FL 0x02
#define UR_LEAVE_PROG   0x51

static File otaFile;

// --------------------------- STRONA WWW (PROGMEM) -------------------------
static const char STRONA_OTA[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset='utf-8'>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<title>OTA ATmega</title><style>
body{font-family:Arial;background:#121212;color:#fff;text-align:center;padding-top:40px}
.box{margin:30px auto;background:#1e1e1e;max-width:420px;padding:30px;border-radius:10px}
.bg{width:100%;background:#333;border-radius:10px;height:25px;margin:20px 0;display:none;overflow:hidden}
.bar{width:0%;background:#00adb5;height:100%;line-height:25px;font-weight:bold}
.btn{padding:10px 20px;background:#00adb5;color:#fff;border:none;border-radius:4px;font-weight:bold}
a{color:#00adb5}
</style></head><body>
<h2>Wgrywanie programu do ATmegi (.bin)</h2>
<div class='box'>
<input type='file' id='f' accept='.bin'><br><br>
<button class='btn' onclick='go()'>Wgraj do ATmegi</button>
<div class='bg' id='bg'><div class='bar' id='bar'>0%</div></div>
<div id='st' style='font-weight:bold;color:#00adb5'></div>
</div><a href='/'>Powrót</a>
<script>
function go(){
  var fi=document.getElementById('f');
  if(!fi.files.length){alert('Wybierz plik .bin');return;}
  var bg=document.getElementById('bg'),bar=document.getElementById('bar'),st=document.getElementById('st');
  bg.style.display='block';
  var fd=new FormData();
  fd.append('update',fi.files[0]);            /* <- files[0], a nie cała lista! */
  var x=new XMLHttpRequest();
  x.upload.onprogress=function(e){
    if(e.lengthComputable){var p=Math.round(e.loaded*100/e.total);
      bar.style.width=p+'%';bar.innerHTML=p+'%';st.innerHTML='Wysyłanie do ESP: '+p+'%';}
  };
  x.upload.onload=function(){st.innerHTML='Plik przesłany na ESP. <br>Programowanie ATmegi (kilkanaście sekund)...';};
  x.onload=function(){st.innerHTML=x.responseText;};
  x.onerror=function(){st.innerHTML='Błąd połączenia';};
  x.open('POST','/do_upload_atmega',true);
  x.send(fd);
}
</script></body></html>
)rawliteral";

// ------------------------------ DIAGNOSTYKA ------------------------------
static uint8_t dbgBuf[12];
static uint8_t dbgN = 0;

static int czytaj() {                       // czyta bajt i loguje pierwsze 12 bajtów
  int c = Serial.read();
  if (c >= 0 && dbgN < sizeof(dbgBuf)) dbgBuf[dbgN++] = (uint8_t)c;
  return c;
}
static void oproznijRx() { while (Serial.available()) czytaj(); }
static String dbgHex() {
  String s = "Odebrano " + String(dbgN) + " B:";
  for (uint8_t i = 0; i < dbgN; i++) {
    s += " ";
    if (dbgBuf[i] < 16) s += "0";
    s += String(dbgBuf[i], HEX);
  }
  return s;
}

// ------------------------------ DANE Z ATMEGI ----------------------------
struct InfoBL {
  uint8_t v0[4];     // aktualny wektor 0 w ATmedze (skok do bootloadera)
  uint8_t vn[4];     // zawartość slotu wektora VBL
  uint8_t vect;      // numer wektora VBL (SPM_READY = 25 = 0x19)
  uint8_t pages;     // liczba stron bootloadera
  uint8_t cap, ver;
};
static InfoBL bl;

static bool pobierzPole(const String &s, const char *klucz, String &out) {
  int i = s.indexOf(klucz);
  if (i < 0) return false;
  i += strlen(klucz);
  int j = s.indexOf(' ', i);
  if (j < 0) j = s.length();
  out = s.substring(i, j);
  out.trim();
  return true;
}
static bool hexNaBajty(const String &h, uint8_t *dst, size_t n) {
  if (h.length() != n * 2) return false;
  for (size_t i = 0; i < n; i++) {
    char t[3] = { h[2 * i], h[2 * i + 1], 0 };
    char *e;
    long v = strtol(t, &e, 16);
    if (*e) return false;
    dst[i] = (uint8_t)v;
  }
  return true;
}
static bool parsujInfo(const String &l) {
  String s;
  if (!pobierzPole(l, "V0=", s) || !hexNaBajty(s, bl.v0, 4)) return false;
  if (!pobierzPole(l, "VN=", s) || !hexNaBajty(s, bl.vn, 4)) return false;
  if (!pobierzPole(l, "V=",  s) || !hexNaBajty(s, &bl.vect, 1)) return false;
  if (!pobierzPole(l, "P=",  s) || !hexNaBajty(s, &bl.pages, 1)) return false;
  if (!pobierzPole(l, "C=",  s) || !hexNaBajty(s, &bl.cap, 1)) return false;
  if (!pobierzPole(l, "R=",  s) || !hexNaBajty(s, &bl.ver, 1)) return false;
  return true;
}

// ------------------------- WEJŚCIE W BOOTLOADER --------------------------
// Wysyła "!OTA_START!" (powtarza rzadko, tylko do pierwszego fragmentu ACK;
// bajty wysłane do bootloadera, który ich nie rozumie, restartują go).
static bool wejdzWBootloader(String &linia) {
  String rx = "";
  uint32_t t0 = millis(), tSend = 0;
  bool pierwszy = true;
  while (millis() - t0 < 4000) {
    if (rx.indexOf("OTA_ACK") < 0 && (pierwszy || millis() - tSend > 700)) {
      Serial.print(F("!OTA_START!\n"));
      Serial.flush();
      tSend = millis();
      pierwszy = false;
    }
    while (Serial.available()) {
      char c = Serial.read();
      rx += c;
      if (c == '\n' && rx.indexOf("OTA_NAK") >= 0) {
        linia = "NAK";                      // ATmega: nieznany bootloader, nie skoczyła
        return false;
      }
      if (c == '\n' && rx.indexOf("OTA_ACK") >= 0) {
        int i = rx.indexOf("OTA_ACK");
        linia = rx.substring(i);
        linia.trim();
        return true;
      }
      if (rx.length() > 300 && rx.indexOf("OTA_ACK") < 0) rx.remove(0, 150);
    }
    yield();
  }
  return false;
}

// Synchronizacja urprotocol: INSYNC/OK nie są stałe, więc uczymy się ich z
// dwóch kolejnych identycznych odpowiedzi (różne bajty, 2 sztuki).
static bool synchronizuj(uint8_t &ins, uint8_t &ok) {
  int pI = -1, pO = -1;
  uint32_t t0 = millis();
  while (millis() - t0 < 1500) {
    oproznijRx();
    Serial.write((uint8_t)UR_GET_SYNC);
    Serial.write((uint8_t)UR_EOP);
    uint8_t b[2];
    uint8_t n = 0;
    uint32_t t1 = millis();
    while (millis() - t1 < 40 && n < 2) {
      if (Serial.available()) b[n++] = (uint8_t)czytaj();
      else yield();
    }
    if (n == 2 && b[0] != b[1]) {
      if (pI == b[0] && pO == b[1]) {
        ins = b[0];
        ok = b[1];
        delay(5);
        oproznijRx();
        return true;
      }
      pI = b[0];
      pO = b[1];
    } else {
      pI = pO = -1;
    }
    delay(5);
  }
  return false;
}

static bool czekajUR(uint8_t ins, uint8_t ok, uint32_t timeoutMs) {
  uint32_t t0 = millis();
  uint8_t stan = 0;
  while (millis() - t0 < timeoutMs) {
    while (Serial.available()) {
      uint8_t c = (uint8_t)czytaj();
      if (stan == 0 && c == ins) stan = 1;
      else if (stan == 1) {
        if (c == ok) return true;
        stan = (c == ins) ? 1 : 0;
      }
    }
    yield();
  }
  return false;
}

// Bootloader wektorowy: wektor 0 ma skakać do bootloadera, a oryginalny skok
// do startu aplikacji ma leżeć w wektorze VBL (SPM_READY). Robimy to samo,
// co avrdude -c urclock.
static bool patchujStrone0(uint8_t *p, String &msg) {
  if (p[1] != 0x94 || (p[0] & 0x0F) != 0x0C) {
    msg = "Wektor 0 obrazu nie jest instrukcją JMP (obraz nie z avr-gcc dla 328P?)";
    return false;
  }
  uint8_t orig[4] = { p[0], p[1], p[2], p[3] };

  uint16_t startSlow = 0x4000 - (uint16_t)bl.pages * (PAGE_SIZE / 2);   // adres bootloadera w słowach
  uint8_t nowyV0[4];
  bool chipJmp = (bl.v0[1] == 0x94 && (bl.v0[0] & 0x0F) == 0x0C);
  uint16_t cel = bl.v0[2] | ((uint16_t)bl.v0[3] << 8);
  if (chipJmp && cel >= startSlow && cel < 0x4000) {
    memcpy(nowyV0, bl.v0, 4);                // użyj dokładnie tego, co jest w chipie
  } else {
    nowyV0[0] = 0x0C; nowyV0[1] = 0x94;
    nowyV0[2] = startSlow & 0xFF; nowyV0[3] = startSlow >> 8;
  }
  memcpy(p + (uint16_t)bl.vect * 4, orig, 4);
  memcpy(p, nowyV0, 4);
  return true;
}

// ------------------------------ PROGRAMOWANIE ----------------------------
static bool programuj(String &msg) {
  File f = LittleFS.open(OTA_FILE, "r");
  if (!f) { msg = "Brak pliku na ESP (upload się nie udał)"; return false; }

  size_t rozmiar = f.size();
  if (rozmiar < 128) { msg = "Plik za mały / pusty"; f.close(); return false; }

  int b0 = f.read();
  f.seek(0);
  if (b0 == ':') {
    msg = "To jest plik HEX (tekst). <br><br>Potrzebny surowy .bin";
    f.close(); return false;
  }

  // 1. Komenda do aplikacji ATmegi -> ACK + reset z EXTRF
  String linia = "";
  oproznijRx();
  if (!wejdzWBootloader(linia)) {
    if (linia == "NAK")
      msg = "ATmega odpowiedziała OTA_NAK: sygnatura bootloadera w jej pamięci jest inna niż zakładam (inna wersja urboota?)";
    else
      msg = "ATmega nie odpowiedziała OTA_ACK <br><br> -brak obsługi !OTA_START! w kodzie ATmegi?";
    f.close(); return false;
  }
  if (!parsujInfo(linia) || bl.pages < 1 || bl.pages > 16 || bl.vect < 1 || bl.vect > 31) {
    msg = "Nieprawidłowe dane od ATmegi (to na pewno urboot?): " + linia;
    f.close(); return false;
  }
  uint32_t appMax = 32768UL - (uint32_t)bl.pages * PAGE_SIZE;
  if (rozmiar > appMax) {
    msg = "Plik za duży (" + String(rozmiar) + " B, max " + String(appMax) +
          " B). <br>Pewnie to wersja 'with_bootloader' <br> - wgraj zwykły .bin BEZ bootloadera.";
    f.close(); return false;
  }

  // 2. Synchronizacja z bootloaderem (autobaud, ok. 1 s okna)
  dbgN = 0;
  delay(20);
  uint8_t ins = 0, ok = 0;
  if (!synchronizuj(ins, ok)) {
    msg = "ATmega odpowiedziała ACK, ale bootloader nie odpowiada na sync. " + dbgHex() +
          " | ACK: " + linia;
    f.close(); return false;
  }

  // 3. Strona po stronie (adres = adres BAJTOWY, długość = 1 bajt)
  uint16_t adres = 0;
  uint8_t bufor[PAGE_SIZE];
  while (f.available()) {
    memset(bufor, 0xFF, sizeof(bufor));
    size_t n = f.read(bufor, PAGE_SIZE);
    if (n == 0) break;

    if (adres == 0 && !patchujStrone0(bufor, msg)) { f.close(); return false; }

    Serial.write((uint8_t)UR_PROG_PAGE_FL);
    Serial.write((uint8_t)(adres & 0xFF));
    Serial.write((uint8_t)(adres >> 8));
    Serial.write((uint8_t)PAGE_SIZE);
    Serial.write(bufor, PAGE_SIZE);
    Serial.write((uint8_t)UR_EOP);
    if (!czekajUR(ins, ok, 800)) {
      msg = "Błąd zapisu strony pod adresem 0x" + String(adres, HEX) + ". " + dbgHex();
      f.close(); return false;
    }
    adres += PAGE_SIZE;
    yield();
  }

  // 4. Koniec - bootloader skraca watchdog i startuje aplikację
  Serial.write((uint8_t)UR_LEAVE_PROG);
  Serial.write((uint8_t)UR_EOP);
  czekajUR(ins, ok, 300);
  f.close();

  msg = "OK! Wgrano " + String(rozmiar) + " B. <br>ATmega startuje nowy program.";
  return true;
}

static bool flashujAtmege(String &msg) {
  bool ok = programuj(msg);
  Serial.flush();
  LittleFS.remove(OTA_FILE);
  return ok;
}

// --------------------------------- HANDLERY ------------------------------
void handleUploadAtmegaPage() {
  server.send_P(200, PSTR("text/html"), STRONA_OTA);
}

// Wywoływany PO zakończeniu uploadu. Programujemy i odsyłamy wynik.
void handleDoUploadAtmega() {
  String wynik;
  bool ok = flashujAtmege(wynik);
  server.sendHeader("Connection", "close");
  server.send(ok ? 200 : 500, "text/plain; charset=utf-8", wynik);
}

// Tylko zapis pliku do LittleFS (bez żadnego programowania tutaj).
void handleAtmegaStreaming() {
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    LittleFS.remove(OTA_FILE);
    otaFile = LittleFS.open(OTA_FILE, "w");
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (otaFile && upload.currentSize > 0) otaFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (otaFile) otaFile.close();
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (otaFile) otaFile.close();
    LittleFS.remove(OTA_FILE);
  }
}

void setupAtmegaOTA() {
  server.on("/upload_atmega_page", handleUploadAtmegaPage);
  server.on("/do_upload_atmega", HTTP_POST, handleDoUploadAtmega, handleAtmegaStreaming);
}

#else
void setupAtmegaOTA() {}
#endif // OTA_ATMEGA
