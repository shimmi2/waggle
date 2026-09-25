# 05 — Server (`fw.inc`)

Jeden soubor. Neřeší databázi, šablony ani autentizaci — to je věc projektu.

Chystá se k němu sada **skillů, šablon a příkladů pro okamžité
nasazení**, aby se kompletní aplikace i s databází, session a
oprávněními dala vybuildit jedním promptem, ve dvouvrstvé i třívrstvé
podobě. Je to **plán, ne stav**; podrobněji v
[01 — Motivace](01-motivace.md). Na hranici knihovny to nic nemění —
budou to příklady a nástroje, ne závislosti.

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
$s  = req_header('X-App-Session');   // token sezení
$i  = req_header('X-App-Serial');    // identifikace instalace v prohlížeči

is_word($function)           // ^[A-Za-z0-9_-]+$ přes strspn (41 ns)
```

`req()` vrátí prázdný string, když přijde pole (`?x[]=1`) — bez toho by
na PHP 8 spadl jakýkoli endpoint na `TypeError`.

Klient posílá dvě hlavičky a je dobré je nezaměňovat. **`X-App-Session`**
je token sezení: server ho mění příkazem `session` a je to ta věc, která
rozhoduje o přihlášení. **`X-App-Serial`** je trvalá identifikace
instalace v prohlížeči — přežije odhlášení i zavření okna a slouží
k tomu, aby šlo poznat, že požadavek přišel z téhož zařízení. Obě jsou
vstup od klienta jako každý jiný, takže žádná z nich sama o sobě nic
nedokazuje; jak se z nich dělá důvěra, je v
[09 — Bezpečnost](09-bezpecnost.md).

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

## Zaneprázdněno

```php
fw_busy('#seznam', 'Kompletuji data…');        // kolečko
fw_busy('#seznam', 'Zpracovávám…', 40);        // pruh na 40 %
fw_busy_done('#seznam', 'Hotovo');             // zeleně a za 0,7 s pryč
fw_busy_off('#seznam');                        // okamžitě pryč
```

Jsou to jen stavitelé příkazu — samy nic neodesílají. Užitečné jsou
teprve tehdy, když se dostanou ke klientovi **dřív** než výsledek, tedy
poslané asynchronně: streamem, nebo pushem. Ve stejné dávce jako pomalá
práce nedělají nic, protože ta dávka dorazí až s ní.

Psaní to zjednodušuje víc, než se zdá, a to kvůli dvěma věcem, které
hlídá framework:

* **Do jednoho cíle se dá posílat opakovaně.** „Připravuji…",
  „Počítám… 40 %", „Dokončuji…" jsou tři obyčejná volání za sebou.
  Nevzniknou tři překryvy, ten jeden se jen aktualizuje, a volající si
  nemusí pamatovat, jestli už nějaký visí.
* **Uklidí se sám.** Jakmile do téhož okna dorazí výsledek, framework
  překryv sundá. Na šťastné cestě proto `fw_busy_off()` posílat
  netřeba — a hlavně se nemůže stát, že po chybě někde zůstane viset
  kolečko, protože se zapomnělo na úklid.

Chování překryvu do detailu popisuje [03 — Protokol](03-protokol.md).

## Push

```php
fw_publish(STREAM_PUB_URL, $token, $cmds);   // false = broker neběží
```

Krátký POST na localhost, žádný trvale běžící PHP proces. Návratovou
hodnotu má smysl testovat hned na začátku, dokud lze ještě vrátit
poctivý HTTP status.

**Jak se to dostane do prohlížeče.** Klient má otevřené spojení
**SSE** — *Server-Sent Events*, standardní část prohlížeče
(`EventSource`). Je to jednosměrný kanál server → klient přes obyčejné
HTTP, které zůstane otevřené a po kterém server posílá zprávy, kdy se
mu zachce. Proti WebSocketu je jednodušší: nemá vlastní protokol ani
handshake, prochází proxy jako běžný požadavek a **prohlížeč se po
výpadku sám znovu připojí** a přes `Last-Event-ID` si vyzvedne, co mu
uteklo. Oproti pollingu odpadá dotazování naprázdno.

Jednosměrnost nevadí, protože opačný směr už máme: klient se ptá
obyčejným požadavkem. Waggle tak potřebuje jen to, co SSE nabízí.

Ten kanál nedrží PHP, ale nchan v nginxu. PHP do něj jen krátce
publikuje a skončí, takže nedrží žádné spojení a neobsazuje worker
Apache. Nastavení je v [10 — Nasazení](10-nasazeni.md).

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
| `req()`, `req_int()`, `req_float()`, `req_rows()`, `req_header()`, `is_word()` | vstupy |
| `send_answer()`, `flush_answer()`, `finish_answer()` | odpověď |
| `throw_http_error()` | chyba |
| `start_direct_answer()`, `finish_direct_answer()` | zachytávání výstupu |
| `fw_stream_start()` | zahájení streamu |
| `fw_busy()`, `fw_busy_done()`, `fw_busy_off()` | překryv „pracuji" |
| `fw_publish()` | publikace do push kanálu |
