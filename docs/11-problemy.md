# 11 — Problémy a pasti

Všechno v téhle kapitole jsou věci, na které jsme při stavbě skutečně
narazili. Každá stála čas, který tu nemusíš strávit znovu.

## Stream se doručuje po blocích, ne plynule

**Projev:** progress bar skáče po ~4 s místo plynulého růstu.

**Není to PHP.** Na vestavěném serveru PHP (7.4 i 8.4) se dávky doručují
přesně po 0,1 s. Drží to Apache `mod_proxy_fcgi`.

**Co nepomůže:** výplň na začátku streamu. Zkoušeno až do 64 kB.

**Co pomůže:** `flushpackets=on` u proxy workeru (viz
[10 — Nasazení](10-nasazeni.md)), nebo místo streamu použít push.

**Mimochodem:** `flush_answer()` protlačuje **všechny** úrovně output
bufferu. Pouhé `ob_flush()` by dávku jen přesypalo do implicitního 4 kB
bufferu PHP-FPM a klient by ji viděl až po jeho naplnění.

## CORS: „Access-Control-Allow-Origin neodpovídá"

**Projev:** prohlížeč zablokuje SSE spojení a v hlášce je **víc originů
oddělených čárkou**.

**Příčina:** hlavička je v odpovědi **dvakrát**. nchan dělá CORS sám
a odráží `Origin` požadavku; každý vlastní `add_header Access-Control-*`
přidá druhou. Prohlížeč obě spojí čárkou a výsledek je nevalidní.

**Řešení je ubrat, ne přidat.** Žádné `add_header Access-Control-*`
v nchan location.

**Jak poznat:**
```bash
curl -s -D - -o /dev/null -H "Origin: https://host" "https://host:8443/nchan/sub?token=x" \
  | grep -ci access-control-allow-origin      # musí být 1
```

## nchan vrací 403 při testu curlem

**Projev:** subscriber endpoint vrací 403, i když je všechno správně.

**Příčina:** `nchan_subscriber eventsource` vyžaduje hlavičku
`Accept: text/event-stream`. Prohlížeč ji posílá sám, curl ne.

```bash
curl -N -H "Accept: text/event-stream" "https://host:8443/nchan/sub?token=…"
```

Když v access logu vidíš u požadavku z prohlížeče **499**, znamená to, že
se spojení navázalo a prohlížeč ho zrušil sám — tedy chyba je na straně
CORS, ne nchanu.

## www versus bez www

**Projev:** CORS nesedí, i když je origin povolený.

**Příčina:** URL odběru natvrdo s `www`, stránka běží bez něj (nebo obráceně).

**Řešení:** odvodit URL z hostu požadavku (`stream_sub_url()`). `Host` je
vstup od klienta, takže patří přes kontrolu znaků.

## Šablona se reinicializuje stokrát za vteřinu

**Projev:** v konzoli se opakuje hláška o reinicializaci, aplikace je
trhaná a reakce nejsou okamžité.

**Příčina:** hook `afterReplace` volá init šablony po **každé** výměně.
Progress bar mění jen text v jednom `<span>`.

**Řešení:** reinicializovat jen tehdy, když v doručeném kusu skutečně je
něco, co šablona obsluhuje — viz [07 — Integrace](07-integrace.md).

## Fragment obsahuje `<script>`, který se neprovede

`innerHTML` skripty nespouští. Není to chyba frameworku, je to chování
prohlížeče. Použij příkaz `call` poslaný za příkazem `html`.

## Mixed content

Z https stránky nelze otevřít `EventSource` na http. Prohlížeč to zablokuje
bez varování. Subscriber endpoint musí mít TLS.

## `?x[]=1` shodí endpoint

Na PHP 8 hodí `addslashes()` i většina string funkcí na poli `TypeError`,
tedy HTTP 500. Je to DoS na jeden parametr. `req()` vrací pro pole
prázdný string.

## Prázdný chybový rám po startu

Když má `#fw_error` třídu, kterou framework kreslí (třeba Bootstrap
`.alert`), visí po startu prázdný. `Fw.init()` ho proto vyprázdní a schová;
při vlastním stylování na to pamatuj.

## Tlačítko Zpět přestalo fungovat

Po přechodu na částečné překreslování se historie neplní sama. Přidej
příkaz `history` u navigací — a **nikdy po odeslání formuláře**, přehrávat
POST tlačítkem Zpět je vždycky špatně.

## Stránka volá metodu na `null`

**Projev:** `Call to a member function tsquery() on null` uprostřed stránky.

**Příčina:** dispatcher includuje stránky **uvnitř funkce**, takže jejich
kód na nejvyšší úrovni běží ve funkčním rozsahu. Staré API typu
`mysql_query()` bylo obyčejná funkce s implicitním spojením a proměnnou
nepotřebovalo; `$db->tsquery()` ji potřebuje.

**Řešení:** `global $db, $pdb;` v dispatcheru i v každé funkci, která
s databází pracuje. Chybí to typicky u funkcí, které dřív žádnou globálku
nedeklarovaly.

## Lint chybu nenajde

`php -l` odhalí jen chyby parsování. Tyhle ne:

* nedefinovaná funkce (`db_esc()` chybí v knihovně, kterou projekt nenačetl)
* nekvótovaný klíč `$_REQUEST[od]` — na PHP 8 fatální **až za běhu**
* dělení nulou — na PHP 7 warning, na PHP 8 `DivisionByZeroError`

Proto se vyplatí ověřovátko, které načte stejnou sadu knihoven jako
produkční vstupní bod a projde tokenizerem každé volání funkce:

```php
$def = array_flip(array_map('strtolower', array_merge(
    get_defined_functions()['internal'], get_defined_functions()['user'])));
// … pro každý T_STRING následovaný '(' zkontroluj isset($def[$name])
```

Odhalí chybějící funkci dřív než uživatel.

## Odpověď není JSON

Když endpoint vypíše cokoli mimo `send_answer()` (echo, notice, BOM),
rozbije to NDJSON. `fw_boot()` proto vypíná `display_errors` a otevírá
záchytný buffer; stray výstup se při `FW_DEBUG` pošle jako `op:"debug"`.

## Změna ve `fw.js` se neprojeví

Příznak: nová klientská vlastnost (atribut, operace) nefunguje, přestože
soubor na serveru je správně a `curl` ho stáhne i s tou změnou. Ostatní
věci přitom chodí.

Prohlížeč má v cache starší `fw.js`. Shell ho načítá bez verze v URL a
Apache k němu neposílá `Cache-Control`, takže se použije heuristika.

Ověření je na jedno slovo v konzoli:

```js
typeof Fw.clear        // "undefined" -> běží stará fw.js
```

Řešení hned: hard reload. Řešení natrvalo: revalidovat vývojové soubory.

```apache
<Directory /var/www/hosting/a/priklad>
    <FilesMatch "\.(js|html)$">
        Header set Cache-Control "no-cache, must-revalidate"
    </FilesMatch>
</Directory>
```

`no-cache` neznamená „neukládat" — soubor se uloží, ale prohlížeč se
vždycky zeptá a dostane 304, pokud se nic nezměnilo.

Než začneš hledat chybu v kódu, vylučuj tohle jako první. Stojí to
deset sekund a opačné pořadí stojí půl hodiny.
