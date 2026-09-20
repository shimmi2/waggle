# Framework

Tenký klient nad PHP. Server posílá **příkazy** a hotové kusy HTML, klient je
aplikuje na DOM. Žádný build step, žádné závislosti, žádný SPA router,
žádný Vue ani React.

Vznikl kvůli migraci dvacet let starých PHP projektů, které při každém kliknutí
reloadovaly celou stránku. Ale není to berlička pro staré kódy — je to
kompletní základ i pro nové aplikace.

Vydání **1.0.0**, protokol **v1**.

```
fw.js       framework, klient
fw.inc      framework, server
docs/       dokumentace, 11 kapitol
doc/        generátor prohlížitelné dokumentace
app/        příklad BEZ AdminLTE — holé HTML a vlastní CSS
app2/       příklad S AdminLTE 4
api/        jedno API pro oba příklady
tools/      rozvoz frameworku do projektů
.claude/    skilly pro práci s frameworkem
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

* **`fw-page`** — jak napsat endpoint a fragment, v jakém pořadí, co
  framework garantuje a co ne, a nejčastější pasti
* **`fw-migrate`** — převod starého projektu a katalog jedenácti pastí
  starého PHP na osmičce, z reálných nálezů

Framework, jehož konvence se dají předat jedním souborem, se dá předat
i modelu se stejným výsledkem jako člověku. To je záměr, ne dodatek.

## Rozvoz do projektů

Projekty si nesou **vlastní kopii** `fw.inc` a `fw.js`, ne symlink. Chyba
v `fw.inc` znamená, že projekt nejede vůbec; se symlinkem by šla do všech
projektů naráz a nedalo by se nasazovat po jednom. S kopií je v gitu
každého projektu vidět, na jaké verzi frameworku běží.

```bash
./tools/fwdeploy.sh --check          # co kde běží
./tools/fwdeploy.sh <cesta>...       # rozvoz, ptá se na každý soubor
```

Seznam projektů může být v `tools/targets.local` (mimo git, je to místní
věc). Skript nikdy nezakládá soubor, který v cíli ještě není — jinak by
překlep v cestě vyrobil nový soubor místo hlášky.
