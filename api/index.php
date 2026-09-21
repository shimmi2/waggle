<?php
/* =====================================================================
 *  api/index.php — demo API
 *
 *  Jeden dispatcher, dvě sady fragmentů:
 *      pages/plain/   pro /app   (holé HTML)
 *      pages/lte/     pro /app2  (AdminLTE)
 *
 *  Skin posílá aplikace v každém požadavku přes Fw.init({params:{skin:…}}).
 *  Endpointy, session i protokol jsou pro obě naprosto totožné — liší se
 *  jen markup, což je přesně ta hranice, kde má být šablona.
 *
 *  Konvence endpointu, v tomhle pořadí:
 *    1. import vstupů   2. sémantika   3. session (401)
 *    4. oprávnění (403) 5. práce       6. send_answer()
 *
 *  Odpověď je atomická: dokud skript nedoběhne, neodešel ani bajt,
 *  takže do posledního okamžiku lze vrátit poctivý HTTP status.
 * ===================================================================== */

require __DIR__ . '/config.inc';       // musí být první
require __DIR__ . '/../fw.inc';
require __DIR__ . '/inc/app.inc';
require __DIR__ . '/inc/auth.inc';
require __DIR__ . '/inc/stream.inc';
fw_boot();

$skin     = req('skin', 8) === 'lte' ? 'lte' : 'plain';
$PAGES    = __DIR__ . '/pages/' . $skin;

$function = req('function', 64);
$session  = req_header('X-App-Session');
$serial   = req_header('X-App-Serial');
$user     = session_user($session);

/* Kompletní obrazovka pro daný stav přihlášení. */
function screen(string $PAGES, ?array $user, string $serial, string $body, array $vars = []): array {
    $menu = $user === null ? 'menu_guest.inc' : 'menu_user.inc';
    return [
        ['op' => 'html', 'sel' => 'left_menu', 'content' => render("$PAGES/$menu",     ['user' => $user])],
        ['op' => 'html', 'sel' => 'top_frame', 'content' => render("$PAGES/top_bar.inc", ['user' => $user])],
        ['op' => 'html', 'sel' => 'main',
         'content' => render("$PAGES/$body", $vars + ['user' => $user, 'serial' => $serial])],
    ];
}

/* Společný začátek: vydat token a ověřit, že broker opravdu běží.
   Dokud neodešel první bajt, můžeme vrátit poctivý HTTP status. */
function busy_kanal(?array $user, string $session): string {
    if ($user === null) throw_http_error(401, 'Nejste přihlášen');
    $tok = stream_token($session, 'main');
    if ($tok === '') throw_http_error(500, 'Nepodařilo se vydat token kanálu');
    if (!fw_publish(STREAM_PUB_URL, $tok, [['op' => 'debug', 'message' => 'busy: kanál živý']]))
        throw_http_error(503, 'nchan neběží na ' . STREAM_PUB_URL
                            . ' — viz nginx-nchan.conf.example');
    return $tok;
}

/* ---------------------------------------------------------------------
 *  Zkratka page_* — přímé servírování stránek přes output buffer.
 *  Cesta je bezpečná z konstrukce: is_word() nepustí '/', '.' ani NUL.
 * ------------------------------------------------------------------- */
if (strncmp($function, 'page_', 5) === 0) {
    if (!is_word($function)) throw_http_error(400, 'Neplatný název stránky');

    start_direct_answer(['sel' => 'main']);
    if     (is_file("$PAGES/$function.html")) readfile("$PAGES/$function.html");
    elseif (is_file("$PAGES/$function.inc"))  require  "$PAGES/$function.inc";
    else { finish_direct_answer(); throw_http_error(404, "Stránka $function neexistuje"); }
    finish_direct_answer();

    send_answer(['op' => 'history', 'url' => "#?function=$function"]);
    finish_answer();
    exit;
}

