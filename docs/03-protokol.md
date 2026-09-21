# 03 — Protokol

## Obálka

Jedna dávka na řádek, formát **NDJSON**:

```
{"v":1,"cmds":[{"op":"html","sel":"main","content":"…"}]}
{"v":1,"cmds":[{"op":"html","sel":"top_frame","content":"…"}]}
```

`v` je verze protokolu. Obálka je objekt, aby do ní šlo přidat pole
(`req_id`, `trace`, `ts`) bez porušení kompatibility.

Prázdný řádek nebo řádek samých mezer klient **zahodí** — toho využívá
`fw_stream_start()`.

## Transporty

| transport | směr | `ctx.origin` |
|---|---|---|
| `fetch` POST, NDJSON odpověď | klient → server → klient | `response` |
| NDJSON stream (průběžný flush) | server → klient | `response` |
| `BroadcastChannel` | okno → okna | `broadcast` |
| SSE přes nchan | server → klient, asynchronně | `push` |
| `Fw.local()` | v rámci okna | `local` |

Odesílání je **vždy** POST. Push je jen jednosměrný, server → klient.

## Požadavek

`POST` na `Fw.cfg.api`, tělo je `FormData` (takže `<input type=file>`
funguje zadarmo). Povinné pole `function`, zbytek jsou parametry.

| hlavička | význam |
|---|---|
| `X-App-Session` | token session; **nikdy** v URL |
| `X-App-Serial` | trvalá identifikace instalace v prohlížeči |

Klient přidává obě hlavičky i k fetchi fragmentů (`op: url`), takže na
serveru neexistuje žádné `append_session` a nikdy se neřeší `?` vs `&`.

Navíc se ke každému požadavku přidají `Fw.cfg.params` — v demu `skin`,
v reálném projektu tenant, jazyk nebo verze klienta.

## Cíl příkazu

`sel` je CSS selektor. Holé slovo (`[A-Za-z0-9_-]+`) se normalizuje na `#id`,
cokoli jiného jde přímo do `querySelectorAll`. Aplikuje se na **všechny**
shody; když selektor nenajde nic, je to varování v konzoli, ne chyba.

## Reference příkazů

### Obsah DOMu

| op | pole | co dělá |
|---|---|---|
| `html` | `sel`, `content` | nahradí obsah prvku |
| `append` | `sel`, `content` | přidá na konec |
| `remove` | `sel` | odstraní prvek |
| `url` | `sel`, `url` | klient stáhne fragment a vloží ho |

`url` má smysl jen pro velký, veřejný, cache-ovatelný fragment servírovaný
přímo webserverem. Cokoli závislého na session statickým souborem být
nemůže, takže by se neušetřilo nic a zaplatil by se druhý round-trip.
**Výchozí je vždy `html` — obsah jde po drátě v odpovědi.**

### Vlastnosti prvků

| op | pole | co dělá |
|---|---|---|
| `attr` | `sel`, `name`, `value` | nastaví atribut; `value: null` ho smaže |
| `value` | `sel`, `value` | hodnota formulářového prvku |
| `class` | `sel`, `add[]`, `remove[]`, `toggle[]` | třídy |

### Stav a řízení

| op | pole | co dělá |
|---|---|---|
| `session` | `value` | nastaví token; `null` = odhlášení |
| `history` | `url` | `pushState`; po submitu formuláře se ignoruje |
| `call` | `fn`, `args[]` | zavolá globální funkci |
| `subscribe` | `name`, `url`, `token` | otevře push kanál |
| `unsubscribe` | `name` | zavře kanál; bez `name` všechny |

### Hlášení

| op | pole | co dělá |
|---|---|---|
| `notify` | `kind`, `message` | hlášení uživateli; `kind` = `success`, `info`, `warning`, `error` |
| `error` | `code`, `message` | totéž s `kind: error` a HTTP kódem |
| `debug` | `message`, `data` | do konzole, jen když `cfg.debug` |

