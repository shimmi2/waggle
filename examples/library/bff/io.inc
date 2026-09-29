<?php
/* =====================================================================
 *  io.inc — hygiena vstupu a výstupu. Knihovna, needituj v projektu.
 *
 *  Tenhle soubor o Waggle neví nic a vědět nemá. Čte se z $_REQUEST,
 *  escapuje se do HTML, kóduje se JSON — věci, které potřebuje každá
 *  PHP vrstva, která mluví přes HTTP, ať protokol nahoře vypadá jakkoli.
 *
 *  Proto je oddělený od fw.inc: třívrstvá aplikace má backendové API,
 *  které protokol mezi BFF a prohlížečem načítat nemá, ale tutéž hygienu
 *  vstupů potřebuje úplně stejně. Dvě kopie téhož by se dřív nebo
 *  později rozešly — a vstupy jsou to poslední, co se smí rozejít.
 *
 *  fw.inc si tenhle soubor načítá sám. Vrstva bez Waggle si ho načte
 *  přímo. NIKDY obojí v jednom procesu: PHP spadne na dvojí deklaraci
 *  ještě před prvním řádkem endpointu, takže nevrací 500 jedna stránka,
 *  ale všechno. Hlídá to tools/fwdeploy.sh.
 *
 *  ---------------------------------------------------------------------
 *  VSTUP — jediné tři garance. Nic víc, nic míň.
 *
 *    skalární string, omezená délka, bez NUL bajtu.
 *
 *  Žádné escapování. „Bezpečné" není vlastnost hodnoty, ale dvojice
 *  hodnota + cíl: co je bezpečné do HTML, není bezpečné do SQL ani do
 *  shellu. Kdyby se escapovalo tady, uloží se do databáze O&#039;Brien
 *  a nikdo už nikdy nezjistí, jak se ten člověk jmenuje.
 * ------------------------------------------------------------------- */

defined('FW_RELEASE')    || define('FW_RELEASE', '1.9.1');

/* Tvrdý strop délky jednoho pole. Volající si smí říct o méně, o víc ne —
   jinak by jedno zapomenuté $max znamenalo, že si někdo pošle megabajty. */
defined('FW_MAXLEN')     || define('FW_MAXLEN', 65536);

/* Vstupy se berou z GET i POST. Vědomé ustoupení kvůli kompatibilitě se
   staršími projekty, kde část endpointů vede na odkaz. Na bezpečnost to
   vliv nemá — ošetřuje se to stejně. */
function in_str(string $name, int $max = 4096): string {
    if ($max > FW_MAXLEN) $max = FW_MAXLEN;
    $v = $_REQUEST[$name] ?? '';
    if (!is_string($v))              return '';   // ?x[]=1 nesmí shodit endpoint
    if (isset($v[$max]))             $v = utf8_rez($v, $max);
    if (strpos($v, "\0") !== false)  $v = str_replace("\0", '', $v);
    return $v;
}

/* Ořez na BAJTY, ale nikdy doprostřed znaku.
 *
 *  Prosté substr() umí strop trefit mezi dva bajty jednoho „ž" a nechat
 *  za sebou půlku. Výsledek pak není platné UTF-8, json_encode() vrátí
 *  false — a protože to nikdo nekontroloval, odešla klientovi prázdná
 *  odpověď s HTTP 200. Ani chyba, ani data; ladí se to hodinu.
 *
 *  Bez mbstring, protože to je volitelné rozšíření: odřízne se nedokončená
 *  úvodní sekvence na konci. Úvodní bajt sám nikdy platný znak není. */
function utf8_rez(string $v, int $max): string {
    $v = substr($v, 0, $max);
    return (string)preg_replace(
        '/(?:[\xC0-\xDF]|[\xE0-\xEF][\x80-\xBF]?|[\xF0-\xF7][\x80-\xBF]{0,2})$/', '', $v);
}

function in_int(string $name): int     { return intval(in_str($name, 32));   }
function in_float(string $name): float { return floatval(in_str($name, 64)); }

/* "slovo" — bezpečné pro názvy funkcí a části cest. Čistý strspn, 41 ns.
   Pozor na jméno: is_* NIC NEČTE, jen odpoví ano/ne nad hodnotou, kterou
   už držíš. Proto se nejmenuje in_word() — to by vypadalo jako getter a
   is_word('op') by se dalo splést s „přečti parametr op". */
function is_word(string $s): bool {
    static $OK = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-';
    return $s !== '' && strspn($s, $OK) === strlen($s);
}

