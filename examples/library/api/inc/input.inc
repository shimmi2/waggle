<?php
/* =====================================================================
 *  input.inc — vstupy backendového API
 *
 *  Vlastní, malá vrstva. Backend SCHVÁLNĚ nepoužívá fw.inc: Waggle je
 *  protokol mezi BFF a prohlížečem, kdežto tady jde o čistý JSON.
 *  Kdyby backend na framework sahal, nedal by se přepsat do Pythonu,
 *  aniž by se s ním tahal i ten protokol — a přitom o něm nemá vědět.
 *
 *  Garance jsou stejné tři jako ve frameworku, aby se to učilo jednou:
 *  skalární string omezené délky bez NUL bajtu. Nic víc. Žádné
 *  escapování, protože „bezpečné" není vlastnost hodnoty, ale dvojice
 *  hodnota + cíl.
 * ===================================================================== */

/* Vstupy se berou z GET i POST. Tohle je vědomé ustoupení kvůli
   kompatibilitě se staršími API autora, kde se část endpointů volá
   odkazem. Na bezpečnost to vliv nemá — ošetřuje se to stejně. */
function in_str(string $name, int $max = 4096): string {
    $v = $_REQUEST[$name] ?? '';
    if (!is_string($v))             return '';        // ?x[]=1 nesmí shodit endpoint
    if (isset($v[$max]))            $v = substr($v, 0, $max);
    if (strpos($v, "\0") !== false) $v = str_replace("\0", '', $v);
    return $v;
}

function in_int(string $name): int     { return intval(in_str($name, 32));   }
function in_float(string $name): float { return floatval(in_str($name, 64)); }

/* Je to jedno slovo? Používá se všude, kde hodnota vstupuje do cesty
   k souboru nebo do jména sloupce. Nepustí '/', '.' ani NUL, takže se
   z ní cesta nedá vylomit. */
function is_word(string $s): bool {
    static $OK = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-';
    return $s !== '' && strspn($s, $OK) === strlen($s);
}

/* Dvouúrovňové pole skalárů z tabulkového formuláře: fd[i][sloupec].
   Stropy na počet řádků i délku hodnoty tu jsou proto, že bez nich je
   jediný parametr DoS. Klíče jsou taky vstup od klienta — do SQL ani
   do HTML nepatří bez ošetření. */
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
            if (isset($cv[$maxLen]))        $cv = substr($cv, 0, $maxLen);
            if (strpos($cv, "\0") !== false) $cv = str_replace("\0", '', $cv);
            $r[(string)$k] = $cv;
        }
        $out[(string)$i] = $r;
    }
    return $out;
}

/* Hlavička požadavku. Token sezení jde VŽDY hlavičkou, nikdy v URL —
   v URL by skončil v access logu, v Referer při odchodu na cizí web,
   v historii prohlížeče a v cache proxy. */
function in_header(string $name, int $max = 1024): string {
    $k = 'HTTP_' . strtoupper(str_replace('-', '_', $name));
    $v = $_SERVER[$k] ?? '';
    if (!is_string($v))  return '';
    if (isset($v[$max])) $v = substr($v, 0, $max);
    return str_replace("\0", '', $v);
}
