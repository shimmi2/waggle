<?php
/* =====================================================================
 *  fw.inc — serverová část Waggle
 *  https://github.com/shimmi2/waggle
 *  Protokol: NDJSON, jedna dávka {"v":1,"cmds":[...]} na řádek.
 *  Framework NEŘEŠÍ databázi ani escapování pro cílový kontext.
 *  Garantuje pouze hygienu vstupu: skalární string omezené délky bez NUL.
 * ===================================================================== */

/* Výchozí hodnoty. Projekt je může přebít tím, že si je nadefinuje
   DŘÍV, než načte fw.inc — viz api/config.inc. */
/* FW_RELEASE je vydání knihovny, FW_V je verze protokolu na drátě.
   Mění se nezávisle: protokol zůstává 1, dokud se nezmění formát dávky.
   Bez tohohle čísla se u běžícího projektu nedalo zjistit, co v něm je. */
defined('FW_RELEASE')    || define('FW_RELEASE', '1.2.1');
defined('FW_V')          || define('FW_V', 1);
defined('FW_MAXLEN')     || define('FW_MAXLEN', 65536);   // strop délky jednoho pole
defined('FW_DEBUG')      || define('FW_DEBUG', false);    // true = chyby prozradí cesty
defined('FW_STREAM_PAD') || define('FW_STREAM_PAD', 8192); // viz fw_stream_start()

$FW_QUEUE   = [];      // fronta příkazů čekajících na odeslání
$FW_STARTED = false;   // odešly už hlavičky? (rozhoduje o způsobu hlášení chyby)
$FW_DIRECT  = null;    // rozpracovaný start_direct_answer()
$FW_DONE    = false;   // pojistka proti dvojímu doběhnutí

/* ---------------------------------------------------------------------
 *  VSTUP — jediné tři garance. Nic víc, nic míň.
 * ------------------------------------------------------------------- */

function req(string $name, int $max = 4096): string {
    $v = $_REQUEST[$name] ?? '';
    if (!is_string($v))              return '';   // ?x[]=1 nesmí shodit endpoint
    if (isset($v[$max]))             $v = substr($v, 0, $max);
    if (strpos($v, "\0") !== false)  $v = str_replace("\0", '', $v);
    return $v;
}

function req_int(string $name): int     { return intval(req($name, 32));   }
function req_float(string $name): float { return floatval(req($name, 64)); }

/* "slovo" — bezpečné pro názvy funkcí a části cest. Čistý strspn, 41 ns. */
function is_word(string $s): bool {
    static $OK = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-';
    return $s !== '' && strspn($s, $OK) === strlen($s);
}

/* Hlavička požadavku se stejnou hygienou jako $_REQUEST. */
function req_header(string $name, int $max = 1024): string {
    $v = $_SERVER['HTTP_' . strtr(strtoupper($name), '-', '_')] ?? '';
    if (!is_string($v))              return '';
    if (isset($v[$max]))             $v = substr($v, 0, $max);
    if (strpos($v, "\0") !== false)  $v = str_replace("\0", '', $v);
    return $v;
}

/* ---------------------------------------------------------------------
 *  VÝSTUP
 *  send_answer()   jen zařadí do fronty  -> odpověď je atomická
 *  flush_answer()  odešle frontu hned    -> jen pro asynchronní případ
 *  finish_answer() odešle a ukončí
 * ------------------------------------------------------------------- */

function send_answer(array $cmds): void {
    global $FW_QUEUE;
    if (isset($cmds['op'])) $cmds = [$cmds];          // povol i jediný příkaz
    foreach ($cmds as $c) $FW_QUEUE[] = $c;
}

function fw_start_output(): void {
    global $FW_STARTED;
    if ($FW_STARTED) return;
    $FW_STARTED = true;
    header('Content-Type: application/x-ndjson; charset=utf-8');
    header('Cache-Control: no-store');
    header('X-Accel-Buffering: no');                   // ať proxy nezdrží stream
    if (function_exists('apache_setenv')) @apache_setenv('no-gzip', '1');
    @ini_set('zlib.output_compression', '0');
    fw_cors();
}

