<?php
/* =====================================================================
 *  boot.inc — zavedení backendového API
 *
 *  Odpověď je VŽDY JSON a vždy jen jedna. Chyba jde HTTP kódem, ne
 *  příznakem v těle: 400 sémantika, 401 nepřihlášen, 403 bez
 *  oprávnění, 404 nenalezeno, 429 příliš mnoho pokusů, 500 naše chyba.
 *
 *  Proč ne obálka {"ok":false,...} s kódem 200: tohle API se volá i
 *  odjinud než z našeho BFF a některé endpointy smí být nahrazené
 *  předpočítaným souborem na CDN. Status v HTTP je jediná věc, kterou
 *  přečte každý klient i každá proxy stejně.
 * ===================================================================== */

declare(strict_types=1);

/* Ani bajt výstupu před hlavičkami. Notice by rozbil JSON stejně
   spolehlivě jako syntaktická chyba. */
@ini_set('display_errors', '0');
while (ob_get_level() > 0) @ob_end_clean();

require __DIR__ . '/input.inc';

$GLOBALS['api_done'] = false;

/* ---------------------------------------------------------------------
 *  ODPOVĚĎ
 * ------------------------------------------------------------------- */

function api_out(array $data, int $code = 200): void {
    if ($GLOBALS['api_done']) return;          // pojistka proti dvojí odpovědi
    $GLOBALS['api_done'] = true;
    http_response_code($code);
    header('Content-Type: application/json; charset=utf-8');
    header('Cache-Control: no-store');
    echo json_encode($data, JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
    exit;
}

/* Chyba. Text je určený UŽIVATELI — půjde mu do červené hlášky — takže
   v něm nesmí být jméno tabulky ani cesta na disk. Podrobnosti patří
   do logu, ne do odpovědi. */
function api_error(int $code, string $msg, string $log = ''): void {
    if ($log !== '') error_log("[library-api] $code: $log");
    api_out(['error' => $msg, 'code' => $code], $code);
}

/* Fatál se nesmí projevit jako prázdná odpověď nebo jako HTML s cestou
   na disk. Tohle je to poslední místo, kde se to dá zachytit. */
register_shutdown_function(function () {
    $e = error_get_last();
    if (!$e || !in_array($e['type'], [E_ERROR, E_PARSE, E_CORE_ERROR, E_COMPILE_ERROR], true)) return;
    error_log("[library-api] fatal: {$e['message']} @ {$e['file']}:{$e['line']}");
    if ($GLOBALS['api_done']) return;
    $GLOBALS['api_done'] = true;
    if (!headers_sent()) { http_response_code(500); header('Content-Type: application/json; charset=utf-8'); }
    echo json_encode(['error' => 'Vnitřní chyba serveru', 'code' => 500,
                      'detail' => (defined('FW_DEBUG') && FW_DEBUG)
                                  ? "{$e['message']} @ {$e['file']}:{$e['line']}" : null]);
});

/* ---------------------------------------------------------------------
 *  DATABÁZE
 * ------------------------------------------------------------------- */

function db(): PDO {
    static $db = null;
    if ($db !== null) return $db;
    try {
        $db = new PDO(DB_DSN, DB_USER, DB_PASS, [
            PDO::ATTR_ERRMODE            => PDO::ERRMODE_EXCEPTION,
            PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
            /* Emulaci vypínáme, ať jsou to skutečné prepared statements.
               Bez toho se placeholdery skládají v PHP a celá ochrana
               proti injekci stojí na kvalitě toho skládání. */
            PDO::ATTR_EMULATE_PREPARES   => false,
        ]);
    } catch (Throwable $e) {
        api_error(500, 'Databáze není dostupná', 'PDO: ' . $e->getMessage());
    }
    return $db;
}

/* Zkratky. Všechny berou pole parametrů — do SQL se nikdy nelepí
   hodnota přímo, ani když „je to jen číslo". */
function q(string $sql, array $p = []): PDOStatement {
    try { $s = db()->prepare($sql); $s->execute($p); return $s; }
    catch (Throwable $e) { api_error(500, 'Chyba databáze', $e->getMessage() . ' | ' . $sql); }
}
function q_all(string $sql, array $p = []): array  { return q($sql, $p)->fetchAll(); }
function q_one(string $sql, array $p = []): ?array { $r = q($sql, $p)->fetch(); return $r === false ? null : $r; }
function q_val(string $sql, array $p = [])         { $r = q($sql, $p)->fetch(PDO::FETCH_NUM); return $r === false ? null : $r[0]; }

/* ---------------------------------------------------------------------
 *  REDIS (volitelný)
 *
 *  Prázdný REDIS_HOST znamená, že se session drží v MySQL. Volající to
 *  nepozná — rozhoduje se jen tady a v api_session.inc.
 * ------------------------------------------------------------------- */

function redis(): ?Redis {
    static $r = false;
    if ($r !== false) return $r;
    $r = null;
    if (REDIS_HOST === '' || !class_exists('Redis')) return $r;
    try {
        $c = new Redis();
        if ($c->connect(REDIS_HOST, (int)REDIS_PORT, 1.0)) $r = $c;
        else error_log('[library-api] redis: connect selhal, jedu na MySQL');
    } catch (Throwable $e) { error_log('[library-api] redis: ' . $e->getMessage()); }
    return $r;
}
