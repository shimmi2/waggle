# 01 — Motivace a cíle

> Let's say adieu to the overlayered, fat and slow balls of dirt called high-level frameworks — and open a new era: agentic coding, top speed, safe and simple apps.

Waggle vznikl ze **dvou potřeb, které se sešly ve stejnou chvíli**. Každá
z nich by sama o sobě vedla k jinému nástroji; dohromady vedly k tomuhle.

## Cíl 1 — odpovědět na vibecoding

Kód dnes ve velkém píší jazykové modely. Tím se obrací ekonomika, na které
stály vysoké frameworky: historické ospravedlnění abstraktních vrstev
znělo *ručně psaný boilerplate je drahý, tak ho schováme.* Když je
generování boilerplatu levné a přečtení výsledku rychlé, obchod se obrací
— explicitní, dohledatelný, grepovatelný kód vyhrává nad chytrou
abstrakcí.

Zásluhu na tom ale nemá samotný model. Model generuje **věrohodný** kód,
ne nutně **konzistentní**. Čtyřicet endpointů napsaných bez specifikace je
čtyřicet mírně odlišných endpointů, a to je horší než vrstva, protože
nekonzistenci nikdo nevynucuje.

Použitelnou strategií dělá generování až **napsaná, úzká a vynucená
konvence**: pevný protokol, pevná kostra endpointu, pevná sada vstupních
funkcí, pevná sada příkazů. Pak je generování rychlé vyplňování známé
šablony a výsledek se dá zkontrolovat pohledem.

Proto je tahle dokumentace zároveň **zadáním pro generátor**.
[07 — Aplikace](07-aplikace.md) obsahuje šablonu endpointu, kterou lze
předat člověku i modelu se stejným výsledkem. Skilly v `.claude/skills/`
jsou z téhož důvodu součástí repozitáře, ne přílohou.

Druhý důsledek je praktický: na jedné úrovni se pracuje stylem **jeden
prompt = jedna nová vlastnost**. Kdo trvá na vrstvách, narazí na limit
kontextu mnohem dřív, protože jedna změna se dotkne pěti souborů ve čtyřech
jazycích.

## Cíl 2 — co s dvacet let starými aplikacemi

Druhá potřeba je konkrétní a stará: PHP aplikace, kde každé tlačítko a
každý odkaz odesílá formulář a vyvolává kompletní reload stránky. Funguje
to. Ale je to pomalé, bliká to, ztrácí se pozice ve scrollu i rozepsaný
obsah polí — a s propracovanou šablonou jako AdminLTE se při každém
kliknutí zbytečně přenáší a znovu inicializuje celý layout.

Obvyklá odpověď zní „přepiš to do Vue". To znamená build step,
`node_modules`, druhý jazyk, zdvojenou logiku na obou stranách a rewrite
na rok.

Waggle je jiná odpověď: **nechat serverovou logiku tam, kde je, a vyměnit
jen způsob doručení.** Existující stránka se převede přidáním atributu, ne
přepsáním:

```html
<a href="#?function=detail&id=1">Detail</a>
<a href="#?function=detail&id=1" data-fw>Detail</a>
```

Endpointy zůstávají v PHP, generují HTML jako dosud, jen ho posílají jako
příkaz místo celé stránky. Postup po krocích je v
[09 — Migrace](09-migrace.md).

Migrace přitom není strop. Streamované odpovědi, push ze serveru,
synchronizace mezi okny a jemné adresování prvků jsou věci, které většina
SPA frameworků řeší složitěji a dráž — takže tentýž základ nese i aplikace
psané od nuly.

## Proč zrovna „Waggle"

Včelí *waggle dance* je jediný způsob, jak si včely předají, kam letět.
Tanec nese směr, vzdálenost i kvalitu zdroje — a je to **příkaz**, ne data
k interpretaci. Včela ho nedostane jako JSON, který si musí sama
vyrenderovat.

