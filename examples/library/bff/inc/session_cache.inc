<?php
/* =====================================================================
 *  session_cache.inc — krátká paměť BFF na to, kdo je přihlášen
 *
 *  BFF potřebuje vědět tři věci, aby mohl řídit aplikaci: id, jméno
 *  a oprávnění. Dostane je při přihlášení a pak si je na minutu
 *  pamatuje, aby se backendu neptal při každém kliknutí.
 *
 *  Je to VĚDOMÝ kompromis a stojí za to vědět, co kupuje a co platí:
 *
 *    kupuje    o jedno HTTP volání na klik méně, tedy 43 ms
 *    platí     když se uživateli změní oprávnění, může do minuty
 *              vidět v menu položku, na kterou už nemá právo
 *
 *  To druhé se vyléčí samo: klikne, backend vrátí 403, dostane hlášku,
 *  a jak cache vyprší, menu se překreslí správně. Data ohrožená nejsou
 *  NIKDY — backend si oprávnění ověřuje sám u každého volání a o téhle
 *  cache nic neví.
 *
 *  Adresář PATŘÍ MIMO DOCROOT. Leží v něm id, jméno a oprávnění
 *  přihlášených; kdo si ho dá do webu, servíruje to cizím lidem.
 *  Jméno souboru je hash tokenu, ne token sám — z výpisu adresáře ani
 *  ze zálohy tak nejde žádné sezení přehrát.
 * ===================================================================== */

function cache_path(string $session): string {
    return rtrim(BFF_CACHE_DIR, '/') . '/' . hash('sha256', $session) . '.json';
}

function cache_dir_ready(): bool {
    $d = rtrim(BFF_CACHE_DIR, '/');
    if (is_dir($d)) return true;
    /* 0700: do adresáře nemá co koukat ani jiný uživatel na stroji. */
    if (@mkdir($d, 0700, true)) return true;
    error_log('[library-bff] cache: nejde vyrobit ' . $d . ', jedu bez ní');
    return false;
}

function cache_get(string $session): ?array {
    if ($session === '' || !cache_dir_ready()) return null;
    $f = cache_path($session);
    $s = @stat($f);
    if (!$s) return null;
    /* Čas VZNIKU, ne posledního přístupu. Kdyby se počítal přístup,
       aktivní uživatel by starý blok nosil libovolně dlouho — a právě
       u aktivního uživatele je čerstvost oprávnění potřeba nejvíc. */
    if (time() - (int)$s['mtime'] > (int)BFF_CACHE_TTL) { @unlink($f); return null; }
    $d = json_decode((string)@file_get_contents($f), true);
    return is_array($d) ? $d : null;
}

function cache_put(string $session, array $block): void {
    if ($session === '' || !cache_dir_ready()) return;
    $f = cache_path($session);
    /* Zápis přes dočasný soubor a rename. Bez toho může jiný požadavek
        přečíst rozepsaný JSON a dostat prázdná oprávnění. */
    $t = $f . '.' . getmypid() . '.tmp';
    if (@file_put_contents($t, json_encode($block, JSON_UNESCAPED_UNICODE)) === false) return;
    @chmod($t, 0600);
    @rename($t, $f);
}

function cache_forget(string $session): void {
    if ($session === '') return;
    @unlink(cache_path($session));
}

/* ---------------------------------------------------------------------
 *  Kdo se mě ptá. Vrací blok, nebo null, když nikdo.
 *
 *  Při minutí cache se jednou zavolá session_check — tedy TÝŽ blok,
 *  jaký vrací login, protože ho na backendu skládá jedna funkce.
 * ------------------------------------------------------------------- */
/* $osvezit vynutí dotaz na backend a cache přeskočí.
 *
 *  Čtecí obrazovky cache věří: když je blok o minutu starý, nakreslí se
 *  menu o položku bohatší, než na jakou má člověk právo, a backend ho
 *  stejně odmítne. Zápisový DIALOG je jiný případ — otevřít ho nad
 *  mrtvým sezením znamená, že člověk vyplní formulář a o práci přijde
 *  až při ukládání. Jedno volání navíc je proti tomu levné. */
function me(bool $osvezit = false): ?array {
    global $SESSION;
    static $me = false;
    if ($me !== false && !$osvezit) return $me;
    $me = null;

    if ($SESSION === '') return $me;

    if (!$osvezit && ($c = cache_get($SESSION))) { $me = $c; return $me; }

    [$code, $data] = api_raw('session_check', [], $SESSION);
    if ($code !== 200 || !isset($data['me'])) return $me;      // 401 = nikdo

    $me = $data['me'];
    cache_put($SESSION, $me);
    return $me;
}

/* Oprávnění. Používá se JEN k rozhodnutí, co nakreslit — o tom, co je
   povolené, rozhoduje backend. Proto je neškodné, že to stojí na
   minutu staré informaci. */
function may(string $priv): bool {
    $m = me();
    return $m !== null && in_array($priv, explode(',', (string)($m['us_acl'] ?? '')), true);
}
