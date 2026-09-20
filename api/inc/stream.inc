<?php
/* stream.inc — push kanály. Věc PROJEKTU, ne frameworku:
   framework umí jen fw_publish(), kde se berou tokeny je na aplikaci.
   Nastavení (porty, URL, TTL) je v config.inc. */


/* URL odběru se odvozuje z hostu, na kterém aplikace běží. Kdyby byla
   natvrdo, rozešly by se simek.info a www.simek.info a vznikl by
   zbytečný cross-origin problém. Host je vstup od klienta -> kontrola. */
function stream_sub_url(): string {
    static $OK = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-';
    $h = (string)($_SERVER['HTTP_HOST'] ?? '');
    $c = strpos($h, ':');
    if ($c !== false) $h = substr($h, 0, $c);
    if ($h === '' || strspn($h, $OK) !== strlen($h)) $h = 'localhost';
    return 'https://' . $h . ':' . STREAM_SUB_PORT . '/nchan/sub';
}

/* Token pro dvojici (session, kanál). Jednoúčelový a neuhodnutelný —
   session token by skončil v access logu nginxu.

   POZOR, tohle je DEMO úložiště. Dva souběžné požadavky si můžou
   navzájem přepsat soubor, protože tu není zamykání. V produkci sem
   patří Redis nebo databáze, ne JSON na disku. */
function stream_token(string $session, string $name): string {
    if ($session === '' || !is_word($name)) return '';

    $all = json_decode((string)@file_get_contents(STREAM_STORE), true);
    if (!is_array($all)) $all = [];

    $now = time();
    foreach ($all as $k => $v) if (($v['exp'] ?? 0) < $now) unset($all[$k]);

    $key = $name . '|' . $session;
    if (!isset($all[$key])) $all[$key] = ['t' => bin2hex(random_bytes(16))];
    $all[$key]['exp'] = $now + STREAM_TTL;

    file_put_contents(STREAM_STORE, json_encode($all));
    return $all[$key]['t'];
}

/* Příkaz, kterým server řekne klientovi, kde má poslouchat. */
function stream_subscribe_cmd(string $session, string $name = 'main'): array {
    return ['op' => 'subscribe', 'name' => $name,
            'url' => stream_sub_url(), 'token' => stream_token($session, $name)];
}

/* Spustí workera na pozadí. Token jde dočasným souborem, NE přes argv —
   argv je v `ps` vidět všem uživatelům stroje. */
function stream_spawn(string $worker, array $args): void {
    $tmp = tempnam(sys_get_temp_dir(), 'fwjob');
    chmod($tmp, 0600);
    file_put_contents($tmp, json_encode($args));

    @exec(sprintf('%s %s %s >/dev/null 2>&1 &',
                  escapeshellarg(STREAM_PHP), escapeshellarg($worker), escapeshellarg($tmp)));
}
