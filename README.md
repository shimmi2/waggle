# Waggle

> Let's say adieu to the overlayered, fat and slow balls of dirt called
> high-level frameworks — and open a new era: agentic coding, top speed,
> safe and simple apps. Apps where data flows in parallel, asynchronous
> motion, like bees in a waggle dance.

Vydání **1.3.0**, protokol **v1**.

## Proč vznikl

* **Nemám rád výsledky, které z projektů udělají frameworky typu
  Symfony.** Naházíte do projektu všechny bundly, které byste mohli
  potřebovat, a pak po každém kliknutí několik vteřin čekáte.
  Ze zamrzlého prohlížeče vypadne pět položek, ta hledaná mezi nimi
  není, kliknete na další stránku a čekáte zas. Upravíte filtr
  a čekáte znovu.

* **Pamatuju dobu, kdy se do prohlížeče natáhlo tisíc položek za půl
  vteřiny.** Stačilo k tomu SQL, Redis a pár dalších zdrojů dat. Tu
  dobu chci zpátky.

* **Nemám rád ani mnoho vrstev v kódu.** Za o třicet procent lepší
  přehlednost se platí třeba Twigem — mezivrstvou, kterou stejně
  generujete z vrstev nad ní. Je hloupé nechat interpretovaný jazyk,
  PHP nebo Python, generovat tutéž stránku dvakrát: dnes, zítra,
  tisíckrát denně, stejně tak za rok. Pak se přidá ORM, navržené pro
  situaci, kdy jsou všechna data z mnoha zdrojů na jednom místě. Jaký
  to skvělý nápad. A i když pominu, kolikrát se ta data musí
  interpretovaným jazykem překopírovat, za ten výkřik techniky stejně
  zaplatíte horší optimalizací dotazů. U většího projektu navíc
  zjistíte, že část operací je nepoužitelná a ORM stejně musíte
  obcházet — čímž dokonale obejdete i jeho smysl. A když se rozhodnete
  vyměnit MySQL za PostgreSQL, ukáže se, že to nejde, a vzdáte to.

  Mají i výhody: větší bezpečnost a odolnost proti chybám v kódu —
  nestane se tak snadno, že zapomenete `WHERE`. Proto i nadále patří
  tam, kde se pracuje s penězi a přesnými transakcemi. Jenže
  v průměrném systému je osmdesát procent agend nad jedinou tabulkou.

* **Agentické kódování to staví do úplně jiného světla.** Pravidla pro
  týmovou práci patří do projektových a firemních skillů; technologie
  na jejich vynucení není potřeba. Kdo na vrstvách trvá, narazí na
  limit tokenů desetkrát dřív, zatímco na jedné úrovni se dá pracovat
  stylem **jeden prompt = jedna nová vlastnost**. Kdo mi nevěří, ať si
  tenhle soubor otevře za dva roky.

Proto předkládám Waggle: jednoduchý asynchronní framework prakticky bez
závislostí, zaměřený na dvě věci.

* Rychlé a bezpečné agentické kódování stylem jeden prompt = jedna nová
  vlastnost.
* Snadný převod starých systémů, které při každém kliknutí reloadovaly
  celou stránku — a to zase agentem.

## Proč Waggle

Waggle je systém asynchronních včeliček. Nezávisle na sobě létají z úlu
na pastvu, vracejí se a tančí svůj waggle. Ten tanec **je** ten příkaz
— směr, vzdálenost, cíl — a dohromady z nich vzniká celek. Včely se
neptají jedna druhé a nečekají na sebe. Dostanou instrukci a jednají.

Vstup do úlu je přitom pevně bráněn a žádná cizí včela neprojde.

Přesně tohle dělá Waggle mezi serverem a prohlížečem.

## Jak to funguje

Tenký klient má HTML a CSS šablony, bootstrap v JavaScriptu a svůj kód.
Povinné to není, ale většině firemních aplikací vyhoví základní
rozdělení na **levé menu, horní rám a hlavní okno**.

Klient posílá požadavky. Server posílá **příkazy** a s nimi hotové kusy
co nejabstraktnějšího HTML. (Vím, že se leckomu při téhle větě otevírá
kudla v kapse. Vydržte — důležitý je účel, bezpečnost a funkce, ne
ideály.) Klient ty příkazy aplikuje na DOM.

Žádný build step, žádné závislosti, žádný SPA router, žádný Vue ani
React.

Referenční implementaci serveru přikládám v PHP, ale nic nebrání
přepsat ji jedním promptem do Pythonu nebo čehokoli dalšího — definicí
je [protokol](docs/03-protokol.md), ne ten soubor. A možné je to jen
díky té jednoduchosti: minimu vrstev a nezávislosti na milionu
nástrojů.

## ORM a další vrstvy nahrazují skilly

Stojí to na několika pravidlech.

**Frontend** je o volbě šablony: AdminLTE, Tabler, CoreUI. V příkladech
je i varianta na čistém HTML a CSS.

**Backend for frontend** je o stavovém modelu aplikace, session,
oprávněních první vrstvy a o kódu. Platí pro něj tohle:

