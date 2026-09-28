# 10 — Bezpečnost

Nejúčinnější ochrana je synergie opatření při psaní kódu i při nasazení.
Celková bezpečnost systému se vždycky dělá **kombinací opatření** — těch
při psaní, ať už lidském nebo vibecodingem, i těch spojených s deploymentem.

## Zásadní a nekompromisní pravidla pro psaní endpointu — ať jsi člověk, nebo agent

Tato pravidla platí shodně na úrovni BFF i datového API. Celá bezpečnost
stojí na dodržení tohoto postupu. Nemáme kromě rozdělení na vrstvy nic
dalšího, co by při chybě požadavek zamítlo nebo alespoň havarovalo.

Všechny endpointy skutečně **musí začínat takto**, jinak hrozí zásadní
kompromitace celého systému. Žádná výjimka (kromě globální kontroly
session) není rozumná, i kdyby stokrát zpřehlednila kód. Opakuji:
**runtime bezpečnost nemá žádné jiné záchytné prvky.**

1. **veškeré** vstupy přes `in_*()`, nikdy přímo `$_REQUEST`, vždy první řádky endpointu
2. s pečlivostí **unit testů** sémantické kontroly pro všechny myslitelné kombinace vstupů → 400, následující řádky endpointu
3. session → 401, až poté kontrola session (pokud není v modulu prováděna globálně, před vstupem do kódu endpointu). Je to dražší operace než prosté PHP, proto stojí až za kontrolou sémantiky. Má to i částečný přínos při DDoS: nesmyslné požadavky nevedou na amplifikační útok proti Redisu, databázi a podobným věcem v pozadí systému.
4. oprávnění → 403, nejdřív musí být žadatel autentizován, proto až nyní může být autorizován.
5. teprve pak práce
6. výstup přes `esc()`

### Základní pravidlo

**„Bezpečné" není vlastnost hodnoty, ale dvojice hodnota + cíl.**

Nejde napsat funkci, po které je řetězec bezpečný pro všechno. Escapování
pro SQL, HTML, shell a filesystém se vzájemně vylučuje. Proto:

> Na vstupu **validuj a typuj**. Na výstupu **escapuj podle cíle**.

Kdyby `in_str()` escapovalo HTML rovnou, uloží se do databáze `O&#039;Brien`,
porovnání `$u === "O'Brien"` selže, `strlen` vrátí 12 místo 7, a proti
SQL injection to stejně neudělá nic — `1 OR 1=1` projde beze změny.
Přesně tohle byly `magic_quotes`, které PHP v 5.4 vyhodilo.

### Vstup: tři garance

```php
in_str($name, $max)   // skalární string, omezená délka, bez NUL
in_int($name)     // int
in_float($name)   // float
```

`in_str()` vrátí prázdný string, když místo hodnoty přijde pole. Bez toho by
`?user[]=x` shodil endpoint na PHP 8 fatální chybou — což je DoS na jeden
parametr.

Čísla není potřeba dál kontrolovat. Po `intval()` neexistuje řetězec,
který by nesl útok.

### Pole z formuláře

Tabulkový formulář posílá `fd[i][sloupec]` a checkboxy `vyber[]`.
Skalární gettery taková pole schválně nepustí, takže je na ně
`in_rows()` — a platí pro něj **tytéž tři garance**:

```php
foreach (in_rows('fd', 500) as $i => $row) {
    $id  = intval($row['id'] ?? 0);
    $txt = db_esc($row['name'] ?? '');
}
```

Propustí jen dvojúrovňové pole skalárů; hlubší zanoření, objekty ani
skaláry na první úrovni neprojdou. Stropy jsou dva, na počet řádků
i na délku hodnoty — bez nich by jediný požadavek uměl vyrobit
statisíce prvků, což je DoS na jeden parametr.

Dvě věci, na kterých se tu chybuje:

* **Klíče jsou taky vstup od klienta.** `$row['name']` je ošetřená
  hodnota, ale `$i` a jména sloupců přišly od útočníka stejně jako
  cokoli jiného. Do SQL ani do HTML nepatří bez ošetření — framework je
  nepřepisuje, protože jen volající ví, co znamenají.
* **Chybějící klíč není prázdná hodnota.** Řádek nemusí obsahovat
  všechny sloupce, takže `?? ''` tam patří vždycky.

### Výstup: escapuj u cíle

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

