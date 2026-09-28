# 00 — Slovníček

Jedno jméno pro jednu věc. Tahle kapitola je krátká schválně: pojmů, které
je potřeba znát, je pět.

## Vrstvy

Vrstvy se rozlišují **podle toho, čím mluví**. To je jediné kritérium,
které platí ve dvouvrstvé i tříúrovňové aplikaci — a dá se ověřit grepem.

| pojem | co dělá | čím mluví | v příkladech |
|---|---|---|---|
| **frontend** | vykresluje a reaguje na události; nemá stav aplikace ani router | Waggle, klientská strana | `examples/library/app/` |
| **BFF** | aplikační logika a skládání HTML; **jediná serverová vrstva, která mluví Waggle** | Waggle | `examples/library/bff/`, `examples/bff/` |
| **datové API** | vlastní data, vynucuje oprávnění; o protokolu neví | prostý JSON přes HTTP | `examples/library/api/` |

**BFF** je *backend for frontend*: vrstva, jejíž tvar určují potřeby
obrazovky, ne tvar databáze. Proto skládá HTML — ví, co se má nakreslit.

**„Backend" není název vrstvy.** Jako volné označení pro „všechno na
serveru" projde, ale nikdy jako jméno patra: u dvouvrstvé aplikace by
znamenalo BFF, u tříúrovňové datové API. Buď řekni BFF, nebo datové API.

## Dva modely

| model | vrstvy | kdy |
|---|---|---|
| **dvouvrstvý** | frontend + BFF, které sahá rovnou na data | převod staršího monolitu; samostatná datová vrstva by byla práce navíc bez užitku |
| **tříúrovňový** | frontend + BFF + datové API | nový projekt, nebo když data používá i jiný klient — mobilní aplikace, integrace, agent |

Waggle je v obou případech tentýž a protokol se neliší. Mění se jen to,
odkud si BFF bere data: v prvním případě z databáze, ve druhém z datového
API přes HTTP.

## Knihovna a co kolem ní

| pojem | co to je |
|---|---|
| **knihovna** | `fw.js`, `fw.inc`, `io.inc`. Kopíruje se do projektu, needituje se v něm. |
| **kostra** | dispatcher, konfigurace, pomocné soubory. Vezme se jednou při zrodu projektu a pak se rozchází. |
| **příklad** | `examples/*`. Referenční text, ne závislost. |
| **fragment** | kus HTML, který BFF složí a pošle jako obsah příkazu. Nikdy ho nestahuje prohlížeč. |
| **okno** | pojmenovaná oblast stránky, na kterou míří příkazy — `left_menu`, `top_frame`, `main`, `overlay`. Úmluva, ne pravidlo. |
| **příkaz** | jedna položka odpovědi: `op` plus jeho parametry. Úplný seznam v [03 — Protokol](03-protokol.md). |

## Jména v kódu

| | |
|---|---|
| `Fw.init({ bff: … })` | adresa BFF. Do 1.6.1 se klíč jmenoval `api`, což bylo matoucí — prohlížeč datové API nikdy nevolá. Starý klíč funguje dál. |
| `BFF_URL`, `BFF_CACHE_DIR` | konfigurace BFF v `examples/library` |
| `API_URL` | adresa **datového** API, kterou volá BFF |
| `in_*` | čtečky vstupu z `io.inc` |
| `is_word()` | predikát, ne čtečka — nic nečte |
| `bff_*` | pomocné funkce BFF v projektech autora |
