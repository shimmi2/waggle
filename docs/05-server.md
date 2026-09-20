# 05 — Server (`fw.inc`)

Jeden soubor. Neřeší databázi, šablony ani autentizaci — to je věc projektu.

## Start

```php
require __DIR__ . '/config.inc';   // přebije výchozí hodnoty frameworku
require __DIR__ . '/../fw.inc';
fw_boot();
```

`fw_boot()` vypne `display_errors` (notice uprostřed odpovědi by rozbil
NDJSON), zahodí implicitní buffer PHP-FPM, otevře vlastní záchytný buffer,
zaregistruje shutdown handler a odbaví CORS preflight.

## Vstupy

Framework garantuje **tři věci a nic víc**: skalární string omezené délky
bez NUL bajtu.

```php
$a  = req('nazev', 128);     // string, ořezaný, bez NUL
$n  = req_int('pocet');      // int — číslo nejde nikam injektovat
$f  = req_float('cena');
$s  = req_header('X-App-Session');

is_word($function)           // ^[A-Za-z0-9_-]+$ přes strspn (41 ns)
```

`req()` vrátí prázdný string, když přijde pole (`?x[]=1`) — bez toho by
na PHP 8 spadl jakýkoli endpoint na `TypeError`.

Tabulkové formuláře posílají `fd[i][sloupec]`, což skalární getter nepustí.
Na ně je `req_rows()`, který propustí jen dvojúrovňové pole skalárů
s omezením počtu řádků i délky hodnot:

```php
foreach (req_rows('fd', 500) as $i => $row) {
    $id  = intval($row['id'] ?? 0);
    $txt = db_esc($row['name'] ?? '');
}
```

Žádná regulární výrazy, žádné rozšíření, žádná modifikace dat kromě
ořezu délky a odstranění NUL.

## Odpověď

```php
send_answer(['op' => 'html', 'sel' => 'main', 'content' => $html]);
send_answer([ ... , ... ]);     // i pole příkazů
finish_answer();                // odešle; volá se i automaticky na konci
```

`send_answer()` **jen řadí do fronty**. Odpověď je proto atomická: dokud
nedoběhne skript, neodešel ani bajt.

## Chyby

```php
throw_http_error(400, 'Název je povinný');
```

Před prvním bajtem nastaví HTTP status a pošle `op:"error"`. Po prvním
bajtu pošle jen příkaz. Shutdown handler chytí i fatální chyby a promění
je v `op:"error"` s kódem 500 — NDJSON zůstane celistvé.

Při `FW_DEBUG` hláška obsahuje soubor a řádek. **Na produkci vypnout.**

## Přímé servírování stránek

Pro migraci starých souborů, které jen něco vypíšou:

```php
start_direct_answer(['sel' => 'main']);   // od teď se výstup zachytává
require "$PAGES/$function.inc";           // původní soubor beze změny
finish_direct_answer();                   // buffer -> příkaz html
```

## Stream

```php
fw_stream_start();                 // prorazí buffer webserveru
foreach (...) {
    send_answer($cmds);
    flush_answer();                // odešle frontu hned
    if (connection_aborted()) exit;
    usleep(100000);
}
```

`flush_answer()` protlačí **všechny** úrovně output bufferu. Bez toho by
dávka jen spadla do implicitního 4 kB bufferu PHP-FPM.

`fw_stream_start()` pošle 8 kB mezer, které klient zahodí. Pomáhá u proxy,
které flushují po naplnění bufferu; na Apache s `mod_proxy_fcgi` **nepomůže**
— viz [11 — Problémy](11-problemy.md).

Stream drží proces webserveru po celou dobu. U čehokoli delšího než pár
sekund je lepší push.

## Push

```php
fw_publish(STREAM_PUB_URL, $token, $cmds);   // false = broker neběží
```

Krátký POST na localhost, žádný trvale běžící PHP proces. Návratovou
hodnotu má smysl testovat hned na začátku, dokud lze ještě vrátit
poctivý HTTP status.

## Konfigurace

Framework používá `defined() || define()`, takže projekt přebije výchozí
hodnoty tím, že si je nadefinuje **před** načtením `fw.inc`:

| konstanta | výchozí | význam |
|---|---|---|
| `FW_V` | `1` | verze protokolu |
| `FW_MAXLEN` | `65536` | strop délky jednoho pole |
| `FW_DEBUG` | `false` | chybová hláška prozradí cesty |
| `FW_STREAM_PAD` | `8192` | výplň pro `fw_stream_start()` |

## Přehled API

| | |
|---|---|
| `fw_boot()` | inicializace, první řádek endpointu |
| `req()`, `req_int()`, `req_float()`, `req_header()`, `is_word()` | vstupy |
| `send_answer()`, `flush_answer()`, `finish_answer()` | odpověď |
| `throw_http_error()` | chyba |
| `start_direct_answer()`, `finish_direct_answer()` | zachytávání výstupu |
| `fw_stream_start()` | zahájení streamu |
| `fw_publish()` | publikace do push kanálu |