Stejnou roli hraje protokol Waggle: server pošle hotový příkaz, klient ho
provede. Žádné schéma, žádná zdvojená logika na obou stranách, žádný
model, který se musí držet v synchronu.

Úl k tomu nemá architekta ani vysoký framework. Má jednoduchá pravidla a
hodně dělníků — dnes stále častěji agentů.

## Čemu se framework vyhýbá

| vyhýbá se | důvod |
|---|---|
| build stepu | zdroják je to, co běží; ladí se bez source map |
| `node_modules` | nulová údržba závislostí, žádný supply chain |
| klientskému routeru | URL řeší příkaz `history`, stav drží server |
| šablonám na klientovi | HTML skládá BFF — mezivrstva, která zná aplikační logiku |
| ORM | viz následující kapitolu |

## Bez ORM: co se získá a co to stojí

V tomhle architektonickém stylu jdou data cestou
**DB → API → BFF → HTML**:

| vrstva | co dělá |
|---|---|
| **DB** | drží data |
| **API** | zpřístupňuje je — a to je přesně ta role, kterou by jinak hrálo ORM |
| **BFF** | aplikační logika a skládání HTML |
| **frontend** | vykresluje a reaguje na události |

Referenčním vzorem je `examples/library`, kde jsou ty vrstvy oddělené
doopravdy, každá ve své doméně. Zjednodušená integrace, kde BFF sahá na
data samo a API jako samostatná vrstva chybí, je taky legitimní — u
převodu staršího monolitu obvykle jiná cesta ani nedává smysl.

Data se na téhle cestě nikdy nestanou doménovým objektem. ORM je stroj na
to, aby z řádků udělal objekty, které se jednou vykreslí a zahodí;
v aplikaci, která data používá právě jednou a právě k vykreslení, je to
režie navíc — hydratace, lazy loading s překvapeními, N+1 dotazy a učení
se query jazyku místo SQL. Ten argument platil před jazykovými modely a
platí i po nich.

Nevýhody té volby jsou ale reálné a je lepší je znát předem.

**Evoluci schématu framework neřeší.** ORM s sebou obvykle nese nástroj
na migrace databáze; tady žádný není. Za tu ztrátu ale platí málokdo tolik,
kolik to na první pohled vypadá: u větších projektů byla automatická
evoluce spíš teorie než praxe. Netriviální změna — rozdělení sloupce,
přepočet historických dat, změna významu příznaku — se stejně dopisovala
ručně a nástroj u ní posloužil nanejvýš jako evidence, co už proběhlo.

V době agentického kódování se navíc posouvá i ta zbývající část: migrační
skript umí napsat agent z popisu změny a ze schématu, které má před sebou.
Co zůstává na lidech, je **spustit ho bezpečně** — zálohou, zkouškou na
kopii a s plánem, jak se vrátit zpátky. To za vás nevyřeší ORM ani agent.

**Bezpečnost nedrží vrstva, ale autor.** ORM parametrizuje dotazy, ať píše
kdokoli. Generovaný a ručně psaný kód je bezpečný jen tak, jak pozorný je
ten, kdo ho čte. Odpověď frameworku není vrstva, ale **zkrácení bezpečné
cesty**: `in_int()` je kratší než sáhnout do `$_REQUEST` a přetypovat,
`is_word()` je kratší než ruční kontrola cesty, `esc()` je kratší než
`htmlspecialchars()` se třemi argumenty. Podrobně v
[10 — Bezpečnost](10-bezpecnost.md).

**ORM tím nekončí a končit nemá.** Zapomenuté `WHERE` odchytí ORM, tady
ho neodchytí nic. Identity map, unit of work a transakční hranice jsou
mechanismy, které ORM řeší dobře a tenhle styl vůbec — a u složitých
operací nad více tabulkami, u přesných transakcí a všude, kde se pracuje
s penězi, si své místo drží dál. Tahle kapitola není argument proti ORM,
je to popis toho, kdy se nevyplatí.

