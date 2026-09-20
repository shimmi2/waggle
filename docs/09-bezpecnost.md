# 09 — Bezpečnost

## Základní pravidlo

**„Bezpečné" není vlastnost hodnoty, ale dvojice hodnota + cíl.**

Nejde napsat funkci, po které je řetězec bezpečný pro všechno. Escapování
pro SQL, HTML, shell a filesystém se vzájemně vylučuje. Proto:

> Na vstupu **validuj a typuj**. Na výstupu **escapuj podle cíle**.

Kdyby `req()` escapovalo HTML rovnou, uloží se do databáze `O&#039;Brien`,
porovnání `$u === "O'Brien"` selže, `strlen` vrátí 12 místo 7, a proti
SQL injection to stejně neudělá nic — `1 OR 1=1` projde beze změny.
Přesně tohle byly `magic_quotes`, které PHP v 5.4 vyhodilo.

## Vstup: tři garance

```php
req($name, $max)   // skalární string, omezená délka, bez NUL
req_int($name)     // int
req_float($name)   // float
```

`req()` vrátí prázdný string, když místo hodnoty přijde pole. Bez toho by
`?user[]=x` shodil endpoint na PHP 8 fatální chybou — což je DoS na jeden
parametr.

Čísla není potřeba dál kontrolovat. Po `intval()` neexistuje řetězec,
který by nesl útok.

## Výstup: escapuj u cíle

| cíl | čím |
|---|---|
| HTML | `esc()` = `htmlspecialchars(…, ENT_QUOTES \| ENT_SUBSTITUTE, 'UTF-8')` |
| SQL — řetězec | `mysqli_real_escape_string()` **a vždy v uvozovkách** |
| SQL — číslo | `intval()` / `floatval()`, nikdy v uvozovkách |
| shell | `escapeshellarg()` |
| cesta | `is_word()`, případně `basename()` |

`ENT_SUBSTITUTE` zároveň řeší nevalidní UTF-8 (nahradí U+FFFD), takže
se validace kódování na vstupu nemusí dělat vůbec.

### Proč ne `addslashes()`

Escapuje čtyři znaky pro jeden kontext. Rozbije se triviálně:

* **číselný kontext** — `WHERE id = $id` s hodnotou `1 OR 1=1` projde;
  žádný apostrof tam není, takže `addslashes()` neudělá nic
* **`LIKE`** — `%` a `_` neescapuje
* **identifikátory** — backtick neřeší
* **vícebajtové charsety** (GBK, Big5, SJIS) — `0xBF` + `'` se změní na
  platný znak plus volný apostrof; proto existuje
  `mysqli_real_escape_string()`, která zná charset spojení
  *(na UTF-8 spojení tenhle konkrétní trik nefunguje)*
* **jiné domény** — pro HTML, shell, cesty ani hlavičky nedělá nic
* **PHP 8** — na poli hodí `TypeError`, tedy fatální chybu

## Cesty

Traversal se neřeší kontrolou, ale konstrukcí:

```php
$function = req('function', 64);
if (!is_word($function)) throw_http_error(400, 'Neplatný název');
// od téhle chvíle je jisté, že tam není '/', '.' ani NUL
```

## Adresáře

### Konvence: `.inc` je všechno, `.php` jen vstupní bod

V celém stromu má příponu `.php` **pouze** to, co někdo skutečně spouští:

* `index.php` — jediný veřejný vstup aplikace
* samostatná API volaná zvenčí
* skripty pouštěné z cronu nebo z shellu

Všechno ostatní — knihovny, šablony, fragmenty stránek, konfigurace —
je `.inc` a webserver to nikdy neservíruje.

Praktický důsledek: knihovna s příponou `.php` v adresáři, kam sahá
webserver, **se dá spustit přímo z URL** mimo dispatcher. Typicky pak
vypíše fatální chybu i s cestou na disk, protože jí chybí kontext.
V portálu takhle vyskočilo `/cz/user.php`.

Než něco přejmenuješ, změř to: soubor, na který vede `include`, je
knihovna; soubor z cronu nebo s odkazem z HTML je vstupní bod. Naslepo
se rozbijí reference.

### `.inc` chrání výhradně webserver

Apache soubory `.inc` **servíruje jako prostý text** — PHP je nespouští.
Strážce typu `if (!defined('APP')) exit;` na prvním řádku je proto
u `.inc` k ničemu: vypíše se jako obyčejný řádek spolu se zbytkem kódu.

Ochrana tedy stojí a padá s direktivou na serveru. Nejlépe globálně,
nezávisle na `AllowOverride`:

```apache
# /etc/apache2/conf-available/security-inc.conf, pak a2enconf security-inc
<FilesMatch "\.inc$">
    Require all denied
</FilesMatch>
RedirectMatch 404 (?i)/\.(git|svn|hg|bzr)(/|$)
<FilesMatch "\.(bak|old|orig|save|swp|sql|dump|log|tar|tgz|zip)$|~$">
    Require all denied
</FilesMatch>
<FilesMatch "^\.">
    Require all denied
</FilesMatch>
```

**Po nasazení vždy ověř**, že soubor s přístupy vrací 403, ne 200:

```bash
curl -o /dev/null -w '%{http_code}\n' https://host/lib/inc/dbconfig.inc
```

Kde direktivu nasadit nejde (cizí hosting, `AllowOverride None`), je jediná
funkční náhrada dát souborům příponu `.php` — Apache je spustí místo
odeslání — a doplnit jim strážce, který přímé volání odmítne 404.

### Adresáře

| adresář | ochrana |
|---|---|
| `api/*.inc` | `<FilesMatch "\.inc$"> Require all denied` |
| `api/pages/` | `Require all denied` — čte je jen `index.php` |
| `api/bin/` | `Require all denied` + kontrola `PHP_SAPI !== 'cli'` |
| `api/data/` | `Require all denied`, vlastník www-data |

A žádné `.git` ve webrootu — `/.git/config` je jinak veřejně čitelný.

## Session

Token jde v hlavičce `X-App-Session`, **nikdy v URL**. V URL by skončil
v access logu, v `Referer` při odchodu na cizí web, v historii prohlížeče
a v cache proxy.

Demo váže token na `X-App-Serial`, takže odcizený token na jiném zařízení
neprojde.

Trade-off: token v `localStorage` není chráněný proti XSS tak jako
`HttpOnly` cookie. Protiváhou je důsledné `esc()` na výstupu a možnost
mít API na jiném hostu bez trápení s `SameSite`.

## Push kanály

Tři věci, každá nutná:

1. **Token kanálu je jednoúčelový a náhodný**, nikdy session.
   `EventSource` neumí vlastní hlavičky, takže token musí jít v URL —
   a tam session nepatří.
2. **Publikační endpoint jen z localhostu** (`allow 127.0.0.1; deny all`).
   Kdo může publikovat, může poslat libovolný příkaz do cizího prohlížeče.
   Tohle je nejcitlivější místo celého návrhu.
3. **Odběratel přes TLS.** Z https stránky prohlížeč spojení na http
   zablokuje jako mixed content.

## Workery

Argumenty se předávají dočasným souborem s právy 0600, ne přes `argv` —
`argv` vidí v `ps` každý uživatel stroje.

## Chybové hlášky

Při `FW_DEBUG` obsahují cestu a řádek. **Na produkci `FW_DEBUG = false`.**

## Kontrolní seznam endpointu

1. vstupy přes `req*()`, nikdy přímo `$_REQUEST`
2. sémantické kontroly → 400
3. session → 401
4. oprávnění → 403
5. teprve pak práce
6. výstup přes `esc()`
