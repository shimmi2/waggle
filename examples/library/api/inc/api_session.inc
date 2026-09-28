<?php
/* =====================================================================
 *  api_session.inc — sezení
 *
 *  Backend ověřuje sezení SÁM, u každého volání kromě login. Tady jsou
 *  data a tady je i hlavní bezpečnost — na to API se může napojit
 *  cokoli, třeba mobilní aplikace, a nic z toho nesmí spoléhat na to,
 *  že před ním stojí náš BFF.
 *
 *  Úložiště je vyměnitelné: Redis, když je k dispozici, jinak tabulka
 *  sessions. Volající to nepozná. Redis si klíče maže sám, u MySQL to
 *  dělá cronjobs/cleaner.php.
 *
 *  Blok, který se o uživateli vrací, skládá JEDNA funkce (session_block)
 *  a používá ji login i session_check. Kdyby ho skládala dvě místa,
 *  rozejdou se — a BFF pak po obnovení cache dostane něco jiného, než
 *  dostal při přihlášení.
 * ===================================================================== */

/* Klíč sezení. 32 bajtů z random_bytes, tedy 64 hex znaků. */
function session_new_key(): string { return bin2hex(random_bytes(32)); }

/* --------------------------------------------------------------------- */

function session_store_put(string $key, array $data): void {
    $json = json_encode($data, JSON_UNESCAPED_UNICODE);
    if ($r = redis()) {
        $r->setex(REDIS_PREFIX . 'ses.' . $key, (int)SESSION_TTL, $json);
        return;
    }
    q("INSERT INTO sessions (se_key, se_us_id, se_data) VALUES (?,?,?)
       ON DUPLICATE KEY UPDATE se_data=VALUES(se_data), se_touched=NOW()",
      [$key, (int)$data['us_id'], $json]);
}

function session_store_get(string $key): ?array {
    if ($r = redis()) {
        $j = $r->get(REDIS_PREFIX . 'ses.' . $key);
        if ($j === false) return null;
        /* Prodloužení při každém použití. Životnost se tak měří od
           poslední aktivity, ne od přihlášení. */
        $r->expire(REDIS_PREFIX . 'ses.' . $key, (int)SESSION_TTL);
        $d = json_decode((string)$j, true);
        return is_array($d) ? $d : null;
    }
    $row = q_one("SELECT se_data FROM sessions
                  WHERE se_key=? AND se_touched > NOW() - INTERVAL ? SECOND", [$key, (int)SESSION_TTL]);
    if (!$row) return null;
    q("UPDATE sessions SET se_touched=NOW() WHERE se_key=?", [$key]);
    $d = json_decode((string)$row['se_data'], true);
    return is_array($d) ? $d : null;
}

function session_store_del(string $key): void {
    if ($r = redis()) { $r->del(REDIS_PREFIX . 'ses.' . $key); return; }
    q("DELETE FROM sessions WHERE se_key=?", [$key]);
}

/* ---------------------------------------------------------------------
 *  Blok o uživateli. JEDINÉ místo, kde vzniká.
 *
 *  Drží se tu hygiena: JEN to, co BFF potřebuje k řízení aplikace.
 *  Oprávnění, id, jméno. Nikoli počet výpůjček, ne e-mail, ne poznámka
 *  — to už jsou data a ta se berou dotazem. Jakmile se sem jednou začne
 *  přidávat „a hodilo by se taky…", nikdo to nezastaví.
 * ------------------------------------------------------------------- */
function session_block(array $u): array {
    return [
        'us_id'   => (int)$u['us_id'],
        'us_name' => (string)$u['us_name'],
        'us_acl'  => (string)$u['us_acl'],
    ];
}

/* ---------------------------------------------------------------------
 *  Ověření. Vrací blok, nebo skončí 401.
 * ------------------------------------------------------------------- */
function session_require(): array {
    $key = in_header('X-App-Session');
    if ($key === '' || !ctype_xdigit($key) || strlen($key) !== 64)
        api_error(401, 'Nejste přihlášen');

    $d = session_store_get($key);
    if ($d === null) api_error(401, 'Sezení vypršelo, přihlaste se prosím znovu');

    $d['session'] = $key;
    return $d;
}

/* Oprávnění. Seznam v us_acl je oddělený čárkami — stejně jako
   v databázi, stejně jako v Redisu. Jedna reprezentace všude. */
function acl_has(array $me, string $priv): bool {
    return in_array($priv, explode(',', (string)($me['us_acl'] ?? '')), true);
}

function acl_require(array $me, string $priv): void {
    if (!acl_has($me, $priv)) api_error(403, 'Na tuhle operaci nemáte oprávnění');
}
