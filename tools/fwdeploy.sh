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
    for f in fw.inc fw.js; do
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

najdi() {    # najdi $1=kořen projektu, $2=jméno souboru -> vypiš existující cíl
    local root="$1" f="$2" k
    for k in "lib/$f" "api/$f" "$f" "app/$f" "public/$f"; do
        [ -f "$root/$k" ] && { echo "$root/$k"; return 0; }
    done
    return 1
}

if [ -n "$REF" ]; then
    echo "zdroj: $REPO@$REF (vydání $V_SRC)"
else
    echo "zdroj: $SRC (vydání $V_SRC)"
fi

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
        cp -f "$SRC/$f" "$cil" || { CHYBY=1; continue; }
        echo "        rozvezeno na $V_SRC"
    done
done
exit $CHYBY
