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
 *    ip:1.2.3.4   Podle IP samotné to nejde: za jednou adresou sedí
 *                 celá firemní síť i celý mobilní operátor.
 *    user:jmeno   Podle jména samotného je to HORŠÍ NEŽ NIC: útočník
 *                 pětkrát selže na 'admin' a zamkne ho právoplatnému
 *                 uživateli.
 *
 *  Účet se proto NIKDY nezamyká natrvalo, jen se odmítá v okně. Po něm
 *  je to samo zpátky, bez zásahu správce.
 * ===================================================================== */

function throttle_keys(string $login): array {
    $ip = (string)($_SERVER['REMOTE_ADDR'] ?? '0.0.0.0');
    return ['ip:' . $ip, 'user:' . mb_strtolower($login)];
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
