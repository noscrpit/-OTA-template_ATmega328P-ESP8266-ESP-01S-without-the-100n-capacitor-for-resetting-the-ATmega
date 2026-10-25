#!/usr/bin/env python3
"""
MiniCore 3.1.3 - dodaje OSOBNE PLYTY tylko dla ATmega328P z zamrozonym kwarcem:
   ATmega328P / 14.31818 MHz xtal
   ATmega328P / 25 MHz xtal (przetaktowanie)
Bez menu Clock (i bez menu Variant, jesli plyta ma wariant 328P - zamrozony na 328P).
Menu Baud rate: wpisy 0% error dla kwarcu na poczatku + wszystkie standardowe.
Oryginalne plyty nie sa zmieniane - tylko dodawane nowe bloki.

Uzycie:
  python3 minicore_add_boards.py boards.txt                  # tylko diff
  python3 minicore_add_boards.py boards.txt --board 328      # inne id plyty (domyslnie 328)
  python3 minicore_add_boards.py boards.txt --apply          # + zapis boards.txt (kopia .orig)
"""
import re, sys, difflib, shutil

DEFAULT_BOARD = "328"

VARIANTS = [
    dict(suffix="14m31818", name=" / 14.31818 MHz xtal", f_cpu="14318180L",
         note="kwarc 14,31818Mhz", bauds=[22372, 89489, 178977, 447443, 596591]),
    dict(suffix="25m", name=" / 25 MHz xtal (przetaktowanie)", f_cpu="25000000L",
         note="kwarc 25Mhz (przetaktowanie)", bauds=[25000, 62500, 125000, 312500, 390625]),
]

def eol_of(l):
    return l[len(l.rstrip("\r\n")):] or "\n"

def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    path = sys.argv[1]
    b = sys.argv[sys.argv.index("--board") + 1] if "--board" in sys.argv else DEFAULT_BOARD
    with open(path, encoding="utf-8", newline="") as f:
        orig = f.readlines()

    idx = [i for i, x in enumerate(orig) if x.startswith(b + ".")]
    if not idx:
        cands = sorted({x.split(".")[0] for x in orig if ".menu.clock." in x})
        sys.exit(f"Nie znaleziono plyty '{b}'. Dostepne id: {', '.join(cands)}  (uzyj --board ID)")

    m = next((re.match(rf"^{re.escape(b)}\.menu\.clock\.([^.=\s]+)=\s*External 16 ?MHz\b", orig[i], re.I)
              for i in idx if re.match(rf"^{re.escape(b)}\.menu\.clock\.([^.=\s]+)=\s*External 16 ?MHz\b",
                                       orig[i], re.I)), None)
    if not m:
        sys.exit("Nie znaleziono wpisu 'External 16 MHz' w menu.clock tej plyty.")
    oid = m.group(1)

    # wariant 328P (jesli plyta ma menu.variant)
    vid = None
    for i in idx:
        mv = re.match(rf"^{re.escape(b)}\.menu\.variant\.([^.=\s]+)=(.*)$", orig[i])
        if mv and re.search(r"328P(?!B)", mv.group(2)):
            vid = mv.group(1)
            break

    base115 = [i for i in idx if orig[i].startswith(f"{b}.menu.baudrate.115200")]
    existing = {re.match(rf"^{re.escape(b)}\.menu\.baudrate\.([^.=]+)=", orig[i]).group(1)
                for i in idx if re.match(rf"^{re.escape(b)}\.menu\.baudrate\.([^.=]+)=", orig[i])}
    eol = eol_of(orig[idx[0]])
    new_all, count, skipped = [eol], 0, []

    for v in VARIANTS:
        nb = f"{b}_{v['suffix']}"
        if any(x.startswith(nb + ".") for x in orig):
            skipped.append(nb)
            continue
        out, baud_done = [], False
        for i in idx:
            x = orig[i]
            rest = x[len(b):]
            e = eol_of(x)
            if rest.startswith(".menu.clock."):
                pre = f".menu.clock.{oid}."
                if rest.startswith(pre):
                    out.append(nb + re.sub(r"16000000L?", v["f_cpu"], rest[len(f".menu.clock.{oid}"):]))
                continue
            if vid and rest.startswith(".menu.variant."):
                pre = f".menu.variant.{vid}."
                if rest.startswith(pre):
                    out.append(nb + rest[len(f".menu.variant.{vid}"):])
                continue
            if rest.startswith(".name="):
                base = "ATmega328P" if vid else x.split("=", 1)[1].rstrip("\r\n")
                out.append(f"{nb}.name={base}{v['name']}{e}")
                continue
            if rest.startswith(".menu.baudrate.") and not baud_done and base115:
                baud_done = True
                for baud in v["bauds"]:
                    if str(baud) in existing:
                        continue
                    for j in base115:
                        y = orig[j][len(f"{b}.menu.baudrate.115200"):]
                        if y.startswith("="):
                            out.append(f"{nb}.menu.baudrate.{baud}={baud} - error 0% -> {v['note']}{eol_of(orig[j])}")
                        else:
                            out.append(f"{nb}.menu.baudrate.{baud}" + y.replace("115200", str(baud)))
            out.append(nb + rest)
        new_all += out + [eol]
        count += 1

    new = list(orig)
    new[max(idx) + 1:max(idx) + 1] = new_all if count else []

    diff = "".join(difflib.unified_diff(orig, new, "a/boards.txt", "b/boards.txt"))
    assert not [x for x in diff.splitlines() if x.startswith("-") and not x.startswith("---")], \
        "BLAD: diff usuwa linie!"
    out_name = "minicore_3.1.3_328p_14M31818_25M.diff"
    with open(out_name, "w", encoding="utf-8", newline="") as f:
        f.write(diff)
    print(f"Plyta '{b}': dodano nowych plyt: {count}, usunieto linii: 0")
    print("Wariant 328P zamrozony:", f"tak (menu.variant.{vid})" if vid else
          "nie (nie znaleziono menu.variant z 328P - menu Variant zostaje skopiowane)")
    if skipped: print("Pominieto (juz istnieja):", ", ".join(skipped))
    print(f"Diff zapisany: {out_name}")
    if "--apply" in sys.argv and count:
        shutil.copy(path, path + ".orig")
        with open(path, "w", encoding="utf-8", newline="") as f:
            f.writelines(new)
        print("boards.txt zaktualizowany (kopia: boards.txt.orig)")

if __name__ == "__main__":
    main()
