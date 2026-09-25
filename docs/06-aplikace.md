# 06 — Aplikace

## Struktura

```
api/
  config.inc        VŠECHNO nastavení na jednom místě, načítá se první
  index.php         dispatcher
  inc/
    app.inc         esc(), render(), pomocné funkce projektu
    auth.inc        přihlášení a oprávnění
    stream.inc      push kanály a tokeny
  bin/              workery na pozadí (.htaccess: Require all denied)
  data/             zápisy za běhu    (.htaccess: Require all denied)
  pages/plain/      fragmenty pro /app
  pages/lte/        fragmenty pro /app2
```

`pages/` je pro HTTP celý zavřený. Fragmenty čte `index.php` z filesystému,
nikdy je nestahuje prohlížeč.

## Tři okna, o která se aplikace dělí

Povinné to není, ale většina firemních aplikací se přirozeně rozpadne
na tři okna — a když se to přijme, je řízení aplikace triviální:

| okno | co v něm je | jak často se mění |
|---|---|---|
| `left_menu` | navigace | zpravidla **jen při přihlášení a odhlášení** |
| `top_frame` | stavové údaje — počet věcí k vyřízení, rychlé odkazy | občas, a jen ony samy |
| `main` | vlastní práce | **pořád** |

Z toho plyne, že drtivá většina odpovědí sahá jen na `main`. Menu se
překresluje dvakrát za sezení a horní rám jen tehdy, když se změní číslo,
které ukazuje.

`main` se pak vyplatí rozdělit ještě na **okno filtru a okno výsledků**.
Filtr zůstane stát i s fokusem a překresluje se jen tabulka pod ním, takže
uživatel může rovnou psát dál. Takhle vypadá typický cyklus:

```json
{"op":"html","sel":"#goods_results","content":"…tabulka…"}
```

A to je celé. Žádný stav na klientovi, žádný model k synchronizaci —
stavový model drží server a klient jen vykresluje, co dostane.

## Skiny — ale jen v příkladu

**Tohle je volba dema, ne doporučení pro produkci.** Dvě sady fragmentů
tu jsou proto, aby bylo vidět, že se markup dá vyměnit, aniž by se hnulo
cokoli jiného. Skutečný projekt má zpravidla **jednu šablonu** a těžiště
úplně jinde: v mnoha jednoduchých endpointech a v tahu na branku.
Přepínání skinů si přidá ten, kdo ho opravdu potřebuje.

Aplikace posílá skin v každém požadavku:

```js
Fw.init({ api: '../api/', params: { skin: 'lte' } });
```

```php
$skin  = req('skin', 8) === 'lte' ? 'lte' : 'plain';
$PAGES = __DIR__ . '/pages/' . $skin;
```

Endpointy, session ani protokol se mezi skiny neliší. Liší se **výhradně
markup** — což je přesně ta hranice, kde má být šablona. Proto se dá přidat
třetí skin bez jediného zásahu do logiky.

## Šablona endpointu

**Tohle je nejdůležitější stránka celé dokumentace.** Framework
negarantuje bezpečnost, jen hygienu vstupu — viz
[02 — Principy](02-principy.md), zásada 10. Všechno ostatní stojí na
tom, že každý endpoint vypadá takhle a v tomhle pořadí. Je to
**podmínka, ne styl**.

Dá se přitom předat člověku i jazykovému modelu se stejným výsledkem,
a právě to je důvod, proč tu žádná vrstva navíc není.

```php
case 'save_form_test':
    /* 1. vstupy — vždy jako první, nikdy nesahat na $_REQUEST přímo */
    $nazev = req('nazev', 128);
    $pocet = req_int('pocet');

    /* 2. sémantika — levné kontroly, bez lookupů do DB */
    if ($nazev === '')             throw_http_error(400, 'Název je povinný');
    if ($pocet < 1 || $pocet > 99) throw_http_error(400, 'Počet musí být 1 až 99');

    /* 3. session */
    if ($user === null)                 throw_http_error(401, 'Nejste přihlášen');

    /* 4. oprávnění */
    if (!acl_check($user, 'save_form')) throw_http_error(403, 'Nemáte oprávnění');

    /* 5. práce */
    db_ulozit($nazev, $pocet);

    /* 6. odpověď */
    send_answer(['op' => 'html', 'sel' => 'main',
                 'content' => render("$PAGES/form_done.inc", ['nazev' => $nazev])]);
    break;
```

Pořadí není libovolné:

* **Vstupy první.** Na `$_REQUEST` se nesahá nikde jinde. Jakmile se
  jednou zvykne sahat přímo, přestane být pravda, že vstupy jsou
  ošetřené, a nikdo to nepozná.
