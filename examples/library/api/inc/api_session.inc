<?php
/* =====================================================================
 *  api_session.inc — sezení
 *
 *  Backend ověřuje sezení SÁM, u každého volání kromě login. Tady jsou
 *  data a tady je i hlavní bezpečnost — na to API se může napojit
 *  cokoli, třeba mobilní aplikace, a nic z toho nesmí spoléhat na to,
 *  že před ním stojí náš BFF.
 *
 *  Úložiště je vyměnitelné: Redis, když je nastavený, jinak tabulka
 *  sessions. Volající to nepozná. Redis si klíče maže sám, u MySQL to
 *  dělá cronjobs/cleaner.php.
 *
 *  Obě větve musí umět TOTÉŽ, včetně zrušení všech sezení jednoho
 *  uživatele. MySQL to má zadarmo ve sloupci se_us_id, Redis k tomu
 *  potřebuje vlastní množinu ses.u.<id>. Kdo sem přidá operaci, přidá
 *  ji do obou větví — větev, která se u vývojáře nepouští, se jinak
 *  tiše rozejde.
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
        $u = REDIS_PREFIX . 'ses.u.' . (int)$data['us_id'];
        $r->setex(REDIS_PREFIX . 'ses.' . $key, (int)SESSION_TTL, $json);
        $r->sAdd($u, $key);
        $r->expire($u, (int)SESSION_TTL);
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
        if (!is_array($d)) return null;
        /* Množina uživatele musí žít aspoň tak dlouho jako jeho sezení,
           jinak by se po hodině činnosti nedalo sezení zrušit. */
        $r->expire(REDIS_PREFIX . 'ses.u.' . (int)$d['us_id'], (int)SESSION_TTL);
        return $d;
    }
    $row = q_one("SELECT se_data FROM sessions
                  WHERE se_key=? AND se_touched > NOW() - INTERVAL ? SECOND", [$key, (int)SESSION_TTL]);
    if (!$row) return null;
    q("UPDATE sessions SET se_touched=NOW() WHERE se_key=?", [$key]);
    $d = json_decode((string)$row['se_data'], true);
    return is_array($d) ? $d : null;
}

function session_store_del(string $key): void {
    /* Klíč v množině ses.u.<id> nechávám: odkazuje na nic, při rušení
       sezení uživatele se smaže naprázdno a s množinou vyprší. */
    if ($r = redis()) { $r->del(REDIS_PREFIX . 'ses.' . $key); return; }
    q("DELETE FROM sessions WHERE se_key=?", [$key]);
}

/* Zruší všechna sezení uživatele kromě $keep. Volá ho každý endpoint,
   který mění oprávnění nebo heslo — blok v sezení drží práva z doby
   přihlášení a bez tohohle by je otevřená sezení měla dál. */
function session_store_kill_user(int $us_id, string $keep = ''): void {
    if ($r = redis()) {
        $u = REDIS_PREFIX . 'ses.u.' . $us_id;
        foreach ((array)$r->sMembers($u) as $k) {
            if ($k === $keep) continue;
            $r->del(REDIS_PREFIX . 'ses.' . $k);
            $r->sRem($u, $k);
        }
        return;
    }
    q("DELETE FROM sessions WHERE se_us_id=? AND se_key<>?", [$us_id, $keep]);
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
