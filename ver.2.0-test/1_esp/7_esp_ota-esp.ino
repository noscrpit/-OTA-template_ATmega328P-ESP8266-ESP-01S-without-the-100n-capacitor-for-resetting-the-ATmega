// =========================================================================
//                           ZAKŁADKA: 7_esp_ota-esp.ino  (aktualizacja samego ESP)
// =========================================================================

#include <Updater.h>
#include <ArduinoOTA.h>

void handleUploadEspPage() {
  String html = F("<!DOCTYPE html><html><head><meta charset='utf-8'><title>OTA ESP</title><style>");
  html += F("body{font-family:Arial;background:#121212;color:#fff;text-align:center;padding-top:50px;}");
  html += F(".container{margin:30px auto;background:#1e1e1e;max-width:400px;padding:30px;border-radius:10px;}");
  html += F(".progress-bg{width:100%;background:#333;border-radius:10px;height:25px;margin:20px 0;display:none;overflow:hidden;}");
  html += F(".progress-bar{width:0%;background:#ff2e63;height:100%;color:#fff;font-weight:bold;line-height:25px;text-align:center;transition:width 0.1s;}");
  html += F(".btn{padding:10px 20px;background:#ff2e63;color:#fff;border:none;border-radius:4px;cursor:pointer;font-weight:bold;}");
  html += F("</style></head><body><h2>Bezprzewodowe wgrywanie softu do ESP8266 (.bin)</h2><div class='container'>");
  html += F("<input type='file' id='fileInputEsp' accept='.bin' style='margin-bottom:20px;'><br>");
  html += F("<button type='button' class='btn' onclick='startUploadEsp()'>Uruchom aktualizację flasha ESP</button>");
  html += F("<div id='prgBgEsp' class='progress-bg'><div id='prgBarEsp' class='progress-bar'>0%</div></div>");
  html += F("<div id='statusTxtEsp' style='margin-top:10px;font-weight:bold;color:#ff2e63;'></div></div><br><a href='/' style='color:#00adb5;text-decoration:none;'>Powrót do menu głównego</a>");
  
  // ZMIANA: Skrócenie czasu odliczania w JavaScript z 5000ms do 2000ms
  html += F("<script>function startUploadEsp(){var fileInput=document.getElementById('fileInputEsp');if(fileInput.files.length===0){alert('Wybierz najpierw plik .bin!');return;}var file=fileInput.files[0];document.getElementById('prgBgEsp').style.display='block';var formData=new FormData();formData.append('update',file);var xhr=new XMLHttpRequest();xhr.upload.addEventListener('progress',function(e){if(e.lengthComputable){var percent=Math.round((e.loaded/e.total)*100);document.getElementById('prgBarEsp').style.width=percent+'%';document.getElementById('prgBarEsp').innerHTML=percent+'%';document.getElementById('statusTxtEsp').innerHTML='Przesłano: '+Math.round(e.loaded/1024)+' KB z '+Math.round(e.total/1024)+' KB';}},false);xhr.open('POST','/do_upload_esp',true);xhr.onload=function(){document.getElementById('statusTxtEsp').innerHTML='Wgrywanie zakończone! ESP wykonuje restart...';setTimeout(function(){window.location.href='/';},2000);};xhr.send(formData);}</script></body></html>");
  server.send(200, "text/html", html);
}


void handleDoUploadEsp() {
  server.sendHeader("Connection", "close");
  server.send(200, "text/html", "OK");
  
  // ZMIANA: Skrócenie czasu na wysłanie bufora sieciowego (50ms zamiast 1000ms)
  delay(50);
  
  // ZMIANA: Błyskawiczny reset sprzętowy Watchdogiem zamiast programowego czyszczenia RAMU przez .restart()
  ESP.reset();
}

void handleEspStreaming() {
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if (!Update.begin(maxSketchSpace)) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.println("WIFI_STATUS:ESP_UPDATED_REBOOT");
    } else {
      Update.printError(Serial);
    }
  }
}

void setupEsp() {
  // Rejestracja stron WWW aktualizacji ESP
  server.on("/upload_esp_page", handleUploadEspPage);
  server.on("/do_upload_esp", HTTP_POST, handleDoUploadEsp, handleEspStreaming);

  // Start sieciowego nasłuchu z Arduino IDE
  ArduinoOTA.setHostname(OTA_HOSTNAME);   // makro z zakładki głównej
  ArduinoOTA.begin();
}

void obslugaArduinoOTA() {
  ArduinoOTA.handle();
}