Protože v aplikaci typu „formulář, seznam, detail" se ty mechanismy
neuplatní nikdy. Rozhodovací pravidlo je jednoduché: **kolik agend
v systému sahá na jednu tabulku?** Když většina, ORM platíte a
nevyužijete — a ničemu nebrání použít ho jen tam, kde se opravdu hodí.

## Co framework záměrně neřeší

Databázi, šablonovací jazyk, autentizaci, routing na serveru, validaci,
lokalizaci. To všechno je věc projektu. Framework dělá jedno: **doručí
příkaz ze serveru do DOMu, čtyřmi transporty a jedním formátem.**

Není to díra, je to **dělba práce**. Co dřív držela vrstva technologie,
drží teď konvence a skilly. Konvenci lze přečíst za deset minut, nedrží se
v paměti procesu, nepřidává round-trip a nezastará s příští major verzí —
a dá se předat agentovi stejně dobře jako člověku, což se o vrstvě říct
nedá.

Hotové stavební díly nad rámec knihovny se chystají odděleně, jako
příklady a komponenty. Viz [14 — Co je v plánu](14-plan.md).

## Na čem stojí a na čem nesmí stát

Tohle je hranice knihovny a drží se **tvrdě**. Je snadné ji rozmělnit
jedním „to se přece hodí", a pak už se to nevrátí.

**Tvrdá závislost je jediná: HTML, CSS a JavaScript v prohlížeči.** Nic
dalšího `fw.js` nepotřebuje — žádný build step, žádný balíčkovač, žádnou
knihovnu třetí strany.

**Volitelná je jedna: nchan.** Kvůli asynchronnímu doručování obecně —
tedy všude, kde server potřebuje promluvit dřív, než se ho někdo zeptá.
Průběh dlouhé operace je jen ten nejviditelnější případ; stejně tak z něj
žijí živé monitoringy, hlášení o události, kterou spustil někdo jiný, nebo
data, která se mění sama od sebe. Bez nchanu funguje všechno ostatní, jen
se tyhle věci doručí streamem, nebo vůbec.

Co do knihovny **nepatří a patřit nebude**:

| | proč to není závislost |
|---|---|
| AdminLTE, Bootstrap, jakákoli šablona | `examples/app-adminlte/` je **příklad**, ne součást. `examples/app/` dokazuje, že to jde i bez nich. |
| PHP | `fw.inc` a `io.inc` jsou **referenční implementace** serverové strany, ne její definice. Definicí je [04 — Protokol](04-protokol.md). Přepsat je do Pythonu nebo Go je práce na den. |
| konkrétní projekty autora | Jejich knihovny zůstávají u nich. Do repozitáře se nikdy nedostane nic, co ví, jak vypadá cizí databáze. |

Zbytek obsahu repozitáře jsou **příklady a nástroje**, ne knihovna:
`examples/*`, `tools/fwdeploy.sh`, `tools/async_sender.php`. Nástroj smí
být v PHP, smí předpokládat nchan, smí si dělat, co chce — protože ho
nikdo nemusí použít. Knihovna ne.

Zkouška, která to rozsoudí: **co přestane fungovat, když ta věc zítra
zmizí?**

* zmizí AdminLTE → jen `examples/app-adminlte/`
* zmizí PHP → serverová strana se přepíše, protokol platí dál
* zmizí nchan → jen push; fetch i stream jedou

Když je odpověď „framework", je to závislost a nepatří tam.

**Jedno místo tuhle čistotu dnes porušuje**, ať se na to nepřijde jako na
překvapení: výchozí překryv operace `busy` má `z-index: 1050`, což je
bootstrapí číslo pro pozadí modálu. Funguje to i jinde, ale je to hodnota
převzatá z cizí knihovny. Správně by to měla být proměnná s touhle výchozí
hodnotou.
