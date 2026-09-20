<?php
/* auth.inc — mock přihlášení pro demo. Nastavení je v config.inc.
   Session je podepsaný token, takže není potřeba žádné úložiště.
   V reálné aplikaci sem přijde databáze, Redis, cokoli — framework
   o tom nic neví. */

function auth_users(): array {
    return [
        'tomas' => ['name' => 'Tomáš Šimek', 'pass' => 'heslo', 'acl' => ['save_form', 'admin']],
        'host'  => ['name' => 'Host',        'pass' => 'host',  'acl' => []],
    ];
}

function b64u(string $s): string  { return rtrim(strtr(base64_encode($s), '+/', '-_'), '='); }
function b64ud(string $s): string { return (string)base64_decode(strtr($s, '-_', '+/')); }

function check_password(string $login, string $pass): ?array {
    $u = auth_users()[$login] ?? null;
    if ($u === null) return null;
    if (!hash_equals($u['pass'], $pass)) return null;
    return ['login' => $login, 'name' => $u['name'], 'acl' => $u['acl']];
}

/* Token je svázaný se sériovým číslem instalace — odcizený token
   na jiném zařízení neprojde. Přesně na tohle je X-App-Serial. */
function make_session(array $u, string $serial): string {
    $p = b64u(json_encode(['u' => $u['login'], 's' => $serial, 'exp' => time() + AUTH_TTL],
                          JSON_UNESCAPED_UNICODE));
    return $p . '.' . b64u(hash_hmac('sha256', $p, AUTH_SECRET, true));
}

function session_user(string $token): ?array {
    if ($token === '') return null;
    $dot = strrpos($token, '.');
    if ($dot === false) return null;
    $p   = substr($token, 0, $dot);
    $sig = substr($token, $dot + 1);
    if (!hash_equals(b64u(hash_hmac('sha256', $p, AUTH_SECRET, true)), $sig)) return null;

    $d = json_decode(b64ud($p), true);
    if (!is_array($d) || ($d['exp'] ?? 0) < time())             return null;
    if (!hash_equals((string)($d['s'] ?? ''), req_header('X-App-Serial'))) return null;

    $u = auth_users()[$d['u']] ?? null;
    if ($u === null) return null;
    return ['login' => $d['u'], 'name' => $u['name'], 'acl' => $u['acl']];
}

function acl_check(?array $user, string $priv): bool {
    return $user !== null && in_array($priv, $user['acl'], true);
}