* Je to jeden nebo několik API modulů na serveru či v cloudu, vhodně
  rozdělených — `auth/…`, `user/…`.
* Každý modul je „jeden velký switch" s endpointy: `auth/login`,
  `auth/change_password`, `my_agenda/dashboard`.
* **Každý endpoint musí řešit bezpečnost v tomhle pořadí.** Není to
  doporučení, je to podmínka — viz [06 — Aplikace](docs/06-aplikace.md)
  a [09 — Bezpečnost](docs/09-bezpecnost.md):

    1. **Import vstupů.** První řádky obalí všechno, co přišlo zvenčí:
       `req()`, `req_int()`, `req_float()`.
    2. **Sémantika.** Levné kontroly bez sahání do databáze:
       `if (!$id) throw_http_error(400, 'Chybí id');`
    3. **Session**, tedy autentizace — pokud ji modul neřeší globálně:
       `if ($user === null) throw_http_error(401, 'Nejste přihlášen');`
       Tohle pořadí mimochodem lépe chrání proti primitivnímu DDoS:
       útočník se nedostane k ničemu drahému.
    4. **Oprávnění k endpointu:**
       `if (!acl_check($user, 'delete_users')) throw_http_error(403, 'Nemáte oprávnění');`
    5. **Vlastní práce.** Přečte data, udělá, co má. V třívrstvém modelu
       volá backendové API, které má vlastní kontrolu oprávnění.
       Ve dvouvrstvém vám stačí SQL, Redis, InfluxDB a další místa, kde
       data leží. Na vznosné ideály máte agenty, ne ORM.

* Ve **třívrstvém** modelu pracuje třetí vrstva stejně. Liší se jen
  tím, že vrací atomická čistá data, jak se na API sluší, a může být
  společná i pro jiné klienty — třeba aplikace pro Android a iOS.
  Nemívá stavový model a neřeší HTML fragmenty. Klidně to může být
  existující API. Ale kdo Waggle napojí na líné endpointy od Symfony,
  jde z deště pod okap.
* Často dostávám dotaz na **jemné škálování oprávnění v odpovědi**.
  Není to problém:

  ```php
  if (!acl_check($user, 'money_boss')) unset($result['real_expenses']);

  foreach ($polozky as $p) {
      if (!$p['confirmed'] && !acl_check($user, 'see_unconfirmed')) continue;
      $result[] = $p;
  }
  ```

* Kde backendové API vrací datový JSON, tam backend for frontend vrací
  **sadu příkazů Waggle**.
* Sada příkazů je jedna nebo víc instrukcí typu „do hlavního okna dej
  podokna `goods_filter` a `goods_results`". Vyvolá ji třeba kliknutí
  na položku menu, která pošle do BFF `get_goods`; odpovědí je ta sada.
* Na úrovni frameworku je podporovaná operace
  [`busy`](docs/03-protokol.md), takže jde triviálně zobrazit
  „analyzuji… 10 %" a pak poslat data. Překryv framework uklidí sám,
  jakmile do téhož okna dorazí výsledek.

## Tři vrstvy

Repozitář má tři vrstvy, které se chovají úplně jinak. Než z něj začneš
něco brát, koukni, do které patří — ušetří to spoustu zbytečných otázek.

| vrstva | co to je | jak často se bere |
|---|---|---|
| **knihovna** | `fw.inc`, `fw.js` | **průběžně**, skriptem `tools/fwdeploy.sh` |
| **kostra** | `api/index.php`, `api/config.inc`, `api/inc/*` | **jednou** při zrodu projektu, pak se rozchází |
| **dema** | `api/pages/*`, `app/*`, `app2/*` | **nikdy** — jen se čtou |

**Knihovna jsou dva soubory.** V reálném projektu je to zlomek celku:
`fw.inc` má 10 kB proti stovkám kB stránek, `fw.js` 20 kB proti
megabajtům šablony. Nikdy se needituje v projektu — každá změna patří
do nové verze Waggle.

**Kostra se rozchází schválně.** Dispatcher demo API má 8 kB, oba
odvozené převedením portálu, zkušebního projektu, kolem 6 kB.

**Dema jsou referenční text, ne startovací balík.** `app/` a `app2/` jsou
tatáž aplikace jednou na holém HTML a jednou na AdminLTE — jsou tu, aby
bylo vidět, že markup je jediné, co se mezi nimi liší.

```
fw.js       knihovna, klient
fw.inc      knihovna, server (referenční implementace v PHP)
api/        kostra + dema, jedno API pro oba příklady
app/        příklad BEZ AdminLTE — holé HTML a vlastní CSS
app2/       příklad S AdminLTE 4
docs/       dokumentace, 11 kapitol
doc/        generátor prohlížitelné dokumentace
tools/      rozvoz knihovny do projektů + odesílač do prohlížeče
.claude/    skilly pro práci s Waggle
nginx-nchan.conf.example
```

## Rychlý start

```html
<div id="left_menu"></div><div id="top_frame"></div><div id="main"></div>
<script src="../fw.js"></script>
<script>Fw.init({ api: '../api/' });</script>
```

