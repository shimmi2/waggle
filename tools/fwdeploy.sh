#!/bin/bash
# Rozvoz Waggle do projektů, které ho používají.
#
# Knihovna jsou dva soubory: fw.inc (server) a fw.js (klient). Nic víc se
# nikdy nekopíruje — kostra api/ se bere jednou při zrodu projektu ručně,
# dema app/ a app2/ se nekopírují vůbec.
#
# Proč kopírovat a ne symlinkovat: fw.inc je soubor, jehož chyba znamená,
# že projekt nejede vůbec. Se symlinkem by šla změna do všech projektů
# naráz a nedalo by se nasazovat po jednom. S kopií si každý projekt nese
# vlastní soubor, v jeho gitu je vidět „Waggle na 1.0.0" a nasazení je
# vědomý krok.
#
#   ./tools/fwdeploy.sh --check              stav všech cílů
#   ./tools/fwdeploy.sh <cesta>...           rozvoz z tohohle stromu (ptá se)
#   ./tools/fwdeploy.sh -y <cesta>...        rozvoz bez ptaní
#   ./tools/fwdeploy.sh --from v1.0.0 …      rozvoz z vydání na GitHubu
#   ./tools/fwdeploy.sh --from main --check  co by přišlo z hlavní větve
#
# Bez --from se bere tenhle strom, takže skript funguje i bez sítě a bez
# přihlášení. S --from se stáhnou oba soubory z daného tagu nebo větve —
# tím se dá rozvážet i na server, kde Waggle nemá pracovní kopii.
#
# Cíl je kořen projektu. Skript v něm sám najde, kam co patří:
#   fw.inc  ->  lib/fw.inc, api/fw.inc nebo fw.inc
#   fw.js   ->  fw.js, app/fw.js nebo public/fw.js
# Nic, co tam ještě není, nezakládá — jinak by překlep v cestě vyrobil
# nový soubor místo hlášky.
#
# Bez cest se bere seznam z tools/targets.local (jedna cesta na řádek,
# # je komentář). Ten soubor je mimo git, protože je to místní věc.

set -u
REPO="${WAGGLE_REPO:-shimmi2/waggle}"
HOME_DIR="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$HOME_DIR"
YES=0; CHECK=0; REF=''; TMP=''; CILE=()

while [ $# -gt 0 ]; do
    case "$1" in
        -y|--yes)   YES=1 ;;
        --check)    CHECK=1 ;;
        --from)     shift; REF="${1:-}"; [ -n "$REF" ] || { echo "--from chce tag nebo větev" >&2; exit 2; } ;;
        --repo)     shift; REPO="${1:-}" ;;
        -h|--help)  sed -n '2,32p' "$0"; exit 0 ;;
        -*)         echo "neznámý přepínač $1" >&2; exit 2 ;;
        *)          CILE+=("$1") ;;
    esac
    shift
done

uklid() { [ -n "$TMP" ] && rm -rf "$TMP"; }
trap uklid EXIT