/* CORS — aby /api mohlo v produkci žít na jiném hostu než /app. */
function fw_cors(): void {
    $o = req_header('Origin');
    if ($o === '') return;
    header('Access-Control-Allow-Origin: ' . $o);
    header('Access-Control-Allow-Credentials: true');
    header('Vary: Origin');
}

function fw_stray(): string {
    return ob_get_level() > 0 ? (string)ob_get_contents() : '';
}

function flush_answer(): void {
    global $FW_QUEUE;
    $stray = fw_stray();
    if (ob_get_level() > 0) ob_clean();
    if ($stray !== '' && FW_DEBUG)
        $FW_QUEUE[] = ['op' => 'debug', 'message' => 'nezachycený výstup PHP', 'data' => $stray];
    if (!$FW_QUEUE) return;
    fw_start_output();
    echo json_encode(['v' => FW_V, 'cmds' => array_values($FW_QUEUE)],
                     JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES), "\n";
    $FW_QUEUE = [];
    fw_pump();
}

/* PHP-FPM má vlastní output_buffering (typicky 4096) POD naším bufferem.
   Pouhé ob_flush() by dávku jen přesypalo do něj a klient by ji uviděl
   až po naplnění 4 kB. Proto protlač všechny úrovně a pak flush(). */
function fw_pump(): void {
    for ($i = ob_get_level(); $i > 0; $i--) @ob_flush();
    @flush();
}

function finish_answer(): void {
    global $FW_DONE;
    if ($FW_DONE) return;
    $FW_DONE = true;
    flush_answer();
}

/* Chyba: před prvním bajtem -> HTTP status, po něm -> příkaz error. */
function throw_http_error(int $code, string $msg): void {
    global $FW_STARTED, $FW_QUEUE, $FW_DONE;
    if (!$FW_STARTED) {
        if (ob_get_level() > 0) ob_clean();
        http_response_code($code);
        $FW_QUEUE = [['op' => 'error', 'code' => $code, 'message' => $msg]];
    } else {
        $FW_QUEUE[] = ['op' => 'error', 'code' => $code, 'message' => $msg];
    }
    $FW_DONE = false;
    finish_answer();
    exit;
}

/* Zahájení streamu. Volat jen z endpointu, který posílá dávky průběžně.

   Apache (mod_proxy_fcgi + core output filter) drží prvních ~8 kB výstupu
   bez ohledu na flush(). U běžné odpovědi to nevadí — doletí celá naráz —
   ale u progress baru by se prvních ~40 % objevilo najednou se zpožděním.
   Výplň buffer naplní a stream se rozjede okamžitě.

   Řádek samých mezer klient v Fw.line() zahodí (trim -> prázdné),
   takže to nestojí ani jeden JSON.parse.

   Čistší, ale globální alternativa: flushpackets=on u proxy_fcgi. */
function fw_stream_start(): void {
    fw_start_output();
    echo str_repeat(' ', FW_STREAM_PAD), "\n";
    fw_pump();
}

/* ---------------------------------------------------------------------
 *  ZANEPRÁZDNĚNO — překryv „pracuji" nad prvkem.
 *
 *  Sám o sobě je to jen stavitel jednoho příkazu. Užitečný je až tehdy,
 *  když se dostane ke klientovi DŘÍV než výsledek — tedy streamem
 *  (fw_stream_start + fw_pump) nebo pushem (fw_publish). Poslaný ve
 *  stejné dávce jako pomalá práce nic neřeší, protože dávka dorazí
 *  až s ní.
 * ------------------------------------------------------------------- */

function fw_busy(string $sel, string $text = '', ?int $pct = null): array {
    $c = ['op' => 'busy', 'sel' => $sel];
    if ($text !== '')     $c['text'] = $text;
    if ($pct !== null)    $c['pct']  = $pct;
    return $c;
}

function fw_busy_done(string $sel, string $text = 'Hotovo'): array {
    return ['op' => 'busy', 'sel' => $sel, 'state' => 'done', 'text' => $text];
}

function fw_busy_off(string $sel): array {
    return ['op' => 'busy', 'sel' => $sel, 'state' => 'off'];
}

/* ---------------------------------------------------------------------
 *  PUSH — publikace dávky do nchanu.
 *  Krátký POST na localhost, žádný trvale běžící PHP proces.
 *  Vrací false, když broker neběží; volající se může rozhodnout.
 * ------------------------------------------------------------------- */

