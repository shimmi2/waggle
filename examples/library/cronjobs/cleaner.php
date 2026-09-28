<?php
/* =====================================================================
 *  cleaner.php — úklid věcí s omezenou životností
 *
 *  BEZ SHEBANGU schválně. PHP ho odstraňuje jen v CLI; když se soubor
 *  dostane pod webserver, pošle ho jako text — a tím odešle bajt před
 *  hlavičkami, takže pozdější http_response_code() už nemá co nastavit.
 *  Ochranou je kontrola PHP_SAPI o několik řádků níž, ne shebang.
 *  Cron se tedy volá s interpretem: /usr/bin/php cleaner.php
 *
 *  Pouští se z cronu, řekněme každých pět minut:
 *      *_/5 * * * *  /usr/bin/php /cesta/cronjobs/cleaner.php
 *  (bez podtržítka, to je tu jen proto, aby to nezavíralo komentář)
 *
 *  POTŘEBA JE JEN TEHDY, KDYŽ NENÍ REDIS. Redis si klíče maže sám podle
 *  TTL, takže s ním tenhle skript nemá co dělat — ohlásí to a skončí.
 *
 *  Uklízí tři věci:
 *    1. sezení v MySQL, ke kterým se nikdo dlouho nevrátil
 *    2. počítadla neúspěšných přihlášení po uplynutí okna
 *    3. krátkou cache BFF (soubory starší než její životnost)
 *
 *  Nic z toho není kritické pro správnost — vypršené sezení se odmítne
 *  i tehdy, když řádek ještě leží v tabulce, protože se kontroluje čas.
 *  Tenhle skript jen brání tomu, aby tabulky rostly navěky.
 * ===================================================================== */

if (PHP_SAPI !== 'cli') { http_response_code(403); exit("cli only\n"); }

/* Konfigurace se bere z API, protože jen ona zná databázi. Když se
   skript pouští z jiného stroje, ukaž mu ji přepínačem. */
/* DVĚ konfigurace, protože každá vrstva zná jen své: databázi API,
   adresář cache BFF. Když vrstvy běží na různých strojích, spustí se
   tenhle skript na každém a ta druhá cesta se prostě nezadá. */
$o = getopt('', ['api-config::', 'bff-config::', 'quiet', 'help']);
if (isset($o['help'])) {
    fwrite(STDERR, "php cleaner.php [--api-config=…/api/config.inc] [--bff-config=…/bff/config.inc] [--quiet]\n");
    exit(0);
}
$cfg_api = $o['api-config'] ?? __DIR__ . '/../api/config.inc';
$cfg_bff = $o['bff-config'] ?? __DIR__ . '/../bff/config.inc';

$mam_api = is_file($cfg_api); $mam_bff = is_file($cfg_bff);
if (!$mam_api && !$mam_bff) {
    fwrite(STDERR, "nenašel jsem ani jednu konfiguraci ($cfg_api, $cfg_bff)\n");
    exit(1);
}
if ($mam_api) require $cfg_api;
if ($mam_bff) require $cfg_bff;

$tise = isset($o['quiet']);
function hlas(string $s): void { global $tise; if (!$tise) echo "  $s\n"; }

$smazano = 0;

/* ---- 1 a 2: databáze ----------------------------------------------- */
if (!$mam_api) {
    hlas('konfigurace API tu není — databázi uklízí jiný stroj.');
} elseif (REDIS_HOST !== '' && class_exists('Redis')) {
    hlas('Redis je nastavený — sezení i počítadla si maže sám, nic tu nedělám.');
} else {
    try {
        $db = new PDO(DB_DSN, DB_USER, DB_PASS, [PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION]);

        $s = $db->prepare("DELETE FROM sessions WHERE se_touched < NOW() - INTERVAL ? SECOND");
        $s->execute([(int)SESSION_TTL]);
        $n = $s->rowCount(); $smazano += $n;
        hlas("sezení: $n");

        $s = $db->prepare("DELETE FROM login_fails WHERE lf_when < NOW() - INTERVAL ? SECOND");
        $s->execute([(int)LOGIN_FAIL_WINDOW]);
        $n = $s->rowCount(); $smazano += $n;
        hlas("počítadel přihlášení: $n");
    } catch (Throwable $e) {
        fwrite(STDERR, "databáze: " . $e->getMessage() . "\n");
        exit(1);
    }
}

/* ---- 3: cache BFF --------------------------------------------------
   Může běžet na jiném stroji než API; pak tenhle adresář neexistuje
   a není to chyba. */
$dir = defined('BFF_CACHE_DIR') ? rtrim(BFF_CACHE_DIR, '/') : '';
if ($dir !== '' && is_dir($dir)) {
    $ttl = defined('BFF_CACHE_TTL') ? (int)BFF_CACHE_TTL : 60;
    $n = 0;
    /* Mazat podle mtime, nikoli podle jména. Jméno je hash tokenu, takže
       z něj nic vyčíst nejde — a to je záměr. */
    foreach (glob($dir . '/*.json') ?: [] as $f)
        if (time() - (int)@filemtime($f) > $ttl && @unlink($f)) $n++;
    /* Zapomenuté .tmp po spadlém zápisu. */
    foreach (glob($dir . '/*.tmp') ?: [] as $f)
        if (time() - (int)@filemtime($f) > 300 && @unlink($f)) $n++;
    $smazano += $n;
    hlas("souborů cache BFF: $n");
} elseif ($dir !== '') {
    hlas("cache BFF ($dir) tu není — běží asi na jiném stroji.");
}

hlas("celkem smazáno: $smazano");
exit(0);
