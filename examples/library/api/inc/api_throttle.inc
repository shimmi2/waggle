<?php
/* =====================================================================
 *  api_throttle.inc — omezení pokusů o přihlášení
 *
 *  Hlavní důvod, proč to tu je, není uhádnuté heslo. Je to CENA
 *  OVĚŘENÍ: password_verify() s bcryptem trvá na běžném stroji kolem
 *  150 ms, kdežto přičtení v Redisu 0,07 ms. Šest útočníků paralelně
 *  vytíží jádro, aniž by cokoli uhodli. Škrtič chrání server před
 *  náklady jeho vlastního hashování.
 *
 *  Proto se volá PŘED hledáním uživatele, ne až před ověřením hesla —
 *  zahlcení se tak nedotkne ani databáze.
 *
 *  Dva klíče zvlášť:
 *    ip:1.2.3.4         Kolik zkouší jedna adresa, na jakákoli jména.
 *                       Vyšší limit: za jednou adresou sedí celá
 *                       firemní síť i celý mobilní operátor.
 *    ipuser:1.2.3.4|jm  Kolik zkouší jedna adresa na jedno jméno.
 *                       Podle jména SAMOTNÉHO to být nesmí — útočník
 *                       by pětkrát selhal na 'admin' z čehokoli a zamkl
 *                       ho právoplatnému uživateli, donekonečna. Takhle
 *                       si zamkne jen svou dvojici; admin z jiné adresy
 *                       se přihlásí.
 *
 *  Účet se proto NIKDY nezamyká natrvalo, jen se odmítá v okně. Po něm
 *  je to samo zpátky, bez zásahu správce.
 * ===================================================================== */

/* IP klienta, ne BFF.

   Ve třívrstvém nasazení volá tohle API jen BFF, takže REMOTE_ADDR je
   vždy adresa BFF. Klíč ip: by pak byl jeden pro všechny: dvacet
   neúspěchů na libovolná jména a každé přihlášení kohokoli vrací 429,
   donekonečna a bez účtu. BFF proto posílá IP klienta parametrem
   client_ip vedle jména a hesla.

   Parametr se bere JEN od BFF. Kdo na API dosáhne přímo (instalace pod
   jednou doménou), by si ho jinak nastavil sám a škrtiči utekl. BFF se
   pozná podle adresy: na stejném stroji je to loopback nebo vlastní
   adresa serveru, jinak to, co je v BFF_IPS. Od kohokoli jiného platí
   REMOTE_ADDR. */
function klient_ip(): string {
    $remote = (string)($_SERVER['REMOTE_ADDR'] ?? '');
    $bff = (defined('BFF_IPS') && BFF_IPS !== '')
         ? array_map('trim', explode(',', BFF_IPS))
         : ['127.0.0.1', '::1', (string)($_SERVER['SERVER_ADDR'] ?? '')];
    if ($remote !== '' && in_array($remote, $bff, true)) {
        $ip = in_str('client_ip', 45);
        if (filter_var($ip, FILTER_VALIDATE_IP) !== false) return $ip;
    }
    return $remote !== '' ? $remote : '0.0.0.0';
}

function throttle_keys(string $login): array {
    /* Dvojice se hashuje: IPv6 (45 znaků) a jméno (64) by se do lf_key
       VARCHAR(64) nevešly a useknuté klíče by se pomíchaly. */
    $ip = klient_ip();
    return ['ip:' . $ip,
            'ipuser:' . substr(hash('sha256', $ip . '|' . mb_strtolower($login)), 0, 40)];
}

function throttle_check(string $login): void {
    [$kip, $kuser] = throttle_keys($login);
    $limity = ['ip' => (int)LOGIN_FAIL_PER_IP, 'user' => (int)LOGIN_FAIL_PER_USER];

    if ($r = redis()) {
        $a = (int)$r->get(REDIS_PREFIX . 'fail.' . $kip);
        $b = (int)$r->get(REDIS_PREFIX . 'fail.' . $kuser);
    } else {
        $a = (int)q_val("SELECT COUNT(*) FROM login_fails WHERE lf_key=? AND lf_when > NOW() - INTERVAL ? SECOND",
                        [$kip, (int)LOGIN_FAIL_WINDOW]);
        $b = (int)q_val("SELECT COUNT(*) FROM login_fails WHERE lf_key=? AND lf_when > NOW() - INTERVAL ? SECOND",
                        [$kuser, (int)LOGIN_FAIL_WINDOW]);
    }
    if ($a >= $limity['ip'] || $b >= $limity['user'])
        api_error(429, 'Příliš mnoho pokusů o přihlášení. Zkuste to za chvíli.');
}

function throttle_fail(string $login): void {
    [$kip, $kuser] = throttle_keys($login);

    if ($r = redis()) {
        /* INCR je atomický, takže žádné závody. EXPIRE se nastavuje jen
           při prvním přičtení — jinak by okno šlo posouvat do nekonečna
           a útočník by se z něj nikdy nedostal, což je zbytečně kruté. */
        foreach ([$kip, $kuser] as $k)
            if ($r->incr(REDIS_PREFIX . 'fail.' . $k) === 1)
                $r->expire(REDIS_PREFIX . 'fail.' . $k, (int)LOGIN_FAIL_WINDOW);
        return;
    }

    /* U MySQL varianty je tohle důležité: jakmile je klíč za limitem,
       PŘESTANEME vkládat. Bez toho je každý neúspěšný pokus zápis na
       disk a útočník tě tím škrtičem udusí — ochrana by se stala
       zesilovačem. Takhle je počet zápisů zastropovaný. */
    foreach ([$kip => (int)LOGIN_FAIL_PER_IP, $kuser => (int)LOGIN_FAIL_PER_USER] as $k => $limit) {
        $n = (int)q_val("SELECT COUNT(*) FROM login_fails WHERE lf_key=? AND lf_when > NOW() - INTERVAL ? SECOND",
                        [$k, (int)LOGIN_FAIL_WINDOW]);
        if ($n < $limit) q("INSERT INTO login_fails (lf_key) VALUES (?)", [$k]);
    }
}

function throttle_ok(string $login): void {
    [$kip, $kuser] = throttle_keys($login);
    if ($r = redis()) { $r->del(REDIS_PREFIX . 'fail.' . $kip, REDIS_PREFIX . 'fail.' . $kuser); return; }
    q("DELETE FROM login_fails WHERE lf_key IN (?,?)", [$kip, $kuser]);
}