```php
require __DIR__ . '/config.inc';
require __DIR__ . '/../fw.inc';
fw_boot();

send_answer([
    ['op' => 'html', 'sel' => 'main',      'content' => '<h1>Ahoj</h1>'],
    ['op' => 'html', 'sel' => 'top_frame', 'content' => 'Přihlášen'],
]);
finish_answer();
```

Odkaz, který to zavolá bez reloadu stránky:

```html
<a href="#?function=index&id=42" data-fw>Detail</a>
```

## Dva příklady, jedno API

`app/` a `app2/` jsou tatáž aplikace. Jedna na holém HTML, druhá na
AdminLTE 4. Jedou přes **jedno** `api/` a liší se **výhradně markupem
fragmentů** — endpointy, session ani protokol se neliší ani o řádek.

Skin posílá aplikace v každém požadavku:

```js
Fw.init({ api: '../api/', params: { skin: 'lte' } });
```

```php
$skin  = req('skin', 8) === 'lte' ? 'lte' : 'plain';
$PAGES = __DIR__ . '/pages/' . $skin;
```

Proto jde přidat třetí skin bez jediného zásahu do logiky. Je to zároveň
odpověď na otázku „musím kvůli frameworku používat AdminLTE" — nemusí se
používat vůbec nic.

Přihlášení do dem: `tomas` / `heslo`. Před ostrým nasazením změň
`AUTH_SECRET` v `api/config.inc` a `FW_DEBUG` přepni na `false` —
s `true` chybová hláška prozradí cesty na disku.

## Dokumentace

Prohlížitelná verze: **[`doc/`](doc/index.html)** — jedna stránka, generuje
se z `docs/*.md` příkazem `php doc/build.php`.

| | |
|---|---|
| [01 — Motivace a cíle](docs/01-motivace.md) | proč vznikl, čemu se vyhýbá, vztah k ORM a generovanému kódu |
| [02 — Principy](docs/02-principy.md) | dvanáct pravidel, na kterých celý návrh stojí |
| [03 — Protokol](docs/03-protokol.md) | obálka, transporty, úplná reference příkazů |
| [04 — Klient](docs/04-klient.md) | `fw.js` — API, události, transporty, hooky |
| [05 — Server](docs/05-server.md) | `fw.inc` — vstupy, fronta odpovědí, chyby, stream, push |
| [06 — Aplikace](docs/06-aplikace.md) | struktura demo API, **šablona endpointu**, skiny |
| [07 — Integrace](docs/07-integrace.md) | AdminLTE a obecný postup pro jakoukoli šablonu |
| [08 — Migrace](docs/08-migrace.md) | kuchařka pro převod starého projektu |
| [09 — Bezpečnost](docs/09-bezpecnost.md) | vstupy, escapování, cesty, tokeny, oprávnění |
| [10 — Nasazení](docs/10-nasazeni.md) | Apache, nginx, nchan, produkční checklist |
| [11 — Problémy](docs/11-problemy.md) | pasti, na které jsme narazili, a jak je poznat |

## Skilly

`.claude/skills/` obsahuje dva návody psané pro jazykové modely — a
mimochodem i pro lidi, protože je to prostě sepsaná zkušenost:

* **`waggle-page`** — jak napsat endpoint a fragment, v jakém pořadí, co
  framework garantuje a co ne, a nejčastější pasti
* **`waggle-migrate`** — převod starého projektu a katalog jedenácti pastí
  starého PHP na osmičce, z reálných nálezů

Úl nemá architekta. Má jednoduchá pravidla a hodně dělníků — dnes stále
častěji agentů. K tomu není potřeba vysoký framework, ale sada skillů,
která drží jednotný přístup. Proto jsou tady, ne jako příloha.

## Rozvoz do projektů

Projekty si nesou **vlastní kopii** `fw.inc` a `fw.js`, ne symlink. Chyba
v `fw.inc` znamená, že projekt nejede vůbec; se symlinkem by šla do všech
projektů naráz a nedalo by se nasazovat po jednom. S kopií je v gitu
každého projektu vidět, na jaké verzi frameworku běží.

```bash
./tools/fwdeploy.sh --check            # co kde běží
./tools/fwdeploy.sh <cesta>...         # rozvoz z tohohle stromu
./tools/fwdeploy.sh --from v1.3.0 …    # rozvoz z vydání na GitHubu
```

Bez `--from` se bere tenhle strom, takže to jede i bez sítě. S `--from`
se stáhnou oba soubory z tagu nebo větve — tím se dá rozvážet i na
server, kde Waggle nemá pracovní kopii. Stažený soubor se před přepsáním
kontroluje: když v něm není `FW_RELEASE`, skript skončí a nesáhne na nic.
Useknutý download nemá jak shodit běžící projekt.

Composer ani npm schválně ne. Kvůli dvěma souborům by přibylo `vendor/`
s autoloadem nebo `node_modules` — tedy přesně ta sněhová koule, kterou
podtitul posílá k šípku.

Seznam projektů může být v `tools/targets.local` (mimo git, je to místní
věc). Skript nikdy nezakládá soubor, který v cíli ještě není — jinak by
překlep v cestě vyrobil nový soubor místo hlášky.
