<?php
/* =====================================================================
 *  api_users.inc — přihlášení, sezení, správa uživatelů
 *
 *  Každá funkce v tomhle souboru JE endpoint a drží bezpečnostní
 *  šablonu v tomhle pořadí:
 *
 *    1. vstupy      2. sémantika      3. sezení
 *    4. oprávnění   5. práce          6. odpověď
 *
 *  Sezení před oprávněním, protože bez identity není co ověřovat.
 *  Sémantika před sezením, protože je levná a nepřihlášený se tak
 *  nedostane k ničemu drahému.
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  do_login — jediný endpoint, který sezení NEvyžaduje
 * ------------------------------------------------------------------- */
function ep_do_login(): void {
    /* 1. vstupy */
    $login = in_str('login', 64);
    $pass  = in_str('password', 256);

    /* 2. sémantika */
    if ($login === '' || $pass === '') api_error(400, 'Vyplňte jméno i heslo');

    /* 3. škrtič — PŘED hledáním uživatele, ne až před ověřením hesla.
          Za limitem odpovídáme za desetinu milisekundy a útok se
          nedotkne ani databáze, ani bcryptu. */
    throttle_check($login);

    /* 4. práce */
    $u = q_one("SELECT us_id, us_name, us_acl, us_password, us_pwcrypttype
                FROM users WHERE us_login=? LIMIT 1", [$login]);

    /* Neexistující uživatel se ověřuje proti atrapě hashe. Bez toho
       odpovíme o 150 ms dřív a tím prozradíme, která konta existují.

       MUSÍ to být PLATNÝ bcrypt hash se STEJNOU cenou, jakou používá
       PASSWORD_DEFAULT (dnes $2y$12$). Neplatný řetězec password_verify()
       odmítne za desetinu milisekundy a obrana je pak k ničemu — ověřeno
       měřením, ne odhadem. Ta atrapa není tajná, je to návnada; heslo
       k ní neexistuje.

       Kdyby PHP jednou zvedlo výchozí cenu, je potřeba ji tu zvednout
       taky, jinak se časy zas rozejdou. */
    static $atrapa = '$2y$12$WP.jo0WHflQeUf5jeBKx/ueWSUzKnqneMLd1alpmK9eufsfuwX4Pa';
    $ok = false;
    if ($u === null) {
        password_verify($pass, $atrapa);
    } elseif ($u['us_pwcrypttype'] === 'plain') {
        /* Jen pro převod starých databází. Po prvním úspěšném
           přihlášení se heslo přehashuje a 'plain' z účtu zmizí. */
        $ok = ($u['us_password'] !== '' && hash_equals((string)$u['us_password'], $pass));
        if ($ok) q("UPDATE users SET us_password=?, us_pwcrypttype='bcrypt' WHERE us_id=?",
                   [password_hash($pass, PASSWORD_DEFAULT), (int)$u['us_id']]);
    } else {
        $ok = password_verify($pass, (string)$u['us_password']);
    }

    if (!$ok) {
        throttle_fail($login);
        /* Stejná hláška pro špatné heslo i neexistující konto. */
        api_error(401, 'Neplatné jméno nebo heslo');
    }

    throttle_ok($login);

    /* Sezení se zakládá tady, ale BLOK skládá session_block() — tedy
       to samé místo, které ho vrací i při session_check. Proto se
       nemůžou rozejít. */
    $key   = session_new_key();
    $block = session_block($u);
    session_store_put($key, $block);

    /* 5. odpověď */
    api_out(['session' => $key, 'me' => $block]);
}

/* ---------------------------------------------------------------------
 *  session_check — pro BFF, když mu vyprší jeho krátká cache
 *
 *  Vrací TÝŽ blok jako login. BFF si ho jen uloží a zas na minutu
 *  přestane volat.
 * ------------------------------------------------------------------- */
function ep_session_check(): void {
    $me = session_require();
    api_out(['me' => ['us_id'   => $me['us_id'],
                      'us_name' => $me['us_name'],
                      'us_acl'  => $me['us_acl']]]);
}

function ep_do_logout(): void {
    $me = session_require();
    session_store_del($me['session']);
    api_out(['ok' => true]);
}

/* ---------------------------------------------------------------------
 *  Správa uživatelů — admin
 * ------------------------------------------------------------------- */
function ep_users_list(): void {
    $me = session_require();
    acl_require($me, 'manage_users');

    $hledat = in_str('q', 64);
    $limit  = max(1, min(500, in_int('limit') ?: 100));

    $where = ''; $p = [];
    if ($hledat !== '') {
        $where = "WHERE us_name LIKE ? OR us_login LIKE ? OR us_email LIKE ?";
        $p = array_fill(0, 3, '%' . $hledat . '%');
    }
    $rows = q_all("SELECT us_id, us_login, us_name, us_phone, us_email, us_acl, us_note, us_created
                   FROM users $where ORDER BY us_name LIMIT $limit", $p);

    api_out(['users' => $rows, 'acls' => q_all("SELECT ac_key, ac_name, ac_description FROM acls ORDER BY ac_sort")]);
}

/* Čtenáři pro výběr do filtru výpůjček.
 *
 *  Schválně NE ep_users_list(): ten je za manage_users a vrací telefon,
 *  e-mail, poznámku i oprávnění. Kdo spravuje výpůjčky, potřebuje jméno
 *  a id — a nic víc mu do ruky nepatří. Menší právo, menší odpověď.
 *
 *  Stejný endpoint umí i vyhledat jednoho podle id. Díky tomu si BFF
 *  dokáže doplnit jméno k id, které přišlo v odkazu, a nemusí věřit
 *  jménu od klienta. */
function ep_readers_list(): void {
    /* 1. vstupy */
    $id    = in_int('us_id');
    $q     = in_str('q', 64);
    $limit = max(1, min(100, in_int('limit') ?: 30));

    /* 2. sémantika — prázdný filtr znamená „prvních pár", to je v pořádku */

    /* 3. sezení, 4. oprávnění */
    $me = session_require();
    acl_require($me, 'see_all_rentals');

    /* 5. práce */
    if ($id > 0) {
        $rows = q_all("SELECT us_id, us_name FROM users WHERE us_id=?", [$id]);
    } elseif ($q !== '') {
        $rows = q_all("SELECT us_id, us_name FROM users
                       WHERE us_name LIKE ? OR us_login LIKE ?
                       ORDER BY us_name LIMIT $limit", ['%' . $q . '%', '%' . $q . '%']);
    } else {
        $rows = q_all("SELECT us_id, us_name FROM users ORDER BY us_name LIMIT $limit");
    }

    /* 6. odpověď */
    api_out(['readers' => $rows, 'total' => count($rows)]);
}

function ep_user_save(): void {
    /* 1. vstupy */
    $op     = in_str('op', 16);
    $id     = in_int('us_id');
    $login  = in_str('us_login', 64);
    $name   = in_str('us_name', 128);
    $phone  = in_str('us_phone', 32);
    $email  = in_str('us_email', 128);
    $note   = in_str('us_note', 2000);
    $pass   = in_str('us_password', 256);
    $acl    = in_str('us_acl', 255);

    /* 2. sémantika
       Záměr říká klient výslovně a server ověří, že si to neodporuje s
       daty. Odvozovat ho z přítomnosti id znamená, že ztracené us_id
       tiše založí druhé konto — a odpoví ok. Viz ep_book_save(). */
    if ($op !== 'insert' && $op !== 'update')
                       api_error(400, 'Neznámá operace', "op='$op'");
    if ($op === 'update' && $id < 1)   api_error(400, 'Nevím, které konto mám změnit');
    if ($op === 'insert' && $id !== 0) api_error(400, 'Nové konto nemůže mít id', "insert s us_id=$id");
    if ($login === '') api_error(400, 'Přihlašovací jméno je povinné');
    if ($name === '')  api_error(400, 'Jméno je povinné');
    if ($email !== '' && !filter_var($email, FILTER_VALIDATE_EMAIL))
        api_error(400, 'E-mail nemá platný formát');

    /* 3. sezení, 4. oprávnění */
    $me = session_require();
    acl_require($me, 'manage_users');

    /* Oprávnění se validují proti číselníku. Kdyby se ukládalo, co
       přišlo, může si kdokoli s manage_users vymyslet právo, které
       v aplikaci nikdy nevznikne — a hledat to pak nikdo nebude. */
    $znama = array_column(q_all("SELECT ac_key FROM acls"), 'ac_key');
    $chtena = array_values(array_intersect(array_filter(array_map('trim', explode(',', $acl))), $znama));
    $acl = implode(',', $chtena);

    /* 5. práce */
    if ($op === 'update') {
        if (!q_val("SELECT us_id FROM users WHERE us_id=?", [$id])) api_error(404, 'Takové konto tu není');
        $jiny = q_val("SELECT us_id FROM users WHERE us_login=? AND us_id<>?", [$login, $id]);
        if ($jiny) api_error(400, 'Přihlašovací jméno už někdo má');
        q("UPDATE users SET us_login=?, us_name=?, us_phone=?, us_email=?, us_note=?, us_acl=?
           WHERE us_id=? LIMIT 1", [$login, $name, $phone, $email, $note, $acl, $id]);
        if ($pass !== '')
            q("UPDATE users SET us_password=?, us_pwcrypttype='bcrypt' WHERE us_id=? LIMIT 1",
              [password_hash($pass, PASSWORD_DEFAULT), $id]);
    } else {
        if (q_val("SELECT us_id FROM users WHERE us_login=?", [$login]))
            api_error(400, 'Přihlašovací jméno už někdo má');
        q("INSERT INTO users (us_login, us_password, us_name, us_phone, us_email, us_note, us_acl)
           VALUES (?,?,?,?,?,?,?)",
          [$login, $pass !== '' ? password_hash($pass, PASSWORD_DEFAULT) : '',
           $name, $phone, $email, $note, $acl]);
        $id = (int)db()->lastInsertId();
    }

    api_out(['ok' => true, 'us_id' => $id]);
}
