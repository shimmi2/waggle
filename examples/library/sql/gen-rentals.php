<?php
/* =====================================================================
 *  gen-rentals.php — spuštění generátoru z příkazové řádky
 *
 *  Vlastní práci dělá gen_rentals() v gen-rentals.inc. Tenhle soubor je
 *  jen obal: přečte přepínače, otevře spojení, zavolá a vypíše souhrn.
 *  Webový instalák si tu funkci volá přímo, bez shellu.
 *
 *  Použití:
 *    php gen-rentals.php --dsn=... --user=Y --pass=Z [--days=730] [--copies=1] [--readers=0]
 * ===================================================================== */

if (PHP_SAPI !== 'cli') { http_response_code(403); exit("cli only\n"); }

require __DIR__ . '/gen-rentals.inc';

$o = getopt('', ['dsn:', 'user:', 'pass:', 'days::', 'copies::', 'readers::', 'help']);
if (isset($o['help']) || !isset($o['dsn'])) {
    fwrite(STDERR, "php gen-rentals.php --dsn=... --user=Y --pass=Z"
                 . " [--days=730] [--copies=1] [--readers=0]\n");
    exit(isset($o['help']) ? 0 : 1);
}

try {
    $db = new PDO($o['dsn'], $o['user'] ?? '', $o['pass'] ?? '',
                  [PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION]);
    $db->exec('SET NAMES utf8mb4');
    $s = gen_rentals($db, (int)($o['days'] ?? 730), (int)($o['copies'] ?? 1), (int)($o['readers'] ?? 0));
} catch (Throwable $e) {
    fwrite(STDERR, '  ' . $e->getMessage() . "\n");
    exit(1);
}

printf("  %d operací, %d otevřených (%d po termínu), %d čtenářů, %d dní, násobič kusů %d\n",
       $s['operaci'], $s['otevrenych'], $s['po_terminu'], $s['ctenaru'], $s['dny'], $s['nasobic']);
