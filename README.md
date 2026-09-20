# Waggle

> Let's say adieu to the overlayered, fat and slow balls of dirt called high-level frameworks — and open a new era: agentic coding, top speed, safe and simple apps.

Včela, která najde pastvu, se vrátí do úlu a tančí. Ten tanec **je** ten
příkaz — směr, vzdálenost, kvalita. Ostatní včely se neptají na schéma
a nedělají dotaz do databáze. Dostanou instrukci a jednají.

Přesně tohle dělá Waggle mezi serverem a prohlížečem.

Tenký klient nad PHP. Server posílá **příkazy** a hotové kusy HTML, klient je
aplikuje na DOM. Žádný build step, žádné závislosti, žádný SPA router,
žádný Vue ani React.

Vznikl kvůli migraci dvacet let starých PHP projektů, které při každém kliknutí
reloadovaly celou stránku. Ale není to berlička pro staré kódy — je to
kompletní základ i pro nové aplikace.

Vydání **1.0.0**, protokol **v1**.

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
sem a rozveze se.

**Kostra se rozchází schválně.** Dispatcher demo API má 8 kB, oba
odvozené portály kolem 6 kB. To není rozjetí, které by se mělo srovnat;
je to projekt, který si vzal, co potřeboval, a zbytek zahodil.

**Dema jsou referenční text, ne startovací balík.** `app/` a `app2/` jsou
tatáž aplikace jednou na holém HTML a jednou na AdminLTE — jsou tu, aby
bylo vidět, že markup je jediné, co se mezi nimi liší.

```
fw.js       knihovna, klient
fw.inc      knihovna, server
api/        kostra + dema, jedno API pro oba příklady
app/        příklad BEZ AdminLTE — holé HTML a vlastní CSS
app2/       příklad S AdminLTE 4
docs/       dokumentace, 11 kapitol
doc/        generátor prohlížitelné dokumentace
tools/      rozvoz knihovny do projektů
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
./tools/fwdeploy.sh --from v1.0.0 …    # rozvoz z vydání na GitHubu
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