Hlášení o výsledku operace **nepatří do stránky**. Uložení se ohlásí
`notify` a do `main` se rovnou vykreslí to, co má uživatel vidět dál —
typicky seznam, ze kterého přišel. Odpadne tím mezistránka „uloženo",
na kterou stejně nikdo nechce klikat:

```json
{"op":"notify","kind":"success","message":"Alias byl uložen."}
{"op":"html","sel":"main","content":"…seznam…"}
{"op":"history","url":"#?function=page_hosting_aliases"}
```

### Zaneprázdněno

| op | pole | co dělá |
|---|---|---|
| `busy` | `sel`, `text`, `pct`, `state` | překryv „pracuji" nad prvkem |

`state` chybí = běží, `done` = zeleně potvrdí a za 0,7 s zmizí,
`off` = okamžitě pryč. `pct` chybí = kolečko, `pct` je číslo 0–100 = pruh.

```json
{"op":"busy","sel":"main","text":"Připravuji…"}
{"op":"busy","sel":"main","text":"Počítám…","pct":40}
{"op":"busy","sel":"main","state":"done","text":"Hotovo"}
```

Kolečko a pruh **nejsou dva typy**. Jde začít bez `pct`, dokud není známo,
kolik toho bude, a přepnout na procenta ve chvíli, kdy to je jasné.

Tři pravidla, která drží framework, aby je nemusel řešit server:

1. **Jedno okno = jeden překryv.** Volající proto může posílat
   „připravuji / počítám / dokončuji" za sebou bez jakéhokoli stavu
   na klientovi.
2. **Cokoli jiného, co do toho okna přijde, překryv sundá.** Není proto
   nutné posílat `state: off` na šťastné cestě — překryv zmizí ve chvíli,
   kdy dorazí výsledek. Hlídá se to v dispatcheru, ne v operaci.
3. **Prvních 300 ms se nekreslí nic.** Operace, která doběhne dřív,
   neukáže vůbec nic. Záblesk působí pomaleji než ticho.

Vzhled se mění přepsáním `Fw.busyRender(host, box, stav)`, stejně jako
u `notify`. Řadič `Fw.busy()` se nepřepisuje — je v něm právě to
účetnictví z bodů 1 až 3.

**Poslaný ve stejné dávce jako pomalá práce je k ničemu**, protože dávka
dorazí až s výsledkem. Musí jít napřed: streamem, nebo pushem. Viz
[10 — Nasazení](10-nasazeni.md).

### Vlastní operace

Projekt si přidá svoje přes `Fw.register('jmeno', fn)`. Registr je jediné
místo, kde se rozhoduje, co který `op` dělá — jádro se nemění.

```js
Fw.register('sync_note', function (cmd, ctx) {
    document.getElementById('sync_note').textContent =
        ctx.origin === 'broadcast' ? 'z jiného okna' : 'tady';
});
```

## Pořadí a atomicita

Příkazy se aplikují **striktně v pořadí pole**, sekvenčně, s `await`.
Proto musí `session` stát před příkazy, které spouštějí požadavek.

## Chyby

| kdy | jak |
|---|---|
| před prvním bajtem | HTTP status (400/401/403/404/500) **a** tělo s `op:"error"` |
| po prvním bajtu | jen příkaz `op:"error"` |

Klient obojí zpracuje stejnou cestou. Neúspěšné přihlášení **není chyba
protokolu** — je to normální stav aplikace, tedy HTTP 200 a překreslený
formulář.

## Ve fragmentech nesmí být `<script>`

`innerHTML` skripty neprovede. Na spuštění kódu je příkaz `call`, poslaný
**za** příkazem `html`. Je to zároveň přívětivější vůči CSP.

## Verzování

Nová operace = zpětně kompatibilní změna; starý klient ji ohlásí jako
neznámou a pokračuje. Změna významu existující operace = zvýšení `v`.
