#!/usr/bin/env python3
"""
Wgrywa plik .bin do ATmegi przez ESP8266 (strona /do_upload_atmega z zakładki 6_esp_ota-atmega.ino).
Używane przez Arduino IDE (przycisk "Prześlij" przy wybranym porcie sieciowym) albo ręcznie:

    python3 ota_atmega.py ADRES_ESP plik.bin [port_http=80]

Kod wyjścia 0 = ATmega zaprogramowana (odpowiedź ESP zaczyna się od "OK"), inny = błąd.
Tylko standardowa biblioteka Pythona.
"""
import http.client, os, re, sys, uuid

def main():
    if len(sys.argv) < 3:
        print(__doc__); return 2
    host, path = sys.argv[1], sys.argv[2]
    port = int(sys.argv[3]) if len(sys.argv) > 3 else 80
    try:
        data = open(path, 'rb').read()
    except OSError as e:
        print(f"Nie mogę odczytać pliku {path}: {e.strerror}"); return 2
    if len(data) < 128 or data[:1] == b':':
        print("To nie wygląda na surowy plik .bin (za mały albo tekst HEX)."); return 2
    b = '----OTA' + uuid.uuid4().hex
    head = (f'--{b}\r\nContent-Disposition: form-data; name="update"; filename="{os.path.basename(path)}"\r\n'
            f'Content-Type: application/octet-stream\r\n\r\n').encode()
    body = head + data + f'\r\n--{b}--\r\n'.encode()
    print(f"Wysyłanie {len(data)} B do http://{host}:{port}/do_upload_atmega (programowanie trwa kilkanaście sekund)...", flush=True)
    try:
        c = http.client.HTTPConnection(host, port, timeout=240)
        c.request('POST', '/do_upload_atmega', body,
                  {'Content-Type': 'multipart/form-data; boundary=' + b, 'Content-Length': str(len(body))})
        r = c.getresponse()
        txt = r.read().decode('utf-8', 'replace')
    except (OSError, http.client.HTTPException) as e:
        print(f"Brak połączenia z ESP ({host}): {e}"); return 1
    txt = re.sub(r'<br\s*/?>', '\n', txt).strip()
    print(txt)
    return 0 if r.status == 200 and txt.startswith('OK') else 1

if __name__ == '__main__':
    sys.exit(main())
