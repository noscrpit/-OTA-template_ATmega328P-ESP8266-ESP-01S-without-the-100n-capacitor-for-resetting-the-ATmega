// =========================================================================
//  SZABLON OTA - ESP8266 (ESP-01S)          ZAKŁADKA GŁÓWNA: 1_esp.ino
//  Zakładki: 6_esp_ota-atmega.ino (wgrywanie programu do ATmegi przez UART)
//            7_esp_ota-esp.ino    (aktualizacja samego ESP)
//  Strona główna: kafelki do obu stron OTA. Tu dopisujesz własny kod projektu.
// =========================================================================
#include <Arduino.h>

#define OTA_ATMEGA                 // ZOSTAW: włącza wgrywanie programu do ATmegi (zakładka 6)
#define UART_BAUD  447443          // MUSI być identyczne jak UART_BAUD w ATmedze (1_atmega.ino)
#define OTA_HOSTNAME "ESP-OTA"     // nazwa sieciowa ESP (ArduinoOTA z Arduino IDE)

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>           // biblioteka WiFiManager (Menedżer bibliotek)
#include <LittleFS.h>

ESP8266WebServer server(80);       // zakładki 6 i 7 korzystają z tego obiektu

// Prototypy funkcji z zakładek OTA
void setupAtmegaOTA();             // 6_esp_ota-atmega.ino
void setupEsp();                   // 7_esp_ota-esp.ino
void obslugaArduinoOTA();          // 7_esp_ota-esp.ino

// ---------------------------- STRONA GŁÓWNA ------------------------------

void handleRoot() {
  String h = F("<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
               "<title>Szablon OTA</title><style>"
               "body{font-family:Arial;background:#121212;color:#fff;text-align:center;padding:30px}"
               ".t{display:block;max-width:360px;margin:20px auto;padding:16px;border-radius:8px;background:#00adb5;color:#fff;"
               "text-decoration:none;font-weight:bold;transition:all 0.3s ease;"
               "animation:pulse 2s infinite alternate}"
               ".t:hover{transform:translateY(-3px);animation:pulse 0.8s infinite alternate}"
               "@keyframes pulse{"
                 "0%{box-shadow:0 0 4px rgba(0,173,181,0.4)}"
                 "100%{box-shadow:0 0 16px rgba(0,173,181,0.8), 0 0 24px rgba(0,173,181,0.2)}"
               "}"
               "</style></head><body><h2>Szablon OTA</h2>");
  h += F("<a class='t' href='/upload_atmega_page'>Wgraj program do ATmegi (.bin)</a>");
  h += F("<a class='t' href='/upload_esp_page'>Zaktualizuj ESP (.bin)</a>");
  // TUTAJ dopisz własne kafelki / strony projektu
  h += F("</body></html>");
  server.send(200, "text/html", h);
}


void saveConfigCallback() {        // po zapisaniu WiFi w portalu restartujemy ESP
  delay(1000);
  ESP.restart();
}

void setup() {
  Serial.begin(UART_BAUD);         // ten sam UART co do ATmegi (OTA używa go do wgrywania)
  Serial.setTimeout(10);
  while (Serial.available() > 0) Serial.read();

  LittleFS.begin();                // pamięć plików (przyda się w projekcie; OTA jej nie wymaga)

  WiFiManager wm;
  wm.setSaveConfigCallback(saveConfigCallback);
  wm.setConfigPortalTimeout(0);
  wm.autoConnect("ESP_Konfiguracja");   // pierwszy raz: sieć "ESP_Konfiguracja" -> wybierz swoje WiFi

  server.on("/", handleRoot);
  setupAtmegaOTA();                // rejestruje /upload_atmega_page i /do_upload_atmega
  setupEsp();                      // rejestruje /upload_esp_page i /do_upload_esp + ArduinoOTA
  // TUTAJ rejestruj własne strony projektu: server.on("/moja", handler);
  server.begin();
}

void loop() {
  obslugaArduinoOTA();             // OBOWIĄZKOWE
  server.handleClient();           // OBOWIĄZKOWE
  // TUTAJ własny kod projektu. UWAGA: nie czytaj Serial w trakcie wgrywania ATmegi -
  // OTA blokuje pętlę na czas wgrywania, więc nie ma konfliktu, dopóki nie robisz tego w przerwaniach.
}
