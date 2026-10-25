# MiniCore 3.1.3 – wsparcie dla kwarców 14,31818 MHz i 25 MHz (ATmega328P)

Łatka do pakietu płytek **[MiniCore](https://github.com/MCUdude/MiniCore) 3.1.3** dla Arduino IDE.
Dodaje do menu **Narzędzia → Płytka** dwie nowe pozycje z zamrożonym kwarcem oraz zestawy prędkości
transmisji UART z błędem 0%:

| Nowa płytka | F_CPU | Uwagi |
|---|---|---|
| `ATmega328P / 14.31818 MHz xtal` | `14318180L` | kwarc NTSC (colorburst) |
| `ATmega328P / 25 MHz xtal (przetaktowanie)` | `25000000L` | **przetaktowanie** – maksimum wg specyfikacji to 20 MHz |

Oryginalne płytki i ich menu **nie są modyfikowane** – łatka wyłącznie dodaje nowe linie
do `boards.txt`.

> MiniCore 3.x używa bootloadera **[Urboot](https://github.com/stefanrueger/urboot)** (Stefan Rueger)
> z automatycznym wykrywaniem prędkości (autobaud), a nie Optiboota. Ma to wpływ na działanie
> menu Baud rate i na budowanie bootloadera – patrz [Bootloader](#bootloader-urboot).

---

## Spis treści

1. [Co zawiera łatka](#co-zawiera-łatka)
2. [Prędkości transmisji (baud)](#prędkości-transmisji-baud)
3. [Wymagania](#wymagania)
4. [Instalacja](#instalacja)
5. [Odinstalowanie](#odinstalowanie)
6. [Użycie w projekcie](#użycie-w-projekcie)
7. [Bootloader (Urboot)](#bootloader-urboot)
8. [Ograniczenia i uwagi](#ograniczenia-i-uwagi)
9. [Rozwiązywanie problemów](#rozwiązywanie-problemów)
10. [FAQ](#faq)

---

## Co zawiera łatka

- Dwie nowe płytki w menu **Płytka** (14,31818 MHz oraz 25 MHz).
- **Zamrożony zegar** – na nowych płytkach nie ma menu *Clock*; `F_CPU` i fusy są skopiowane
  z wpisu „External 16 MHz” i dostosowane do nowej częstotliwości.
- **Zamrożony wariant 328P** – jeśli płytka `328` ma menu *Variant*, wariant jest ustawiony na 328P.
- **Upload.default_speed** jest ustawione na prędkość z błędem 0% (89489 dla 14,31818 MHz,
  125000 dla 25 MHz), więc wpis `Default` w menu Baud rate działa poprawnie dla danego kwarca.
- **Menu Baud rate** zawiera:
  - zaraz po wpisie `Default` wpisy z błędem 0% dla danego kwarca, np.
    `447443 - error 0% -> kwarc 14,31818Mhz`,
    `390625 - error 0% -> kwarc 25Mhz (przetaktowanie)`,
  - poniżej wszystkie dotychczasowe, standardowe wpisy (istniejący wpis `125000` jest na
    płytce 25 MHz zastąpiony wersją opisaną jako 0%),
  - dzięki autobaud w Urboot wpisy zmieniają **prędkość uploadu**; nie wymagają osobnego
    bootloadera dla każdej prędkości.
- Skrypt `minicore_add_boards.py`, który generuje plik `.diff` na podstawie **Twojego** `boards.txt`
  (dzięki temu łatka nie zależy od dokładnej zawartości pliku w konkretnej kopii pakietu).

## Prędkości transmisji (baud)

Prędkości liczone dla trybu **U2X** (podwójna prędkość):

```
UBRR = F_CPU / (8 · baud) − 1
```

### 25 MHz (przetaktowanie) – błąd dokładnie 0%

`25 000 000 / 8 = 3 125 000 = 2³·5⁸`, więc poniższe wartości są całkowite i dokładne.

| Baud | UBRR | Błąd |
|---:|---:|---:|
| 25000 | 124 | 0% |
| 62500 | 49 | 0% |
| 125000 | 24 | 0% |
| 312500 | 9 | 0% |
| 390625 | 7 | 0% |

### 14,31818 MHz – błąd praktycznie 0%

Przy `F_CPU = 14 318 180 Hz` rzeczywista prędkość to `1 789 772,5 / k`, gdzie `k = UBRR + 1`.
Wynik nie jest liczbą całkowitą, więc „0%” oznacza tu błąd wynikający wyłącznie z zaokrąglenia
do pełnego bauda (< 0,001%). Standardowe prędkości (9600, 115200 itd.) **nie** dają przy tym
kwarcu zerowego błędu.

| Baud (zaokrąglony) | UBRR | Dokładna wartość | Błąd |
|---:|---:|---:|---:|
| 22372 | 79 | 22372,16 | ≈ 0,0007% |
| 89489 | 19 | 89488,63 | ≈ 0,0004% |
| 178977 | 9 | 178977,25 | ≈ 0,0001% |
| 447443 | 3 | 447443,13 | ≈ 0,00003% |
| 596591 | 2 | 596590,83 | ≈ 0,00003% |

> Po stronie komputera (terminal, avrdude) trzeba ustawić **dokładnie** tę prędkość,
> a konwerter USB–UART musi być w stanie ją wygenerować.

## Wymagania

- Arduino IDE 1.8.x lub 2.x z zainstalowanym pakietem **MiniCore 3.1.3**
  (Menedżer płytek).
- **Python 3** (do uruchomienia skryptu generującego łatkę).
- Narzędzie `patch` (Linux, macOS, Git Bash w Windows) lub `git apply`.
  Alternatywnie skrypt potrafi zmodyfikować plik samodzielnie (`--apply`).
- **Programator ISP** – do ustawienia fusów i wgrania bootloadera (*Wypal bootloader*).
  Zmiana kwarca wymaga tej operacji.

## Instalacja

### 1. Znajdź katalog pakietu

| System | Ścieżka |
|---|---|
| Linux | `~/.arduino15/packages/MiniCore/hardware/avr/3.1.3/` |
| macOS | `~/Library/Arduino15/packages/MiniCore/hardware/avr/3.1.3/` |
| Windows | `%LOCALAPPDATA%\Arduino15\packages\MiniCore\hardware\avr\3.1.3\` |

W katalogu powinien znajdować się plik `boards.txt`.

### 2. Zamknij Arduino IDE

IDE wczytuje definicje płytek przy starcie.

### 3. Skopiuj skrypt i wygeneruj plik `.diff`

```bash
cd <katalog pakietu 3.1.3>
cp /ścieżka/do/minicore_add_boards.py .
python3 minicore_add_boards.py boards.txt
```

Skrypt wypisze podsumowanie, np.:

```
Plyta '328': dodano nowych plyt: 2, usunieto linii: 0
Wariant 328P zamrozony: tak (menu.variant.modelP)
Diff zapisany: minicore_3.1.3_328p_14M31818_25M.diff
```

Powstaje plik `minicore_3.1.3_328p_14M31818_25M.diff`. Samo wygenerowanie diffa
**nie zmienia** `boards.txt`.

### 4. Nałóż łatkę

Najpierw test na sucho:

```bash
patch -p1 --dry-run < minicore_3.1.3_328p_14M31818_25M.diff
```

Jeśli nie ma błędów:

```bash
patch -p1 < minicore_3.1.3_328p_14M31818_25M.diff
```

Alternatywy:

```bash
# git
git apply --check minicore_3.1.3_328p_14M31818_25M.diff
git apply minicore_3.1.3_328p_14M31818_25M.diff

# bez narzędzia patch – skrypt sam zapisze kopię boards.txt.orig i zmodyfikuje plik
python3 minicore_add_boards.py boards.txt --apply
```

### 5. Uruchom Arduino IDE

W **Narzędzia → Płytka → MiniCore** pojawią się nowe pozycje. Po wybraniu jednej z nich
w menu **Baud rate** na początku znajdziesz prędkości dla wybranego kwarca.

### 6. Wypal bootloader

Podłącz programator ISP, wybierz go w menu **Programator** i użyj **Narzędzia → Wypal bootloader**.
Ustawia to fusy dla zewnętrznego kwarca i wgrywa Urboot (istniejący plik z pakietu – patrz
[Bootloader](#bootloader-urboot)).

### Opcje skryptu

| Opcja | Opis |
|---|---|
| `--board ID` | id płytki w `boards.txt` (domyślnie `328`) |
| `--apply` | zapisuje `boards.txt.orig` i nadpisuje `boards.txt` |

Jeśli płytka o podanym id nie istnieje, skrypt wypisze listę dostępnych id.

## Odinstalowanie

```bash
# jeśli łatkę nałożono przez patch
patch -R -p1 < minicore_3.1.3_328p_14M31818_25M.diff

# jeśli użyto --apply
mv boards.txt.orig boards.txt
```

Można też przeinstalować pakiet MiniCore w Menedżerze płytek. Aktualizacja pakietu do
nowszej wersji **usuwa łatkę** – po zmianie wersji trzeba ją wygenerować ponownie.

## Użycie w projekcie

Po wybraniu jednej z nowych płytek **nie trzeba nic definiować w kodzie** – IDE przekazuje
do kompilatora `-DF_CPU=14318180L` lub `-DF_CPU=25000000L`.

W szkicu wystarczy podać prędkość UART:

```cpp
void setup() {
  Serial.begin(447443);   // 14,31818 MHz → UBRR = 3, błąd ≈ 0%
  // Serial.begin(390625); // 25 MHz       → UBRR = 7, błąd 0%
}

void loop() {
  Serial.println(F("Hello"));
  delay(1000);
}
```

Zasady:

- Menu **Baud rate** określa **prędkość uploadu** (po stronie avrdude). Urboot wykrywa prędkość
  automatycznie, więc jej zmiana nie wymaga ponownego wypalania bootloadera. Prędkość w szkicu
  (`Serial.begin`) ustawiasz niezależnie, ale zwykle warto używać tej samej wartości.
- Rdzeń wylicza UBRR wzorem `(F_CPU / 4 / baud − 1) / 2`; dla prędkości z tabel powyżej
  wynik jest zgodny z wartościami UBRR z tabel.
- **Serial Monitor** w Arduino IDE ma stałą listę prędkości, która nie zawiera tych wartości.
  Użyj zewnętrznego terminala (PuTTY, CoolTerm, `picocom`, `minicom`) z dokładną prędkością.
- Kwarc musi być fizycznie zamontowany. Fusy ustawiane są podczas *Wypal bootloader*
  (skopiowane z konfiguracji 16 MHz z zewnętrznym kwarcem).

## Bootloader (Urboot)

MiniCore 3.x korzysta z bootloadera **Urboot** z automatycznym wykrywaniem prędkości
transmisji (autobaud). W `boards.txt` z MiniCore 3.1.3 ścieżka pliku bootloadera ma postać:

```
urboot/{build.mcu}/watchdog_1_s/autobaud/{upload.uartpins}/{build.bootloader_led}/urboot_{build.mcu}_pr_ee_ce.hex
```

Wynika z tego, że:

1. **Plik bootloadera nie zależy od `F_CPU` ani od prędkości.** Nowe płytki używają tego samego
   pliku `urboot_atmega328p_pr_ee_ce.hex` co oryginalna płytka ATmega328P, więc **nie trzeba
   budować żadnych nowych plików `.hex`**. Różnią się tylko fusy (zewnętrzny kwarc) i `F_CPU`
   przekazywane do kompilatora.
2. **Prędkość uploadu można zmieniać bez ponownego wypalania bootloadera.** Wpisy z menu
   *Baud rate* ustawiają wyłącznie `upload.speed` po stronie avrdude.
3. **Autobaud nie jest gwarancją działania** przy dowolnym zegarze i prędkości – patrz
   [Ograniczenia](#ograniczenia-i-uwagi).

Po zainstalowaniu łatki wystarczy wybrać nową płytkę, podłączyć programator ISP i użyć
**Wypal bootloader**.

> Jeśli używasz innej wersji MiniCore niż 3.1.3 i ścieżka `bootloader.file` w Twoim
> `boards.txt` zawiera `{build.f_cpu}`, plik dla nowego zegara nie będzie istniał. Wtedy trzeba
> zbudować bootloader ze źródeł [Urboot](https://github.com/stefanrueger/urboot)
> (`make MCU=atmega328p F_CPU=... AUTOBAUD=1 ...` lub skrypt `mkurboots`) z tymi samymi
> opcjami, co plik dołączony do pakietu, i zachować jego schemat nazw.

## Ograniczenia i uwagi

- **Przetaktowanie 25 MHz.** ATmega328P jest specyfikowana do 20 MHz (przy 4,5–5,5 V).
  Stabilność przy 25 MHz zależy od egzemplarza, napięcia zasilania i temperatury.
  Używasz tego na własną odpowiedzialność.
- **Czas w szkicu.** `delayMicroseconds()` jest w rdzeniu Arduino zoptymalizowane dla
  wybranych częstotliwości; przy 14,31818 MHz opóźnienia są przybliżone. Dokładność
  `millis()` zależy od implementacji `clockCyclesToMicroseconds()` w `wiring.c` – wersja
  `((a) * 1000L) / (F_CPU / 1000L)` daje poprawne wyniki dla zegarów niebędących
  wielokrotnością 1 MHz.
- **Biblioteki** zależne od `F_CPU` (np. `SoftwareSerial`, biblioteki z opóźnieniami
  w cyklach) mogą nie mieć tablic dla tych częstotliwości.
- **Autobaud w Urboot.** Wykrywanie prędkości opiera się na pomiarze czasu bitu w cyklach
  procesora. Przy wysokich prędkościach względem zegara (mało cykli na bit) dokładność
  pomiaru spada. Wpisy z tabel są wyliczone matematycznie dla UART mikrokontrolera, ale
  zachowanie autobauda przy 25 MHz i 14,31818 MHz trzeba sprawdzić praktycznie – jeśli upload
  przy danej prędkości zawodzi, spróbuj niższej wartości z listy.
- **Standardowe baudy** przy 14,31818 MHz mają znaczny błąd (np. 115200: −2,9% / +3,6%);
  przy 25 MHz 115200 daje +0,47%. Dlatego wpisy 0% są umieszczone na początku listy.
- **Programowanie z niestandardową prędkością:** avrdude (w wersji 7.x) obsługuje dowolne
  prędkości, starsze wersje na Linuksie mogą akceptować tylko wartości standardowe.
  Konwerter USB–UART musi wygenerować wymaganą prędkość (FTDI i CP210x zwykle tak,
  CH340 – z ograniczeniami).
- **Menu Arduino IDE jest statyczne.** Nie ma mechanizmu ukrywania wpisów zależnie od wyboru
  w innym menu – dlatego rozwiązaniem są osobne płytki z zamrożonym kwarcem.
- **Aktualizacja MiniCore** usuwa łatkę (patrz [Odinstalowanie](#odinstalowanie)).
- Skrypt przetestowano na rzeczywistym pliku `boards.txt` z MiniCore 3.1.3 (diff nakłada się
  poprawnie i nie usuwa żadnych linii). Działania na sprzęcie (autobaud przy 14,31818 MHz
  i 25 MHz) nie weryfikowano. Zawsze użyj `patch --dry-run` przed właściwym nałożeniem łatki.

## Rozwiązywanie problemów

| Objaw | Przyczyna / rozwiązanie |
|---|---|
| `Nie znaleziono plyty '328'` | W Twoim `boards.txt` płytka ma inne id – użyj `--board ID` z listy wypisanej przez skrypt. |
| `Nie znaleziono wpisu 'External 16 MHz'` | Inna etykieta wpisu zegara w tej wersji pakietu – dostosuj wyrażenie regularne `rx_clk` w skrypcie. |
| `Wariant 328P zamrozony: nie` | Nie znaleziono menu *Variant* z wariantem 328P – menu zostaje skopiowane i wariant wybierasz ręcznie. |
| `patch` zgłasza `Hunk FAILED` | Diff wygenerowano z innej kopii `boards.txt`. Wygeneruj go ponownie na plikach, na których go nakładasz. |
| Nowych płytek nie widać w IDE | IDE było uruchomione podczas łatania – zamknij je całkowicie i uruchom ponownie. |
| Upload kończy się `not in sync` | Brak bootloadera w układzie, niezgodny kwarc (fusy), zamienione linie RX/TX, brak kondensatora na linii reset lub zbyt wysoka prędkość uploadu dla autobauda – spróbuj `Default` lub niższej wartości z menu Baud rate. |
| Znaki „krzaki” na terminalu | Prędkość terminala różni się od `Serial.begin()` lub konwerter nie generuje tej prędkości. |
| `Wypal bootloader` kończy się błędem | Sprawdź programator ISP, okablowanie i obecność kwarca (bez działającego zegara układ nie odpowiada na ISP). |

## FAQ

**Czy mogę dodać inne układy (np. ATmega168, 1284)?**
Skrypt obsługuje jedną płytkę naraz i był testowany tylko dla `328`. Dla innego id uruchom go
z `--board ID` (działanie na innych układach nie było weryfikowane).
Każde uruchomienie generuje własny plik `.diff` (nazwa pliku wyjściowego jest stała,
więc przed kolejnym uruchomieniem zmień nazwę poprzedniego).

**Czy łatka zmienia rdzeń Arduino (`wiring.c`, `HardwareSerial`)?**
Nie. Modyfikowany jest wyłącznie `boards.txt`.

**Dlaczego w menu Baud rate są też standardowe wpisy?**
Aby nic z dotychczasowej funkcjonalności nie zniknęło. Wpisy dla kwarca stoją zaraz po
wpisie `Default`, a sam `Default` jest ustawiony na prędkość z błędem 0%.

**Dlaczego 25 MHz to „przetaktowanie”?**
Maksymalna częstotliwość zegara ATmega328P wg dokumentacji producenta to 20 MHz.

---

## Licencja i podziękowania

Łatka dotyczy pakietu [MiniCore](https://github.com/MCUdude/MiniCore) autorstwa
MCUdude i współtwórców – obowiązuje jego licencja. Skrypt i dokumentacja zawarte w tym
repozytorium są udostępniane bez gwarancji, „tak jak są”.