### Cesty

Traversal se neřeší kontrolou, ale konstrukcí:

```php
$function = in_str('function', 64);
if (!is_word($function)) throw_http_error(400, 'Neplatný název');
// od téhle chvíle je jisté, že tam není '/', '.' ani NUL
```

## Nezabezpečený deployment může jakékoli opatření v kódu vynulovat

Proč by se útočník namáhal s kódem, když si může rovnou stáhnout konfiguraci i s hesly? Tahle třída přehmatů je pořád nejčastější.

**Každá vrstva** patří na **vlastní site** — vlastní virtual host, vlastní docroot,
vlastní systémový účet, ideálně vlastní kontejner nebo virtuál.

| vrstva | kdo na ni smí | co v docrootu je |
|---|---|---|
| frontend | kdokoli z internetu | statické soubory; PHP ani dynamické stránky tam nemusí běžet vůbec. Může být bez úprav na jakémkoli cloudu, s přidanou ochranou proti DDoS. |
| BFF | jen prohlížeč | dispatcher, fragmenty, konfigurace BFF. Může na cloud, pokud se vyřeší sdílená cache sezení. Dalšími opatřeními se dá provázat s ochranou frontendu proti DDoS. |
| datové API | **jen BFF** | dispatcher, přístup k databázi; může být schované v DMZ |

**Datové API nemusí být z internetu dostupné vůbec.** Ať poslouchá na
localhostu nebo ve vnitřní síti či DMZ; BFF — ať už je na témže stroji,
nebo za firewallem — víc nepotřebuje.
Veřejná adresa se hodí až ve chvíli, kdy API
používá i něco jiného než váš BFF — nativní aplikace pro iOS a Android,
integrace partnera, jiný systém. Pak ale platí, že si takový klient řeší
autentizaci sám a chodí na tytéž endpointy se stejnou kontrolou oprávnění.

Co tím získáte, se kódem zařídit nedá:

* Kdo prolomí frontend nebo BFF, stejně se k datům nedostane přímo. Musí
  projít endpointy datového API, a ty kontrolují oprávnění nezávisle na
  tom, co si o nich myslí BFF. Kompromitace BFF samozřejmě otevírá cestu
  k útokům typu man in the middle a podobně — proto mluvíme o synergii.
* Chyba ve fragmentu nedosáhne na konfiguraci **sousední** vrstvy,
  protože ta leží v jiném docrootu, na který ten webserver nevidí.

Cena je tři virtual hosty, tři konfigurace a CORS mezi frontendem
a BFF. U nového projektu je to půlhodina a stojí to za to.

### Čím to ale není

Oddělení vrstev **nenahrazuje** nic z toho, co následuje. Ohraničuje
škodu, nezabraňuje jí. Když se konfigurace servíruje jako text, útočník
si přečte přístupy k databázi té vrstvy, na kterou dosáhl — a že jsou
sites tři, mu v tom nezabrání.

| opatření | co samo o sobě řeší |
|---|---|
| zákaz `.inc` na webserveru | **naprostá nutnost.** Bez něj je konfigurace veřejně čitelná. |
| oddělené vrstvy | ohraničí škodu na jednu vrstvu a odřízne přímou cestu k datům |
| konfigurace mimo docroot | vyřadí celou třídu chyb — není co špatně naservírovat |
| vlastní účet a pool na vrstvu | zabrání tomu, aby jedna vrstva četla soubory druhé |

Žádné z nich nestačí samo. Pořadí, ve kterém se vyplatí je zavádět, je
odshora dolů.

**Dvouvrstvý model tuhle vrstvu navíc nemá** — BFF sahá na data přímo,
stejně jako by to dělalo datové API, takže všechno pod ním visí jen na té
kázni při psaní kódu popsané výš. (Pokud si nepřibere ORM nebo podobnou
technologii, což jde proti naší filosofii.)

U převodu staršího monolitu je dvouvrstvý model legitimní volba, ale je
dobré vědět, co se za něj platí: kromě snížené bezpečnosti i nemožnost
postavit nad API další věci — MCP server pro AI, úplně jinou aplikaci a
tak dál. **Pro nové projekty proto důrazně doporučuji model třívrstvý.**

