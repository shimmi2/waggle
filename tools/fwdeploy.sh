#!/bin/bash
# Rozvoz Waggle do projektů, které ho používají.
#
# Proč kopírovat a ne symlinkovat: fw.inc je soubor, jehož chyba znamená,
# že projekt nejede vůbec. Se symlinkem by šla změna do všech projektů
# naráz a nedalo by se nasazovat po jednom. S kopií si každý projekt nese
# vlastní soubor, v jeho gitu je vidět „framework na 1.0.0" a nasazení je
# vědomý krok.
#
#   ./tools/fwdeploy.sh --check            stav všech cílů
#   ./tools/fwdeploy.sh --check <cesta>    stav jednoho
#   ./tools/fwdeploy.sh <cesta>...         rozvoz (ptá se)
#   ./tools/fwdeploy.sh -y <cesta>...      rozvoz bez ptaní
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
SRC="$(cd "$(dirname "$0")/.." && pwd)"
YES=0; CHECK=0; CILE=()

for a in "$@"; do
    case "$a" in
        -y|--yes)   YES=1 ;;
        --check)    CHECK=1 ;;
        -h|--help)  sed -n '2,25p' "$0"; exit 0 ;;
        -*)         echo "neznámý přepínač $a" >&2; exit 2 ;;
        *)          CILE+=("$a") ;;
    esac
done

if [ ${#CILE[@]} -eq 0 ]; then
    L="$SRC/tools/targets.local"
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

vydani() {   # vytáhne číslo vydání ze souboru, ať je to .inc nebo .js
    local v
    v="$(grep -om1 "RELEASE[^0-9]*['\"]\([0-9.]*\)" "$1" 2>/dev/null \
        | grep -o "[0-9][0-9.]*" | tail -1)"
    echo "${v:-bez verze}"
}
V_SRC="$(vydani "$SRC/fw.inc")"
[ "$V_SRC" != "bez verze" ] || { echo "ve zdroji $SRC/fw.inc není FW_RELEASE" >&2; exit 1; }

najdi() {    # najdi $1=kořen projektu, $2=jméno souboru -> vypiš existující cíl
    local root="$1" f="$2" k
    for k in "lib/$f" "api/$f" "$f" "app/$f" "public/$f"; do
        [ -f "$root/$k" ] && { echo "$root/$k"; return 0; }
    done
    return 1
}

echo "zdroj: $SRC (vydání $V_SRC)"
CHYBY=0
for root in "${CILE[@]}"; do
    if [ ! -d "$root" ]; then
        echo "  !! $root — adresář neexistuje"; CHYBY=1; continue
    fi
    echo "  $root"
    for f in fw.inc fw.js; do
        cil="$(najdi "$root" "$f")" || { echo "      $f — v projektu není, přeskakuji"; continue; }
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
        cp -a "$SRC/$f" "$cil" || { CHYBY=1; continue; }
        echo "        rozvezeno na $V_SRC"
    done
done
exit $CHYBY
