# 01 — Motivace a cíle

> Let's say adieu to the overlayered, fat and slow balls of dirt called high-level frameworks — and open a new era: agentic coding, top speed, safe and simple apps.

## Proč Waggle

Včelí *waggle dance* je jediný způsob, jak si včely předají, kam letět.
Tanec nese směr, vzdálenost i kvalitu zdroje — a je to **příkaz**, ne
data k interpretaci. Včela ho nedostane jako JSON, který si musí sama
vyrenderovat.

Stejnou roli hraje protokol Waggle: server pošle hotový příkaz, klient
ho provede. Žádné schéma, žádná zdvojená logika na obou stranách, žádný
model, který se musí držet v synchronu.

A je v tom i druhá půlka. Úl nemá architekta ani vysoký framework. Má
jednoduchá pravidla a hodně dělníků — dnes stále častěji agentů. K tomu
není potřeba katedrála, ale sada skillů, která drží jednotný přístup.
Proto jsou v `.claude/skills/` součástí repozitáře, ne přílohou.

## Odkud to vzešlo

Dvacet let staré PHP aplikace, kde každé tlačítko a každý odkaz odesílá
formulář a vyvolává kompletní reload stránky. Funguje to. Ale je to pomalé,
bliká to, ztrácí se pozice ve scrollu a rozepsaný obsah polí — a s propracovanou
šablonou jako AdminLTE se při každém kliknutí zbytečně přenáší a znovu
inicializuje celý layout.

Obvyklá odpověď je „přepiš to do Vue". To znamená build step, node_modules,
druhý jazyk, zdvojenou logiku na obou stranách a rewrite, který trvá rok.

Tenhle framework je jiná odpověď: **nechat serverovou logiku tam, kde je,
a vyměnit jen způsob doručení.**

## Dva cíle, ne jeden

**1. Evoluční migrace.** Existující stránka se převede přidáním atributu, ne
přepsáním. `<a href="#?function=aaa&id=1">` → `<a href="#?function=aaa&id=1" data-fw>`.
Endpointy zůstávají v PHP, generují HTML jako dosud, jen ho posílají jako
příkaz místo celé stránky. Viz [08 — Migrace](08-migrace.md).

**2. Plnohodnotný základ pro nové aplikace.** Tohle není berlička. Streamované
odpovědi, push ze serveru, synchronizace mezi okny a jemné adresování prvků
jsou věci, které většina SPA frameworků řeší složitěji a dráž.

## Čemu se vyhýbá a proč

| vyhýbáme se | důvod |
|---|---|
| build step | zdroják = to, co běží; debugování bez source map |
| node_modules | nulová údržba závislostí, žádný supply chain |
| klientskému routeru | URL řeší `history`, stav drží server |
| šablonám na klientovi | HTML generuje ten, kdo má data |
| ORM | viz níže |

## O ORM a generovaném kódu

Ptal ses, jestli sdílím názor, že ORM je zbytečnost, protože AI dokáže
napsat standardizovaný kód pro každou komponentu zvlášť.

**Se závěrem souhlasím, s odůvodněním jen zčásti** — a ten rozdíl je důležitý,
protože z něj plyne, jak psát tuhle dokumentaci.

### Proč tu ORM nepatří, nezávisle na AI

V tomhle architektonickém stylu jdou data cestou **databáze → HTML**.
Nikdy se nestanou doménovým objektem. ORM je stroj na to, aby z řádků udělal
objekty, které pak něco vyrenderuje a zahodí. V aplikaci, která ta data
používá právě jednou a právě k vykreslení, je to čistá režie — hydratace,
lazy loading s překvapeními, N+1 dotazy a učení se query jazyku místo SQL.

Tenhle argument platil před AI a bude platit po ní.

### Kde AI opravdu mění rovnici

Historické ospravedlnění abstraktních vrstev znělo: *ručně psaný boilerplate
je drahý, tak ho schováme.* Když je generování boilerplatu levné a
přečtení výsledku rychlé, obchod se obrací — explicitní, dohledatelný,
grepovatelný kód vyhrává nad chytrou abstrakcí. V tom máš pravdu.

### Ale zásluhu nemá AI, má ji úzká konvence

Tohle je ten rozdíl. AI generuje **věrohodný** kód, ne nutně **konzistentní**.
Když necháš čtyřicet endpointů napsat bez specifikace, dostaneš čtyřicet
mírně odlišných endpointů — a to je horší než ORM, protože nekonzistenci
nikdo nevynucuje.

Co z generování dělá použitelnou strategii, je existence **napsané, úzké,
vynucené konvence**: pevný protokol, pevná kostra endpointu, pevná sada
vstupních funkcí, pevná sada příkazů. Pak je generování jen rychlé
vyplňování známé šablony a výsledek se dá zkontrolovat pohledem.

**Proto je tahle dokumentace zároveň zadáním pro generátor.**
[06 — Aplikace](06-aplikace.md) obsahuje šablonu endpointu, která se dá
předat člověku i modelu se stejným výsledkem.

### Dvě věci, které generování nenahradí

1. **Bezpečnost z konstrukce.** ORM parametrizuje dotazy, ať píše kdokoli.
   Generovaný kód je bezpečný jen tak, jak pozorný je ten, kdo ho čte.
   Odpověď frameworku: bezpečná cesta musí být zároveň ta nejpohodlnější —
   `req_int()` je kratší než `$_REQUEST[...]`, `is_word()` je kratší než
   ruční kontrola cesty. Viz [09 — Bezpečnost](09-bezpecnost.md).

2. **Evoluce schématu.** Migrace databáze ORM řeší a tenhle framework ne.
   Je to skutečná mezera, ne vyřešený problém — potřebuješ na to vlastní
   postup, ať už ORM máš, nebo nemáš.

A pro úplnost: identity map, unit of work a transakční hranice jsou věci,
kde ORM pořád dává smysl — v doménově složitých, zápisově náročných
systémech. V aplikaci typu „formulář, seznam, detail" prakticky nikdy.

## Co framework záměrně neřeší

Databázi, šablonovací jazyk, autentizaci, routing na serveru, validaci,
lokalizaci. To všechno je věc projektu. Framework dělá jedno:
**doručí příkaz ze serveru do DOMu, čtyřmi transporty a jedním formátem.**
