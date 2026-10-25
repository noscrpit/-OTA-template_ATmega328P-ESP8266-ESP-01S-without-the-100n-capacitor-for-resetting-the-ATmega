# MiniCore 3.1.3 – support for 14.31818 MHz and 25 MHz crystals (ATmega328P)

A patch for the **[MiniCore](https://github.com/MCUdude/MiniCore) 3.1.3** board package for the Arduino IDE.
It adds two new entries to **Tools → Board** with a fixed crystal frequency, together with
sets of UART baud rates that have 0% error:

| New board | F_CPU | Notes |
|---|---|---|
| `ATmega328P / 14.31818 MHz xtal` | `14318180L` | NTSC (colorburst) crystal |
| `ATmega328P / 25 MHz xtal (przetaktowanie)` | `25000000L` | **overclocked** – the specified maximum is 20 MHz |

The original boards and their menus are **not modified** – the patch only adds new lines
to `boards.txt`.

> MiniCore 3.x uses the **[Urboot](https://github.com/stefanrueger/urboot)** bootloader (Stefan Rueger)
> with automatic baud rate detection (autobaud), not Optiboot. This affects how the Baud rate menu
> works – see [Bootloader](#bootloader-urboot).

> **Note:** the board name suffix `(przetaktowanie)` and the baud rate menu labels
> (e.g. `kwarc 14,31818Mhz`) are in Polish. *przetaktowanie* = overclocking, *kwarc* = crystal.
> The messages printed by the script are in Polish as well.

---

## Table of contents

1. [What the patch contains](#what-the-patch-contains)
2. [Baud rates](#baud-rates)
3. [Requirements](#requirements)
4. [Installation](#installation)
5. [Uninstalling](#uninstalling)
6. [Using it in a project](#using-it-in-a-project)
7. [Bootloader (Urboot)](#bootloader-urboot)
8. [Limitations and notes](#limitations-and-notes)
9. [Troubleshooting](#troubleshooting)
10. [FAQ](#faq)

---

## What the patch contains

- Two new boards in the **Board** menu (14.31818 MHz and 25 MHz).
- **Fixed clock** – the new boards have no *Clock* menu; `F_CPU` and the fuses are copied
  from the "External 16 MHz" entry and adapted to the new frequency.
- **Fixed 328P variant** – if the `328` board has a *Variant* menu, the variant is set to 328P.
- **`upload.default_speed`** is set to a baud rate with 0% error (89489 for 14.31818 MHz,
  125000 for 25 MHz), so the `Default` entry in the Baud rate menu works correctly for the given crystal.
- **The Baud rate menu** contains:
  - right after the `Default` entry, the baud rates with 0% error for the given crystal, e.g.
    `447443 - error 0% -> kwarc 14,31818Mhz`,
    `390625 - error 0% -> kwarc 25Mhz (przetaktowanie)`,
  - below them, all the existing standard entries (the existing `125000` entry is replaced on the
    25 MHz board by the version labelled as 0%),
  - thanks to autobaud in Urboot, the entries change the **upload speed**; they do not need a separate
    bootloader for each speed.
- The `minicore_add_boards.py` script, which generates the `.diff` file from **your own** `boards.txt`
  (so the patch does not depend on the exact contents of the file in a particular copy of the package).

## Baud rates

The rates are calculated for **U2X** mode (double speed):

```
UBRR = F_CPU / (8 · baud) − 1
```

### 25 MHz (overclocked) – exactly 0% error

`25,000,000 / 8 = 3,125,000 = 2³·5⁸`, so the values below are integers and exact.

| Baud | UBRR | Error |
|---:|---:|---:|
| 25000 | 124 | 0% |
| 62500 | 49 | 0% |
| 125000 | 24 | 0% |
| 312500 | 9 | 0% |
| 390625 | 7 | 0% |

### 14.31818 MHz – practically 0% error

With `F_CPU = 14,318,180 Hz` the actual rate is `1,789,772.5 / k`, where `k = UBRR + 1`.
The result is not an integer, so "0%" here means an error caused only by rounding
to a whole baud value (< 0.001%). Standard rates (9600, 115200, etc.) do **not** give zero error
with this crystal.

| Baud (rounded) | UBRR | Exact value | Error |
|---:|---:|---:|---:|
| 22372 | 79 | 22372.16 | ≈ 0.0007% |
| 89489 | 19 | 89488.63 | ≈ 0.0004% |
| 178977 | 9 | 178977.25 | ≈ 0.0001% |
| 447443 | 3 | 447443.13 | ≈ 0.00003% |
| 596591 | 2 | 596590.83 | ≈ 0.00003% |

> On the computer side (terminal, avrdude) you must set **exactly** this rate,
> and the USB–UART converter must be able to generate it.

## Requirements

- Arduino IDE 1.8.x or 2.x with the **MiniCore 3.1.3** package installed
  (Boards Manager).
- **Python 3** (to run the script that generates the patch).
- The `patch` tool (Linux, macOS, Git Bash on Windows) or `git apply`.
  Alternatively the script can modify the file itself (`--apply`).
- **An ISP programmer** – to set the fuses and upload the bootloader (*Burn Bootloader*).
  Changing the crystal requires this operation.

## Installation

### 1. Find the package directory

| System | Path |
|---|---|
| Linux | `~/.arduino15/packages/MiniCore/hardware/avr/3.1.3/` |
| macOS | `~/Library/Arduino15/packages/MiniCore/hardware/avr/3.1.3/` |
| Windows | `%LOCALAPPDATA%\Arduino15\packages\MiniCore\hardware\avr\3.1.3\` |

The directory should contain the `boards.txt` file.

### 2. Close the Arduino IDE

The IDE reads board definitions at startup.

Use `patch_update.sh` in the MiniCore directory, or:

### 3. Copy the script and generate the `.diff` file

```bash
cp -vf --backup=numbered programmers.diff  platform.diff  "/patch_to/3.1.3"
in 3.1.3 dir:
patch -p1 <  programmers.diff programmers.txt && patch -p1 <  platform.diff platform.txt

cd <package directory 3.1.3>
cp /path/to/minicore_add_boards.py .
python3 minicore_add_boards.py boards.txt
```

The script prints a summary (in Polish), e.g.:

```
Plyta '328': dodano nowych plyt: 2, usunieto linii: 0
Wariant 328P zamrozony: tak (menu.variant.modelP)
Diff zapisany: minicore_3.1.3_328p_14M31818_25M.diff
```

(*added boards: 2, removed lines: 0 / 328P variant frozen: yes / diff saved*)

This creates `minicore_3.1.3_328p_14M31818_25M.diff`. Generating the diff
**does not change** `boards.txt`.

### 4. Apply the patch

First a dry run:

```bash
patch -p1 --dry-run < minicore_3.1.3_328p_14M31818_25M.diff
```

If there are no errors:

```bash
patch -p1 < minicore_3.1.3_328p_14M31818_25M.diff
```

Alternatives:

```bash
# git
git apply --check minicore_3.1.3_328p_14M31818_25M.diff
git apply minicore_3.1.3_328p_14M31818_25M.diff

# without the patch tool – the script saves boards.txt.orig itself and modifies the file
python3 minicore_add_boards.py boards.txt --apply
```

### 5. Start the Arduino IDE

The new entries appear under **Tools → Board → MiniCore**. After selecting one of them,
the **Baud rate** menu lists the rates for the chosen crystal right after `Default`.

### 6. Burn the bootloader

Connect the ISP programmer, select it in the **Programmer** menu and use **Tools → Burn Bootloader**.
This sets the fuses for the external crystal and uploads Urboot (the existing file from the package – see
[Bootloader](#bootloader-urboot)).

### Script options

| Option | Description |
|---|---|
| `--board ID` | board id in `boards.txt` (default `328`) |
| `--apply` | saves `boards.txt.orig` and overwrites `boards.txt` |

If the board with the given id does not exist, the script prints a list of available ids.

## Uninstalling

```bash
# if the patch was applied with patch
patch -R -p1 < minicore_3.1.3_328p_14M31818_25M.diff

# if --apply was used
mv boards.txt.orig boards.txt
```

You can also reinstall the MiniCore package in the Boards Manager. Updating the package to a
newer version **removes the patch** – after changing versions it has to be generated again.

## Using it in a project

After selecting one of the new boards, **you do not need to define anything in code** – the IDE passes
`-DF_CPU=14318180L` or `-DF_CPU=25000000L` to the compiler.

In the sketch you only need to set the UART speed:

```cpp
void setup() {
  Serial.begin(447443);   // 14.31818 MHz → UBRR = 3, error ≈ 0%
  // Serial.begin(390625); // 25 MHz       → UBRR = 7, error 0%
}

void loop() {
  Serial.println(F("Hello"));
  delay(1000);
}
```

Rules:

- The **Baud rate** menu sets the **upload speed** (on the avrdude side). Urboot detects the speed
  automatically, so changing it does not require burning the bootloader again. The speed in the sketch
  (`Serial.begin`) is set independently, but it is usually worth using the same value.
- The core calculates UBRR with `(F_CPU / 4 / baud − 1) / 2`; for the rates in the tables above
  the result matches the UBRR values in the tables.
- The **Serial Monitor** in the Arduino IDE has a fixed list of speeds that does not include these values.
  Use an external terminal (PuTTY, CoolTerm, `picocom`, `minicom`) with the exact rate.
- The crystal must be physically installed. The fuses are set during *Burn Bootloader*
  (copied from the 16 MHz external crystal configuration).

## Bootloader (Urboot)

MiniCore 3.x uses the **Urboot** bootloader with automatic baud rate detection
(autobaud). In the `boards.txt` from MiniCore 3.1.3 the bootloader file path looks like this:

```
urboot/{build.mcu}/watchdog_1_s/autobaud/{upload.uartpins}/{build.bootloader_led}/urboot_{build.mcu}_pr_ee_ce.hex
```

This means that:

1. **The bootloader file depends neither on `F_CPU` nor on the baud rate.** The new boards use the same
   file `urboot_atmega328p_pr_ee_ce.hex` as the original ATmega328P board, so **you do not need to
   build any new `.hex` files**. Only the fuses (external crystal) and the `F_CPU` passed to
   the compiler differ.
2. **The upload speed can be changed without burning the bootloader again.** The entries in the
   *Baud rate* menu only set `upload.speed` on the avrdude side.
3. **Autobaud is not a guarantee** that it will work at any clock and speed – see
   [Limitations](#limitations-and-notes).

After installing the patch, just select the new board, connect the ISP programmer and use
**Burn Bootloader**.

> If you use a MiniCore version other than 3.1.3 and the `bootloader.file` path in your
> `boards.txt` contains `{build.f_cpu}`, the file for the new clock will not exist. In that case you have to
> build the bootloader from the [Urboot](https://github.com/stefanrueger/urboot) sources
> (`make MCU=atmega328p F_CPU=... AUTOBAUD=1 ...` or the `mkurboots` script) with the same
> options as the file shipped with the package, and keep its naming scheme.

## Limitations and notes

- **25 MHz overclocking.** The ATmega328P is specified for up to 20 MHz (at 4.5–5.5 V).
  Stability at 25 MHz depends on the individual chip, the supply voltage and the temperature.
  You use this at your own risk.
- **Timing in sketches.** `delayMicroseconds()` in the Arduino core is optimized for
  selected frequencies; at 14.31818 MHz the delays are approximate. The accuracy of
  `millis()` depends on the implementation of `clockCyclesToMicroseconds()` in `wiring.c` – the version
  `((a) * 1000L) / (F_CPU / 1000L)` gives correct results for clocks that are not a
  multiple of 1 MHz.
- **Libraries** that depend on `F_CPU` (e.g. `SoftwareSerial`, libraries with cycle-counted
  delays) may not have tables for these frequencies.
- **Autobaud in Urboot.** Speed detection is based on measuring the bit time in CPU cycles.
  At high baud rates relative to the clock (few cycles per bit) the measurement accuracy
  drops. The values in the tables are calculated mathematically for the microcontroller's UART, but
  the behaviour of autobaud at 25 MHz and 14.31818 MHz has to be checked in practice – if an upload
  fails at a given speed, try a lower value from the list.
- **Standard baud rates** at 14.31818 MHz have a significant error (e.g. 115200: −2.9% / +3.6%);
  at 25 MHz 115200 gives +0.47%. That is why the 0% entries are placed at the start of the list.
- **Programming at a non-standard speed:** avrdude (version 7.x) supports arbitrary
  speeds; older versions on Linux may accept only standard values.
  The USB–UART converter must be able to generate the required rate (FTDI and CP210x usually can,
  CH340 – with limitations).
- **Arduino IDE menus are static.** There is no mechanism for hiding entries depending on the selection
  in another menu – that is why separate boards with a fixed crystal are the solution.
- **Updating MiniCore** removes the patch (see [Uninstalling](#uninstalling)).
- The script was tested on a real `boards.txt` file from MiniCore 3.1.3 (the diff applies
  cleanly and removes no lines). Operation on hardware (autobaud at 14.31818 MHz
  and 25 MHz) has not been verified. Always use `patch --dry-run` before applying the patch for real.

## Troubleshooting

The first three messages are printed by the script in Polish.

| Symptom | Cause / solution |
|---|---|
| `Nie znaleziono plyty '328'` (*board '328' not found*) | The board has a different id in your `boards.txt` – use `--board ID` from the list printed by the script. |
| `Nie znaleziono wpisu 'External 16 MHz'` (*"External 16 MHz" entry not found*) | A different clock entry label in this package version – adjust the `rx_clk` regular expression in the script. |
| `Wariant 328P zamrozony: nie` (*328P variant frozen: no*) | No *Variant* menu with a 328P variant was found – the menu is copied and you select the variant manually. |
| `patch` reports `Hunk FAILED` | The diff was generated from a different copy of `boards.txt`. Generate it again on the files you are applying it to. |
| New boards do not show up in the IDE | The IDE was running while patching – close it completely and start it again. |
| Upload ends with `not in sync` | No bootloader in the chip, mismatched crystal (fuses), swapped RX/TX lines, missing capacitor on the reset line, or an upload speed too high for autobaud – try `Default` or a lower value from the Baud rate menu. |
| Garbage characters in the terminal | The terminal speed differs from `Serial.begin()` or the converter does not generate this speed. |
| `Burn Bootloader` fails | Check the ISP programmer, the wiring and the presence of the crystal (without a working clock the chip does not respond over ISP). |

## FAQ

**Can I add other chips (e.g. ATmega168, 1284)?**
The script handles one board at a time and has only been tested with `328`. For another id run it
with `--board ID` (operation on other chips has not been verified).
Each run generates its own `.diff` file (the output file name is fixed,
so rename the previous one before the next run).

**Does the patch modify the Arduino core (`wiring.c`, `HardwareSerial`)?**
No. Only `boards.txt` is modified.

**Why does the Baud rate menu also contain the standard entries?**
So that nothing of the existing functionality disappears. The crystal entries come right after
the `Default` entry, and `Default` itself is set to a baud rate with 0% error.

**Why is 25 MHz called "overclocking"?**
The maximum clock frequency of the ATmega328P according to the manufacturer's documentation is 20 MHz.

---

## License and acknowledgements

The patch applies to the [MiniCore](https://github.com/MCUdude/MiniCore) package by
MCUdude and contributors – its license applies. The script and documentation in this
repository are provided without warranty, "as is".