* **Sémantika před session** schválně. Je levná, nestojí lookup do
  databáze, a nepřihlášený útočník se tak nedostane k ničemu drahému.
* **Session před oprávněním**, protože bez identity není co ověřovat.
* **Práce až po všech kontrolách**, aby odpověď zůstala atomická.
  Dokud skript nedoběhne, neodešel ani bajt, a jde ještě vrátit poctivý
  HTTP status.

### Jediná přípustná výjimka

Kontrola session smí být **společná pro celý modul API** — jednou
nahoře v dispatcheru místo v každém `case`. Má to smysl u modulu, kde
je nepřihlášený přístup vyloučený ze zásady, třeba `admin/*`.

Nic jiného se vynechávat nesmí. Zvlášť ne oprávnění: to je vždycky věc
konkrétního endpointu, protože „přihlášen" a „smí tohle" jsou dvě různé
otázky.

### Čím to nekončí

Šablona hlídá **cestu dovnitř**. Ven vede cest víc a každá má vlastní
pravidlo:

* do SQL nepatří hodnota bez vazby parametru nebo escapování, **a to
  včetně uvozovek kolem stringu** — `WHERE a='$x'` s ošetřeným `$x` je
  v pořádku, `WHERE a=$x` u stringu není;
* do HTML nepatří nic bez `esc()`;
* do cesty k souboru nepatří nic, co neprošlo `is_word()`;
* do shellu nepatří nic bez `escapeshellarg()`.

Framework neudělá ani jedno z toho za vás — nemůže, protože neví, kam
ta hodnota míří. Celé je to rozvedené v
[09 — Bezpečnost](09-bezpecnost.md).

**Na tomhle stojí a padá bezpečnost celé aplikace.** Endpoint, který
šablonu poruší, je díra bez ohledu na to, jak dobře je napsaný zbytek.

## Stránka jako soubor

Když dispatcher obsluhuje `page_xxx` tím, že hledá `pages/xxx.inc`,
znamená **portování stránky napsat jeden soubor a nic jiného** — žádný
zápis do `switch`, žádná registrace.

Stránka dostane globálky (session, spojení), přečte si vstupy, zkontroluje
oprávnění, udělá práci a vykreslí. Výsledek operace **nehlásí do stránky**:

```php
if ($cmd === 'save') {
    // … validace a zápis …
    bff_notify('success', 'Alias byl uložen.');
    bff_goto('hosting_aliases');    // místo této stránky se vykreslí seznam
    return;
}
```

`bff_goto()` řeší dispatcher: po doběhnutí stránky vykreslí místo ní tu
druhou a přepíše historii, takže tlačítko Zpět sedí.

### Překreslit jen část stránky

Výpis s filtrem se vyplatí rozdělit na dva cíle — filtr a tabulku — a při
filtrování, řazení nebo hromadné akci překreslovat **jen tabulku**. Hlavní
zisk není objem dat, ale že **ve filtru zůstane kurzor a focus**; překreslení
celé oblasti při psaní do vyhledávacího pole je nepoužitelné.

```php
if ($partial) bff_partial('#users_table');   // jinak se vykreslí celý #main
```

Dispatcher pak pošle `html` na ten selektor **a nezapíše historii** —
útržek bez kostry nesmí jít vrátit tlačítkem Zpět. Formuláře do historie
nezapisují nikdy, takže filtr je bezpečný sám o sobě; u odkazů (řazení,
hromadné akce) to zajišťuje právě `bff_partial()`.

Návrat z editace posílá celý `#main` — kostra se tím obnoví.

**Past:** cílová stránka čte tytéž `$_REQUEST`. Kdyby v nich zůstalo
`cmd=mdel` a `id`, provedla by akci **podruhé**. Před vykreslením cíle je
nutné je vynulovat.

## Šablony

Framework šablonovací systém nemá. Demo používá `render()` — obyčejný
`include` do bufferu:

```php
function render(string $file, array $vars = []): string {
    extract($vars, EXTR_SKIP);
    ob_start();
    require $file;
    return (string)ob_get_clean();
}
```

Ve fragmentu se pak píše normální PHP a **každá proměnná jde přes `esc()`**:

```php
<tr><th>Uživatel</th><td><?= esc($user['name']) ?></td></tr>
```

## Zkratka `page_*`

`function=page_xxx` obslouží `pages/<skin>/page_xxx.html` (statický) nebo
`.inc` (generovaný) přes output buffer, bez jediné řádky ve `switch`.