/* Tabulkový formulář posílá fd[i][sloupec]. Skalární getter takové pole
   schválně nepustí, takže je na ně tenhle — a platí pro něj TYTÉŽ tři
   garance: string omezené délky bez NUL, nic víc.

   Propustí jen dvojúrovňové pole skalárů. Hlubší zanoření, objekty ani
   skaláry na první úrovni neprojdou; nepadá se na nich, jen se
   přeskočí. Stropy jsou dva, na počet řádků i na délku hodnoty —
   bez nich by jediný požadavek mohl vyrobit statisíce prvků.

   Klíče řádků i sloupců zůstávají tak, jak přišly, a jsou to vstup
   od klienta jako každý jiný: do SQL ani do HTML nepatří bez ošetření.
   Knihovna je nepřepisuje, protože jen volající ví, co znamenají. */
function in_rows(string $name, int $maxRows = 500, int $maxLen = 4096): array {
    $v = $_REQUEST[$name] ?? null;
    if (!is_array($v)) return [];
    $out = [];
    foreach ($v as $i => $row) {
        if (count($out) >= $maxRows) break;
        if (!is_array($row)) continue;
        $r = [];
        foreach ($row as $k => $cv) {
            if (!is_scalar($cv)) continue;
            $cv = (string)$cv;
            if (isset($cv[$maxLen]))         $cv = utf8_rez($cv, $maxLen);
            if (strpos($cv, "\0") !== false) $cv = str_replace("\0", '', $cv);
            $r[(string)$k] = $cv;
        }
        $out[(string)$i] = $r;
    }
    return $out;
}

/* Hlavička požadavku se stejnou hygienou jako $_REQUEST. Token sezení
   jde VŽDY hlavičkou, nikdy v URL — v URL by skončil v access logu,
   v Referer při odchodu na cizí web, v historii prohlížeče a v cache
   proxy. */
function in_header(string $name, int $max = 1024): string {
    $v = $_SERVER['HTTP_' . strtr(strtoupper($name), '-', '_')] ?? '';
    if (!is_string($v))              return '';
    if (isset($v[$max]))             $v = substr($v, 0, $max);
    if (strpos($v, "\0") !== false)  $v = str_replace("\0", '', $v);
    return $v;
}

/* ---------------------------------------------------------------------
 *  Původní jména z vydání 1.4.0 a starších. Zůstávají a v protokolu v1
 *  se neodeberou — visí na nich stovky volání v existujících projektech
 *  a přejmenovat je hromadně v živém kódu je riziko bez užitku.
 *
 *  Jsou schválně tady a ne schované ve fw.inc: kdo otevře tenhle soubor,
 *  má na jednom místě vidět obě jména i to, které z nich je dnešní.
 *  V novém kódu piš in_*.
 * ------------------------------------------------------------------- */
function req(string $name, int $max = 4096): string { return in_str($name, $max); }
function req_int(string $name): int                 { return in_int($name); }
function req_float(string $name): float             { return in_float($name); }
function req_rows(string $name, int $maxRows = 500, int $maxLen = 4096): array {
    return in_rows($name, $maxRows, $maxLen);
}
function req_header(string $name, int $max = 1024): string { return in_header($name, $max); }

/* ---------------------------------------------------------------------
 *  VÝSTUP — druhá polovina té dvojice. Escapuje se až tady, protože
 *  teprve tady je znám CÍL.
 * ------------------------------------------------------------------- */

/* Do HTML nejde nic bez tohohle. ENT_QUOTES kvůli hodnotám v atributech,
   ENT_SUBSTITUTE proto, aby neplatné UTF-8 nevrátilo prázdný string —
   tiché zmizení textu se ladí hůř než otazník. */
function esc(?string $s): string {
    return htmlspecialchars((string)$s, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8');
}

/* Jedno místo, kde se rozhoduje, jak vypadá JSON na drátě. Příznaky se
   nesmějí rozejít mezi vrstvami: UNESCAPED_UNICODE kvůli diakritice
   (jinak je odpověď třikrát delší a v logu nečitelná), UNESCAPED_SLASHES
   kvůli URL. */
function json_text(array $data): string {
    $t = json_encode($data, JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
    if ($t !== false) return $t;
    /* Neplatné UTF-8 nemusí přijít jen ze vstupu — umí ho podat i databáze
       v latin1 nebo cizí API. Prázdná odpověď je ta nejhorší možná reakce,
       takže se to zkusí znovu s náhradou za U+FFFD. */
    $t = json_encode($data, JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES
                          | JSON_INVALID_UTF8_SUBSTITUTE);
    if ($t !== false) { error_log('[fw] json_text: neplatné UTF-8 nahrazeno'); return $t; }
    error_log('[fw] json_text selhal: ' . json_last_error_msg());
    return '';
}