vydani() {   # vytáhne číslo vydání ze souboru, ať je to .inc nebo .js
    local v
    v="$(grep -om1 "RELEASE[^0-9]*['\"]\([0-9.]*\)" "$1" 2>/dev/null \
        | grep -o "[0-9][0-9.]*" | tail -1)"
    echo "${v:-bez verze}"
}

# --- stažení vydání z GitHubu ----------------------------------------
# gh umí i soukromé repo, curl jen veřejné. Zkusí se to v tomhle pořadí.
stahni() {
    local ref="$1" f dir
    dir="$(mktemp -d)" || return 1
    for f in io.inc fw.inc fw.js; do
        if command -v gh >/dev/null 2>&1 && \
           gh api "repos/$REPO/contents/$f?ref=$ref" \
              -H "Accept: application/vnd.github.raw" > "$dir/$f" 2>/dev/null \
           && [ -s "$dir/$f" ]; then
            :
        elif command -v curl >/dev/null 2>&1 && \
             curl -fsL "https://raw.githubusercontent.com/$REPO/$ref/$f" -o "$dir/$f" 2>/dev/null \
             && [ -s "$dir/$f" ]; then
            :
        else
            echo "nepodařilo se stáhnout $f z $REPO@$ref" >&2
            echo "  (soukromé repo potřebuje přihlášené gh: gh auth status)" >&2
            rm -rf "$dir"; return 1
        fi
        # Useknutý nebo chybový soubor nesmí přepsat běžící projekt.
        if [ "$(vydani "$dir/$f")" = "bez verze" ]; then
            echo "stažený $f nevypadá jako Waggle (chybí RELEASE) — končím" >&2
            rm -rf "$dir"; return 1
        fi
    done
    echo "$dir"
}

if [ -n "$REF" ]; then
    TMP="$(stahni "$REF")" || exit 1
    SRC="$TMP"
fi

if [ ${#CILE[@]} -eq 0 ]; then
    L="$HOME_DIR/tools/targets.local"
    if [ ! -f "$L" ]; then
        echo "není co rozvážet: chybí $L a nedostal jsem cesty" >&2
        echo "založ ho, jedna cesta k projektu na řádek" >&2
        exit 2
    fi
    while IFS= read -r r; do
        r="${r%%#*}"; r="$(echo "$r" | xargs)"
        [ -n "$r" ] && CILE+=("$r")
    done < "$L"
fi

V_SRC="$(vydani "$SRC/fw.inc")"
[ "$V_SRC" != "bez verze" ] || { echo "ve zdroji $SRC/fw.inc není FW_RELEASE" >&2; exit 1; }

# fw.inc vyžaduje io.inc, takže spolu musí i verzovat. Rozejití by
# znamenalo projekt, kde půlka knihovny je z jiného vydání — a to se
# pozná až podle chování, ne podle chyby.
V_IO="$(vydani "$SRC/io.inc")"
[ "$V_IO" = "$V_SRC" ] || {
    echo "zdroj se rozešel: fw.inc je $V_SRC, io.inc $V_IO" >&2; exit 1; }

# Nová verze knihovny může přinést funkci, kterou si projekt už dávno
# napsal sám. PHP na dvojí deklaraci spadne fatální chybou a celé API
# začne vracet 500 — a pozná se to až po nasazení. Proto se to hlídá
# předem: vezmou se jména funkcí ze zdrojového fw.inc a hledají se
# v projektu všude jinde než v souboru, který se přepisuje.
# Patro = adresář, ve kterém běží JEDEN proces. Knihovní soubor svoje
# patro označuje tím, že v něm leží; když bydlí v inc/ nebo lib/, patří
# patro o úroveň výš, protože vstupní bod je tam.
patro() {
    local d; d="$(dirname "$1")"
    case "$(basename "$d")" in inc|lib) dirname "$d" ;; *) echo "$d" ;; esac
}

# Nová verze knihovny může přinést funkci, kterou si projekt už dávno
# napsal sám. PHP na dvojí deklaraci spadne fatální chybou ještě před
# prvním řádkem endpointu, takže nevrací 500 jedna stránka, ale všechno.
#
# Hledá se v celém patře, ale CIZÍ patra se vynechávají: třívrstvá
# aplikace má vlastní kopii knihovny v bff/ i v api/inc/ a ty dva
# procesy se v jednom include grafu nikdy nesejdou. Vynechat celý
# zbytek projektu by naopak minulo kolizi o adresář vedle — a přesně
# ta shodila vydání 1.2.2.
kolize() {
    local root="$1" cil="$2" f jmena hit nalez="" moje ciziptr cizi=""
    [ "${cil##*.}" = "inc" ] || return 0
    moje="$(patro "$cil")"
    while IFS= read -r ciziptr; do
        [ -n "$ciziptr" ] || continue
        local pt; pt="$(patro "$ciziptr")"
        [ "$pt" = "$moje" ] || cizi="$cizi $pt"
    done <<< "$(find "$root" \( -name io.inc -o -name fw.inc \) 2>/dev/null)"

    jmena="$(grep -hoE '^function [a-z_]+\(' "$SRC/fw.inc" "$SRC/io.inc" 2>/dev/null \
             | sed 's/^function //; s/($//' | sort -u)"
    [ -n "$jmena" ] || return 0
    while IFS= read -r f; do
        [ -n "$f" ] || continue
        hit="$(grep -rlE "^function +$f *\(" "$moje" --include='*.inc' --include='*.php' 2>/dev/null \
               | grep -vE '/(io|fw)\.inc$' | while IFS= read -r h; do
                     for c in $cizi; do case "$h" in "$c"/*) continue 2 ;; esac; done
                     echo "$h"
                 done | head -1)"
        [ -n "$hit" ] && nalez="$nalez\n        $f()  v  ${hit#$root/}"
    done <<< "$jmena"
    [ -z "$nalez" ] && return 0
    echo "      !! KOLIZE JMEN — nasazení by shodilo celé API na dvojí deklaraci:"
    printf "%b\n" "$nalez"
    echo "         Odstraň projektovou kopii, knihovna tu funkci má taky."
    return 1
}

najdi() {    # najdi $1=kořen projektu, $2=jméno souboru -> vypiš existující cíl
    local root="$1" f="$2" k
    for k in "lib/$f" "api/$f" "bff/$f" "$f" "app/$f" "public/$f"; do
        [ -f "$root/$k" ] && { echo "$root/$k"; return 0; }
    done
    return 1
}

if [ -n "$REF" ]; then
    echo "zdroj: $REPO@$REF (vydání $V_SRC)"
else
    echo "zdroj: $SRC (vydání $V_SRC)"
fi

# Syntaktická kontrola ZDROJE, jednou pro všechny cíle. Rozvézt rozbitý
# fw.inc znamená 500 na celém projektu a pozná se to až po nasazení —
# README dosud tvrdilo, že se kontroluje, a kontrolovalo se jen to, že
# v souboru někde stojí RELEASE. Ten je na patnáctém řádku ze tří set,
# takže useknutý soubor tou kontrolou prošel.
for f in io.inc fw.inc; do
    if command -v php >/dev/null 2>&1; then
        php -l "$SRC/$f" >/dev/null 2>&1 || {
            echo "zdrojový $f neprojde php -l — nerozvážím nic" >&2; exit 1; }
    fi
done
if command -v node >/dev/null 2>&1; then
    node --check "$SRC/fw.js" >/dev/null 2>&1 || {
        echo "zdrojový fw.js neprojde node --check — nerozvážím nic" >&2; exit 1; }
fi

# Atomický zápis: vedle cíle a přejmenovat. cp -f přepisuje na místě,
# takže běžící požadavek může načíst napůl zapsaný soubor.
poloz() {
    local zdroj="$1" cil="$2" tmp
    tmp="$(dirname "$cil")/.fwdeploy.$$.$(basename "$cil")"
    cp -f "$zdroj" "$tmp" || { rm -f "$tmp"; return 1; }
    chmod --reference="$cil" "$tmp" 2>/dev/null || true
    mv -f "$tmp" "$cil" || { rm -f "$tmp"; return 1; }
}

CHYBY=0
for root in "${CILE[@]}"; do
    if [ ! -d "$root" ]; then
        echo "  !! $root — adresář neexistuje"; CHYBY=1; continue
    fi
    echo "  $root"

    # Kolize v jednom souboru zastaví CELÝ projekt. Dřív se přeskočil jen
    # fw.inc a fw.js se rozvezl — projekt pak měl každou půlku knihovny
    # z jiné verze, což je horší než nerozvézt nic.
    PRESKOC=0
    for f in io.inc fw.inc; do
        cil="$(najdi "$root" "$f")" || continue
        # Jména se berou z obou knihovních souborů, takže druhý průchod by
        # vypsal totéž. Stačí první nález.
        kolize "$root" "$cil" || { PRESKOC=1; break; }
    done
    if [ $PRESKOC -eq 1 ]; then
        echo "      nerozvážím do tohohle projektu nic, dokud kolize trvá"
        CHYBY=1; continue
    fi

    for f in io.inc fw.inc fw.js; do
        cil="$(najdi "$root" "$f")" || {
            # io.inc je nový v 1.6.0 a fw.inc ho VYŽADUJE. Kdyby se
            # rozvezl jen fw.inc, spadne projekt na chybějícím require
            # hned prvním požadavkem. Proto se zakládá vedle fw.inc.
            if [ "$f" = "io.inc" ] && cil_fw="$(najdi "$root" fw.inc)"; then
                cil="$(dirname "$cil_fw")/io.inc"
                echo "      ${cil#$root/} — chybí, zakládám vedle fw.inc"
                [ $CHECK -eq 1 ] && continue
                poloz "$SRC/io.inc" "$cil" && echo "        založeno na $V_SRC" || CHYBY=1
                continue
            fi
            echo "      $f — v projektu není, přeskakuji"; continue; }
        rel="${cil#$root/}"
        if cmp -s "$SRC/$f" "$cil"; then
            echo "      $rel — shodné ($(vydani "$cil"))"
            continue
        fi
        echo "      $rel — LIŠÍ SE (v projektu $(vydani "$cil"), $(diff "$SRC/$f" "$cil" | grep -c '^[<>]') řádků)"
        [ $CHECK -eq 1 ] && continue
        if [ $YES -eq 0 ]; then
            read -r -p "        přepsat? [a/N] " o </dev/tty
            case "$o" in a|A|y|Y) ;; *) echo "        ponecháno"; continue ;; esac
        fi
        poloz "$SRC/$f" "$cil" || { CHYBY=1; continue; }
        echo "        rozvezeno na $V_SRC"
    done
done
exit $CHYBY