Cesta je bezpečná z konstrukce — `is_word()` nepustí `/`, `.` ani NUL.
Je to hlavní vstupní brána pro migraci starých stránek.

## Session v demu

Podepsaný token, žádné úložiště:

```php
$payload = base64url({u: login, s: serial, exp: čas});
$token   = $payload . '.' . base64url(hmac_sha256($payload, AUTH_SECRET));
```

Token je svázaný se sériovým číslem instalace, takže odcizený token na
jiném zařízení neprojde. V reálné aplikaci sem patří databáze nebo Redis —
framework o session neví nic, jen ji přenáší hlavičkou.

## Workery na pozadí

```php
stream_spawn(__DIR__ . '/bin/worker_progress.php', ['token' => $tok]);
```

Zapíše argumenty do dočasného souboru s právy 0600, spustí PHP na pozadí
a skončí. Token se nepředává přes `argv`, protože `argv` je v `ps` vidět
všem uživatelům stroje.

Worker je v `bin/`, který je pro HTTP zavřený, a začíná kontrolou
`PHP_SAPI !== 'cli'`.

## Demo endpointy

| funkce | ukazuje |
|---|---|
| `index`, `do_login`, `do_logout` | kompletní obrazovka, `session`, `subscribe` |
| `form_test`, `save_form_test` | formulář, validace 400/401/403 |
| `page_about`, `page_time` | zkratka `page_*`, statický i generovaný |
| `ping` | jemné adresování — `html` + `class` + `attr` bez překreslení karty |
| `sync` | synchronizace mezi okny, vlastní operace |
| `progress_run` | stream, PHP drží spojení 10 s |
| `progress_push` | push, PHP odpoví za ~20 ms |
| `boom` | fatální chyba → 500 s celistvým NDJSON |

## Komponenty

Komponenta je endpoint jako každý jiný — liší se jen tím, že kreslí do
vlastního okna a s hostitelskou stránkou mluví protokolem, ne PHP.

Typický případ je výběr hodnoty do formuláře (adresa z RÚIAN, zákazník).
Ve staré aplikaci to bylo `window.open()` na samostatný skript, který
výsledek vracel přes `opener.document.<formulář>.<pole>.value` a zavřel se.
Tím znal cizí formulář jménem a žil a padal s `window.opener`.

Tady je to takhle:

1. Shell má vedle `#main` ještě prázdné okno `#overlay`. Prázdné = zavřeno.
2. Hostitel otevře komponentu odkazem a pošle jí **id svých polí**:
   `#?function=page_pick_ruian&target=us_contact&dsc=us_contact_dsc`
3. Komponenta kreslí do `#overlay` (`bff_partial('#overlay')`). Hledá,
   stránkuje, filtruje — všechno ve svém okně.
4. Při výběru pošle hostiteli dvě operace `value` a vypíše prázdno:

   ```json
   {"op":"value","sel":"#us_contact","value":"40991342"}
   {"op":"value","sel":"#us_contact_dsc","value":"Hrčava, 105"}
   {"op":"html","sel":"#overlay","content":""}
   ```

Co z toho plyne:

- **Hostitel se nepřekresluje.** Rozepsaný formulář zůstane, jak byl —
  to je hlavní důvod, proč komponenta nesmí kreslit do `#main`.
- **Komponenta nezná hostitele.** Dostala dvě id a víc jí netřeba; stejný
  `pick_ruian` obslouží libovolnou stránku.
- **Popisek se čte z databáze**, po drátě chodí jen kód. Odkaz tedy nenese
  nic, čemu by se muselo věřit.

Jediné, co je potřeba ohlídat, jsou ta id: skládá se z nich CSS selektor,
takže musí projít `is_word()` (`bff_pick_id()`). Podvržený `target` se
zahodí a operace `value` se vůbec nepošle.

Zavřít se dá dvěma způsoby a oba jsou v pořádku:

- `data-fw` na `cmd=close` — komponenta nevypíše nic a překryv dostane
  prázdný obsah. Jeden malý dotaz navíc; takhle to dnes dělají pickery.
- `data-fw-clear="#overlay"` — totéž, ale bez cesty na server.

## Ladění dotazů

`bff_sql_note($popis, $sql, $t0, $rows)` pošle pod `FW_DEBUG` do konzole
prohlížeče dotaz tak, jak doopravdy odešel, jeho čas a `EXPLAIN`. Hodí se
přesně na případy typu „vždyť ten dotaz je přesný, proč trvá vteřiny":

```
[fw] SQL [počet adres] 686.7 ms      {ms, rows, sql, explain: [...]}
[fw] SQL [seznam adres] 2622 ms, 50 řádků
```