function fw_publish(string $pub_url, string $token, array $cmds, float $timeout = 2.0): bool {
    if (isset($cmds['op'])) $cmds = [$cmds];
    $body = json_encode(['v' => FW_V, 'cmds' => array_values($cmds)],
                        JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);

    $ctx = stream_context_create(['http' => [
        'method'        => 'POST',
        'header'        => "Content-Type: application/json\r\nContent-Length: " . strlen($body) . "\r\n",
        'content'       => $body,
        'timeout'       => $timeout,
        'ignore_errors' => true,
    ]]);

    $sep = strpos($pub_url, '?') === false ? '?' : '&';
    $r = @file_get_contents($pub_url . $sep . 'token=' . rawurlencode($token), false, $ctx);
    if ($r === false) return false;

    /* nchan odpoví 201/202 když někdo poslouchá, 202 i když nikdo. */
    foreach ($http_response_header ?? [] as $h)
        if (strncmp($h, 'HTTP/', 5) === 0) return (int)substr($h, 9, 3) < 400;
    return true;
}

/* ---------------------------------------------------------------------
 *  PŘÍMÉ SERVÍROVÁNÍ STRÁNEK
 *  Původní soubory jen mírně upravíme; jejich výstup zachytí buffer.
 * ------------------------------------------------------------------- */

function start_direct_answer(array $cmd): void {
    global $FW_DIRECT;
    $FW_DIRECT = $cmd + ['op' => 'html'];
    ob_start();                                        // vnořený buffer pro obsah
}

function finish_direct_answer(): void {
    global $FW_DIRECT;
    if ($FW_DIRECT === null) return;
    $c = $FW_DIRECT; $FW_DIRECT = null;
    $c['content'] = (string)ob_get_clean();
    send_answer($c);
}

/* ---------------------------------------------------------------------
 *  BOOTSTRAP — volat jako první řádek endpointu.
 * ------------------------------------------------------------------- */

function fw_boot(): void {
    @ini_set('display_errors', '0');                   // notice nesmí rozbít NDJSON
    /* POZOR: tady bylo ob_end_flush(), přestože komentář říká „zahoď".
       Flush implicitního bufferu odešle, co v něm je — a tím potvrdí
       odpověď i s výchozími hlavičkami PHP. Všechny pozdější header()
       v fw_start_output() a fw_cors() pak selžou na „headers already
       sent", odpověď odejde jako text/html BEZ Access-Control-Allow-Origin
       a prohlížeč to ohlásí jako chybu CORS. Jediný zatoulaný bajt před
       fw_boot() stačí. Zahazujeme, jak komentář odjakživa sliboval. */
    while (ob_get_level() > 0) @ob_end_clean();        // zahoď implicitní buffer PHP-FPM
    @ini_set('implicit_flush', '1');
    ob_start();                                        // jediná úroveň: náš záchytný buffer
    register_shutdown_function('fw_shutdown');
    /* CORS hlavičky nesouvisí s HTTP statusem, takže je můžeme poslat
       hned — ať je klient dostane i tehdy, když se odpověď pokazí
       později. fw_start_output() je pak jen zopakuje do stejné sady. */
    fw_cors();
    if (($_SERVER['REQUEST_METHOD'] ?? '') === 'OPTIONS') {   // CORS preflight
        fw_cors();
        header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
        header('Access-Control-Allow-Headers: X-App-Session, X-App-Serial, Content-Type');
        header('Access-Control-Max-Age: 86400');
        http_response_code(204);
        exit;
    }
}

function fw_shutdown(): void {
    global $FW_STARTED;
    $e = error_get_last();
    if ($e && in_array($e['type'], [E_ERROR, E_PARSE, E_CORE_ERROR, E_COMPILE_ERROR], true)) {
        $msg = FW_DEBUG ? "{$e['message']} @ {$e['file']}:{$e['line']}" : 'Vnitřní chyba serveru';
        if (!$FW_STARTED) { if (ob_get_level() > 0) ob_clean(); http_response_code(500); }
        send_answer(['op' => 'error', 'code' => 500, 'message' => $msg]);
    }
    finish_answer();
}