V `examples/library` je rozdíl vidět na dvou instalátorech:
`install.sh` rozkopíruje kód do tří docrootů a založí databázový účet bez
DDL, kdežto `install.php` nechá všechno pod jednou doménou — a instalace
pak nese červený odznak `UNSAFE_DEMO`, protože datové API je z internetu
dosažitelné.

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

Totéž pro nginx, kde se to píše do server bloku. Pozor na pořadí:
`location` s regulárním výrazem má přednost před prefixovým, takže
tenhle blok musí stát **před** tím, který posílá `.php` na PHP-FPM —
jinak se `.inc` nejdřív chytí jako PHP a direktiva se neuplatní:

```nginx
# .inc se nikdy neservíruje ani nespouští
location ~ \.inc$                                   { return 403; }

# fragmenty, workery a běhová data čte jen index.php
location ~ ^/(pages|bin|data)/                      { return 403; }

# zálohy, výpisy a tečkové soubory
location ~ \.(bak|old|orig|save|swp|sql|dump|log|tar|tgz|zip)$|~$ { return 403; }
location ~ /\.                                      { return 403; }

location ~ \.php$ {
    include snippets/fastcgi-php.conf;
    fastcgi_pass unix:/var/run/php/php8.4-fpm.sock;
}
```

Kde direktivu nasadit nejde (cizí hosting, `AllowOverride None`), je jediná
funkční náhrada dát souborům příponu `.php` — server je spustí místo
odeslání — a doplnit jim strážce, který přímé volání odmítne 404.

### Konfigurace mimo docroot

Zákaz `.inc` je nutnost, ale pořád je to **pravidlo, které se dá zapomenout
nasadit** — při stěhování na nový stroj, po upgradu, v novém virtual hostu.
U jednoho jediného souboru se té závislosti dá zbavit úplně: konfigurace
s přístupy k databázi nemusí v docrootu ležet vůbec.

```php
require '/etc/waggle/mujprojekt/bff.inc';   // PHP ano, webserver nikdy
```

Kdo nechce sahat na kostru, může nechat soubor na místě a udělat z něj
**zavaděč**:

```php
<?php  /* bff/config.inc — jediný řádek, žádné tajemství */
require '/etc/waggle/mujprojekt/bff.inc';
```

I kdyby se tenhle soubor jednou naservíroval jako text, útočník se dozví
cestu, ne heslo. Za tu jednu řádku to stojí.

Dvě věci k ověření: PHP musí na cestu dosáhnout (`open_basedir` bývá na
sdíleném hostingu nastavený a `/etc` v něm nebude), a soubor má patřit
účtu, pod kterým běží PHP-FPM, s právy `0600`.

### Na co se nespoléhat

**Symlink z docrootu ven není ochrana.** Nabízí se to — nechat
`bff/config.inc` jako odkaz na `/etc/waggle/bff.inc` — jenže jestli
webserver odkaz následuje, rozhoduje direktiva `Options FollowSymLinks`.
Když ji máte zapnutou — a v mnoha instalacích je zapnutá — server odkaz
následuje a **obsah cíle pošle**, jako by tam ležel. Když ji máte
vypnutou, odkaz odmítne 403; jenže to je pak zásluha direktivy, a kdo umí
nastavit tuhle, umí zakázat i `.inc`. Tak jako tak rozhoduje konfigurace
serveru, ne ten symlink. Nejhorší na tom je, že to vypadá jako opatření.

**Proměnné prostředí** (`env[]` v poolu PHP-FPM) docroot obcházejí, ale
heslo v nich je vidět ve `phpinfo()`, v `/proc/self/environ` pro kohokoli
na stroji a dědí se do všeho, co spustíte přes `exec()`. Jako náhrada
konfiguračního souboru to není lepší, jen jinak zranitelné.

**Práva souboru** `0600` chrání před ostatními účty na stroji, ne před
webserverem — ten běží pod týmž uživatelem jako PHP a soubor přečte.
Je to doplněk, ne alternativa.

### Co zamknout v každém docrootu

Cesty jsou vztažené k docrootu té které vrstvy — u BFF i u datového API
platí totéž.

| adresář | ochrana |
|---|---|
| `*.inc` | `<FilesMatch "\.inc$"> Require all denied` |
| `pages/` | `Require all denied` — čte je jen `index.php` |
| `bin/` | `Require all denied` + kontrola `PHP_SAPI !== 'cli'` |
| `data/` | `Require all denied`, vlastník www-data |

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
