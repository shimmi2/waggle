<?php
/* =====================================================================
 *  api_client.inc — volání backendového API
 *
 *  BFF nesahá na databázi ani na Redis. Všechno si bere odsud, a proto
 *  je to jediné místo, kde se řeší síť.
 *
 *  Dvě věci, na kterých tady stojí použitelnost celé aplikace:
 *
 *  1. KEEP-ALIVE. Jedno spojení se drží po dobu požadavku. Bez toho
 *     platíš TCP i TLS handshake u každého volání; naměřeno 67 ms proti
 *     43 ms, tedy o třetinu víc. A protože jedna obrazovka volá backend
 *     klidně pětkrát, je to znát.
 *
 *  2. ROZLIŠENÍ „backend řekl ne" OD „backend neodpověděl". Tohle je
 *     nejčastější chyba třívrstvých systémů. Když se to slije, ladíš
 *     výpadek API podle hlášky „kniha nenalezena" a hledáš hodinu
 *     v úplně jiné vrstvě. 4xx z backendu se předává uživateli, spadlé
 *     spojení je 502 s vlastní hláškou.
 * ===================================================================== */

/* Držené spojení — ale JEN po dobu jednoho požadavku.
 *
 *  Zisk je v tom, že jedna obrazovka volá backend víckrát: statistika
 *  pětkrát. Na tom se ušetří TCP i TLS handshake.
 *
 *  Na konci požadavku se handle ZAVÍRÁ, a to schválně. Nečinné spojení
 *  držené mezi požadavky zablokuje jednovláknový server — a `php -S`,
 *  na kterém si kickstart nejspíš každý zkusí lokálně, jednovláknový
 *  je. Bez toho zavření běží druhé volání do patnáctisekundového
 *  timeoutu a vypadá to jako spadlý backend. (Naměřeno, ne odhadnuto.)
 *  Přes požadavky se spojení stejně spolehlivě držet nedá, protože
 *  PHP-FPM procesy recykluje.
 */
function api_handle() {
    static $ch = null;
    if ($ch !== null) return $ch;
    $ch = curl_init();
    register_shutdown_function(function () use (&$ch) {
        if (is_resource($ch) || $ch instanceof CurlHandle) curl_close($ch);
    });
    curl_setopt_array($ch, [
        CURLOPT_RETURNTRANSFER => true,
        CURLOPT_POST           => true,
        CURLOPT_TIMEOUT        => 15,
        CURLOPT_CONNECTTIMEOUT => 5,
        CURLOPT_FOLLOWLOCATION => false,   // API nemá kam přesměrovávat
    ]);
    return $ch;
}

/* Zavolá endpoint. Vrací [http_kód, data]. Kód 0 znamená, že se
   nepodařilo ani spojit — tedy NE chybu aplikace, ale výpadek. */
function api_raw(string $fn, array $args = [], string $session = ''): array {
    $ch = api_handle();
    curl_setopt($ch, CURLOPT_URL, rtrim(API_URL, '/') . '/?fn=' . urlencode($fn));
    curl_setopt($ch, CURLOPT_POSTFIELDS, http_build_query($args));
    curl_setopt($ch, CURLOPT_HTTPHEADER,
        $session !== '' ? ['X-App-Session: ' . $session, 'Expect:'] : ['Expect:']);

    $body = curl_exec($ch);
    if ($body === false) {
        error_log('[library-bff] api ' . $fn . ': ' . curl_error($ch));
        return [0, ['error' => 'Služba není dostupná, zkuste to prosím za chvíli']];
    }
    $code = (int)curl_getinfo($ch, CURLINFO_HTTP_CODE);
    $data = json_decode((string)$body, true);
    if (!is_array($data)) {
        error_log('[library-bff] api ' . $fn . ': odpověď není JSON: ' . substr((string)$body, 0, 300));
        return [502, ['error' => 'Backend odpověděl nesmyslem']];
    }
    /* 5xx z backendu je taky výpadek, ne chyba uživatele. Jeho text
       uživateli neukazujeme — může v něm být cesta na disk. */
    if ($code >= 500) {
        error_log('[library-bff] api ' . $fn . " -> $code: " . ($data['error'] ?? ''));
        return [$code, ['error' => 'Služba má potíže, zkuste to prosím za chvíli']];
    }
    return [$code, $data];
}

/* ---------------------------------------------------------------------
 *  Pohodlnější obal: chybu vyřeší sám.
 *
 *  401 je jediná chyba, kterou musí zpracovat KLIENT, a ne my — musí
 *  zahodit sezení a vrátit se na přihlášení. Posíláme proto příkaz
 *  error s kódem 401 a app.js si na něj v registru operací sáhne.
 * ------------------------------------------------------------------- */
function api_call(string $fn, array $args = []): array {
    global $SESSION;
    [$code, $data] = api_raw($fn, $args, $SESSION);
    if ($code === 200) return $data;

    if ($code === 401) {
        cache_forget($SESSION);
        send_answer([['op' => 'session', 'value' => null],
                     ['op' => 'error', 'code' => 401,
                      'message' => $data['error'] ?? 'Byl jste odhlášen']]);
        finish_answer();
        exit;
    }

    /* Ostatní chyby jsou hláška uživateli. Obrazovka zůstane, jak byla —
       uživatel po 403 nemá přijít o to, co má rozepsané. */
    send_answer([['op' => 'error', 'code' => $code ?: 502,
                  'message' => $data['error'] ?? 'Neznámá chyba']]);
    finish_answer();
    exit;
}