switch ($function) {

/* ------------------------------------------------------------------ */
case 'index':
default:
    send_answer(screen($PAGES, $user, $serial,
                       $user === null ? 'welcome.inc' : 'dashboard.inc'));
    if ($user !== null) send_answer(stream_subscribe_cmd($session, 'main'));
    send_answer(['op' => 'history', 'url' => '#?function=index']);
    break;

/* ------------------------------------------------------------------ */
case 'do_login':
    $login = req('user', 64);
    $pass  = req('password', 256);
    if ($login === '' || $pass === '') throw_http_error(400, 'Vyplňte jméno i heslo');

    $u = check_password($login, $pass);
    if ($u === null) {
        /* Neúspěch není chyba protokolu, ale normální stav aplikace. */
        send_answer(['op' => 'html', 'sel' => 'main',
                     'content' => render("$PAGES/login_failed.inc", ['login' => $login])]);
        break;
    }

    $new = make_session($u, $serial);
    send_answer(array_merge(
        [['op' => 'session', 'value' => $new]],          // musí být PRVNÍ
        screen($PAGES, $u, $serial, 'dashboard.inc'),
        [stream_subscribe_cmd($new, 'main')]
    ));
    break;

/* ------------------------------------------------------------------ */
case 'do_logout':
    send_answer(array_merge(
        [['op' => 'unsubscribe']],                       // bez name = všechny kanály
        [['op' => 'session', 'value' => null]],
        screen($PAGES, null, $serial, 'welcome.inc')
    ));
    break;

/* ------------------------------------------------------------------ */
case 'form_test':
case 'sync':
case 'progress':
case 'busy':
    send_answer([
        ['op' => 'html', 'sel' => 'main', 'content' => render("$PAGES/$function.inc", ['user' => $user])],
        ['op' => 'history', 'url' => "#?function=$function"],
    ]);
    break;

/* ------------------------------------------------------------------ */
case 'save_form_test':
    $nazev = req('nazev', 128);
    $pocet = req_int('pocet');
    $pozn  = req('poznamka', 1024);

    if ($nazev === '')                  throw_http_error(400, 'Název je povinný');
    if ($pocet < 1 || $pocet > 99)      throw_http_error(400, 'Počet musí být 1 až 99');
    if ($user === null)                 throw_http_error(401, 'Nejste přihlášen');
    if (!acl_check($user, 'save_form')) throw_http_error(403, 'Nemáte oprávnění');

    send_answer(['op' => 'html', 'sel' => 'main',
                 'content' => render("$PAGES/form_done.inc",
                     ['nazev' => $nazev, 'pocet' => $pocet, 'poznamka' => $pozn, 'user' => $user])]);
    break;

/* ------------------------------------------------------------------ *
 *  Zaneprázdněno: jedna operace, čtyři chování.
 *
 *  Všechny čtyři doručují pushem, a to je tu ta pointa. fw_publish()
 *  je krátký POST na nchan na localhostu — Apache v té cestě není,
 *  takže ho jeho buffering nedrží. PHP si klidně drží request a mezi
 *  kroky publikuje; není potřeba ani fw_stream_start(), ani worker.
 * ------------------------------------------------------------------ */

case 'busy_quick':                       // doběhne dřív, než se cokoli nakreslí
    $tok = busy_kanal($user, $session);
    fw_publish(STREAM_PUB_URL, $tok, [fw_busy('#busy_zone', 'Tohle nikdo neuvidí…')]);
    usleep(150000);                      // 150 ms < prodleva 300 ms
    send_answer(['op' => 'html', 'sel' => '#busy_out',
                 'content' => 'Hotovo za 150 ms — překryv se ani nenakreslil.']);
    break;

case 'busy_phases':                      // tři obyčejná volání za sebou
    $tok = busy_kanal($user, $session);
    foreach (['Připravuji…', 'Počítám…', 'Dokončuji…'] as $faze) {
        fw_publish(STREAM_PUB_URL, $tok, [fw_busy('#busy_zone', $faze)]);
        usleep(900000);
    }
    fw_publish(STREAM_PUB_URL, $tok, [fw_busy_done('#busy_zone', 'Hotovo')]);
    send_answer(['op' => 'html', 'sel' => '#busy_out',
                 'content' => 'Tři fáze, jeden překryv, žádný stav na klientovi.']);
    break;

case 'busy_bar':                         // kolečko, které se promění v pruh
    $tok = busy_kanal($user, $session);
    /* Zatím nevíme, kolik toho bude — tedy bez pct, tedy kolečko. */
    fw_publish(STREAM_PUB_URL, $tok, [fw_busy('#busy_zone', 'Prohledávám…')]);
    usleep(1200000);
    /* Teď to víme. Stačí poslat pct a z kolečka je pruh. */
    for ($i = 0; $i <= 100; $i += 4) {
        fw_publish(STREAM_PUB_URL, $tok, [fw_busy('#busy_zone', "Zpracovávám 240 položek…", $i)]);
        usleep(120000);
    }
    fw_publish(STREAM_PUB_URL, $tok, [fw_busy_done('#busy_zone', 'Zpracováno 240 položek')]);
    send_answer(['op' => 'html', 'sel' => '#busy_out',
                 'content' => 'Kolečko se změnilo v pruh ve chvíli, kdy dorazilo první pct.']);
    break;

case 'busy_clear':                       // překreslení okna překryv sundá
    $tok = busy_kanal($user, $session);
    fw_publish(STREAM_PUB_URL, $tok, [fw_busy('#busy_zone', 'Tenhle překryv nikdo nevypne…')]);
    usleep(1800000);
    /* Nikdo neposílá busy state=off. Překryv zmizí proto, že do jeho
       okna přišlo html — hlídá to Fw.apply v dispatcheru. */
    send_answer(['op' => 'html', 'sel' => '#busy_zone', 'content' =>
        '<div style="padding:24px;text-align:center">Okno bylo překresleno.'
      . '<div id="busy_out"><small>Překryv zmizel sám, bez state=off.</small></div></div>']);
    break;

/* ------------------------------------------------------------------ *
 *  Progress: dvakrát totéž, dvěma technologiemi.
 * ------------------------------------------------------------------ */
case 'progress_run':                     // stream: PHP drží spojení 10 s
    if ($user === null) throw_http_error(401, 'Nejste přihlášen');
    set_time_limit(60);
    fw_stream_start();                   // prorazí buffer Apache

    send_answer([
        ['op' => 'class', 'sel' => '#run_btn', 'add' => ['disabled']],
        ['op' => 'html',  'sel' => '#pb_note', 'content' => 'běží streamem…'],
    ]);
    flush_answer();

    for ($i = 0; $i <= 100; $i++) {
        /* Měníme JEN prvky uvnitř karty. Překreslení celého #main by
           zabilo CSS přechod a reinicializovalo šablonu 101x. */
        send_answer(progress_cmds($i));
        flush_answer();
        if (connection_aborted()) exit;  // uživatel odešel -> nedrž worker
        usleep(100000);
    }
    send_answer(progress_done('#run_btn', 'streamem'));
    break;

case 'progress_push':                    // push: PHP odpoví za ~20 ms
    if ($user === null) throw_http_error(401, 'Nejste přihlášen');

    $tok = stream_token($session, 'main');
    if ($tok === '') throw_http_error(500, 'Nepodařilo se vydat token kanálu');

    /* Zkušební publikace ověří, že broker běží — dokud neodešel první
       bajt, můžeme ještě vrátit poctivý HTTP status. */
    $ok = fw_publish(STREAM_PUB_URL, $tok, [
        ['op' => 'class', 'sel' => '#run_push', 'add' => ['disabled']],
        ['op' => 'html',  'sel' => '#pb_note', 'content' => 'běží na pozadí, doručuje se pushem…'],
    ]);
    if (!$ok) throw_http_error(503,
        'nchan neběží na ' . STREAM_PUB_URL . ' — viz nginx-nchan.conf.example');

    stream_spawn(__DIR__ . '/bin/worker_progress.php', ['token' => $tok]);
    send_answer(['op' => 'debug', 'message' => 'worker spuštěn, procenta dorazí kanálem "main"']);
    break;

/* ------------------------------------------------------------------ */
case 'ping':                             // ukázka jemného adresování
    if ($user === null) throw_http_error(401, 'Nejste přihlášen');
    send_answer([
        ['op' => 'html',  'sel' => '#ping_out', 'content' => date('H:i:s')],
        ['op' => 'class', 'sel' => '#ping_card', 'add' => ['border-success']],
        ['op' => 'attr',  'sel' => '#ping_btn', 'name' => 'title', 'value' => 'naposled ' . date('H:i:s')],
    ]);
    break;

case 'boom':                             // ukázka fatální chyby
    if ($user === null) throw_http_error(401, 'Nejste přihlášen');
    neexistujici_funkce();
    break;
}

finish_answer();
