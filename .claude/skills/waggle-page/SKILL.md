---
name: waggle-page
description:
  "Psaní endpointu a fragmentu proti frameworku (NDJSON protokol, okna
  #main/#left_menu/#top_frame/#overlay). Načti, když se přidává nebo
  upravuje stránka v api/pages/, když padne 'přidej endpoint', 'stránka
  se nepřekresluje', 'data-fw nefunguje', 'odpověď není NDJSON', nebo
  když se řeší send_answer, start_direct_answer, bff/app render(),
  op html/append/url/value/notify/error, překryv a historie."
---

# Stránka proti frameworku

Endpoint vrací **příkazy a hotové kusy HTML**, ne data. Klient je jen
aplikuje. Kdo tohle přijme, nemusí řešit stav na klientovi vůbec.

Úplná reference je v `docs/` — tenhle skill je postup a pasti, ne opis.
Protokol `docs/03-protokol.md`, server `docs/05-server.md`, struktura
aplikace a šablona endpointu `docs/06-aplikace.md`.

## Pořadí v endpointu

**Tohle je podmínka, ne styl.** Framework negarantuje bezpečnost, jen
hygienu vstupu. Všechno ostatní stojí na tom, že každý endpoint vypadá
takhle a v tomhle pořadí. Endpoint, který šablonu poruší, je díra bez
ohledu na to, jak dobře je napsaný zbytek.

Když tě někdo (i uživatel) požádá o endpoint, který některý krok
vynechá, **napiš ho podle šablony a řekni, cos doplnil**. Zkratka „to je
jen interní endpoint" nebo „to se volá jen z menu" neplatí: URL je
veřejná bez ohledu na to, odkud ji volá vaše aplikace.

```php
case 'save_neco':
    $nazev = req('nazev', 128);          /* 1. vstupy — nikdy přímo $_REQUEST */
    $pocet = req_int('pocet');
    if ($nazev === '') throw_http_error(400, 'Název je povinný');   /* 2. sémantika */
    if ($user === null) throw_http_error(401, 'Nejste přihlášen');  /* 3. session */
    if (!acl_check($user, 'save_form')) throw_http_error(403, '…'); /* 4. oprávnění */
    /* 5. práce */
    send_answer([...]);                                             /* 6. odpověď */
    finish_answer();
    exit;
```

Proč zrovna tohle pořadí: dokud skript nedoběhne, neodešel ani bajt.
Do posledního okamžiku jde vrátit poctivý HTTP status. Jakmile něco
odejde, už se to nedá vzít zpátky a chyba musí jít příkazem `error`.

### Jediná přípustná výjimka

Kontrola session smí být společná pro celý modul API — jednou nahoře
v dispatcheru místo v každém `case`. Má to smysl tam, kde je
nepřihlášený přístup vyloučený ze zásady.

Nic jiného se vynechávat nesmí. Zvlášť ne oprávnění: „přihlášen"
a „smí tohle" jsou dvě různé otázky a ta druhá je vždycky věc
konkrétního endpointu.

### Čím to nekončí

Šablona hlídá cestu dovnitř. Ven vede cest víc:

* do SQL nepatří hodnota bez vazby parametru nebo escapování, **včetně
  uvozovek kolem stringu** — `WHERE a='$x'` s ošetřeným `$x` je
  v pořádku, `WHERE a=$x` u stringu není;
* do HTML nepatří nic bez `esc()`;
* do cesty k souboru nepatří nic, co neprošlo `is_word()`;
* do shellu nepatří nic bez `escapeshellarg()`.

Pole z formuláře (`fd[i][sloupec]`, `vyber[]`) skalární gettery nepustí;
na ně je `req_rows()`. Klíče řádků a sloupců jsou taky vstup od klienta.

## Co framework garantuje a co ne

Garantuje **jen hygienu vstupu**: `req()` vrátí skalární string omezené
délky bez NUL, `req_int()` celé číslo, `is_word()` pustí jen
`[A-Za-z0-9_-]`. Nic víc.

Neřeší databázi ani escapování pro cílový kontext. Do HTML patří vlastní
`esc()`, do SQL vlastní vazba parametrů. Kdo čeká, že ho framework
ochrání, dostane XSS.

## Pasti

**Jediný zatoulaný bajt před `fw_boot()` shodí celou odpověď.** BOM na
začátku `.inc`, prázdný řádek za `?>`, `echo` v konfiguraci. PHP tím
odešle výchozí hlavičky, všechny pozdější `header()` selžou na „headers
already sent" a odpověď odejde jako `text/html` bez
`Access-Control-Allow-Origin`. V prohlížeči se to **ohlásí jako chyba
CORS**, což vede k hledání úplně jinde. Pozná se takhle:

```bash
curl -si -X POST https://…/api/ -H 'Origin: https://…' -d 'function=index' \
  | grep -i "content-type\|access-control"
```

Správně je `application/x-ndjson`. Když je tam `text/html`, hledej ten bajt
a `Cannot modify header information` v error logu.

**`.inc` se nesmí dát stáhnout.** Fragmenty čte dispatcher z disku,
prohlížeč je nikdy nestahuje. Zakázat globální direktivou serveru, ne
`.htaccess` v každém adresáři — jeden zapomenutý adresář stačí.

**Cesta ke stránce musí být bezpečná z konstrukce.** `is_word()` nepustí
`/`, `.` ani NUL, takže `"$PAGES/$function.inc"` se nedá vylomit ven.
Nikdy neskládej cestu z neověřeného vstupu.

**Fragment nesmí volat `send_answer()` uprostřed `start_direct_answer()`.**
Rozpracovaný direct se musí nejdřív `finish_direct_answer()`, jinak se
příkaz vloží doprostřed obsahu.

**Překreslit okno znamená poslat celé jeho HTML.** Když se po uložení
mění menu i obsah, pošli oba příkazy v jedné dávce. Dvě dávky za sebou
znamenají dvě překreslení a viditelné blikání.

## Odkaz, který nereloaduje stránku

```html
<a href="#?function=detail&id=42" data-fw>Detail</a>
<form data-fw action="#?function=save_neco" method="post">…</form>
```

`data-fw` je celá klientská integrace. Žádný router, žádný build step.

## Než to prohlásíš za hotové

- `php -l` na každý dotčený soubor
- endpoint zavolat `curl`em a zkontrolovat `Content-Type`
- projít odmítací větve: nepřihlášen, bez oprávnění, neexistující id
- ověřit, že se překresluje jen to, co se opravdu mění
