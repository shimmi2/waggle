# 03 — Slovníček

Jedno jméno pro jednu věc. Pokud si z dokumentace odnesete jen tuhle
kapitolu, budete si s ostatními rozumět.

## Vrstvy

Vrstvy se rozlišují **podle toho, čím mluví**. To je jediné kritérium,
které platí ve dvouvrstvé i třívrstvé aplikaci — a dá se ověřit grepem.

| pojem | co dělá | čím mluví | v příkladech |
|---|---|---|---|
| **frontend** | strana prohlížeče, psaná v JavaScriptu: vykresluje a reaguje na události. Nemá stav aplikace ani router. | Waggle, klientská strana | `examples/library/app/` |
| **BFF** | *Backend For Frontend*. Aplikační logika a skládání HTML; **jediná serverová vrstva, která mluví Waggle**. | Waggle | `examples/library/bff/`, `examples/basic_demo/bff/` |
| **datové API** | vlastní data, vynucuje oprávnění; o protokolu neví | JSON přes HTTP | `examples/library/api/` |

**BFF** je *Backend For Frontend*: vrstva, jejíž tvar určují potřeby
obrazovky, ne tvar databáze. Proto skládá HTML — ví, co se má nakreslit.

Ano, znamená to, že se HTML generuje na serveru, a dokonce se trochu
ušpiní potřebami konkrétního frontendu: fragment pro AdminLTE ví o
`card` a `card-body`, fragment pro holé HTML ne. Drží se to ale
**v nutné míře** — CSS, JavaScript a ostatní smetí zůstávají na straně
prohlížeče a do fragmentu nelezou. V `examples/` je to vidět na jedné
aplikaci ve dvou kabátech: holé HTML a AdminLTE se liší **výhradně
markupem fragmentů**, endpointy ani protokol ani o řádek.

V době agentického kódování už tahle duplicita nebolí tak, jako bolela
dřív. Napsat druhou sadu fragmentů podle první je práce, kterou dnes
odvede jeden prompt — a výsledek se kontroluje pohledem, protože je to
HTML a ne konfigurace šablonovacího stroje.

**Datové API** je ta obyčejná věc, kterou má dnes skoro každý projekt a
které se běžně říká REST: požadavek přes HTTP, odpověď v JSON, chyby
stavovým kódem. V `examples/library` je postavené podle datových API
z firemních projektů autora, takže je záměrně nudné a poznáte ho na
první pohled. Striktní REST to není — endpoint se volá jménem
(`?fn=books_list`), ne cestou ke zdroji; na způsobu práce s ním to ale
nic nemění.

**„Backend" není název vrstvy.** Jako volné označení pro „všechno na
serveru" projde, ale nikdy jako jméno patra: u dvouvrstvé aplikace by
znamenalo BFF, u třívrstvé datové API. Buď řekni BFF, nebo datové API.

## Dva modely

| model | vrstvy | kdy |
|---|---|---|
| **dvouvrstvý** | frontend + BFF, které sahá rovnou na data | převod staršího monolitu; samostatná datová vrstva by byla práce navíc bez užitku |
| **třívrstvý** | frontend + BFF + datové API | nový projekt, nebo když data používá i jiný klient — mobilní aplikace, integrace, agent |

Waggle je v obou případech tentýž a protokol se neliší. Mění se jen to,
odkud si BFF bere data: v prvním případě z databáze, ve druhém z datového
API přes HTTP.

## Knihovna a co kolem ní

| pojem | co to je |
|---|---|
| **knihovna** | `fw.js`, `fw.inc`, `io.inc`. Kopíruje se do projektu, needituje se v něm. |
| **kostra** | dispatcher, konfigurace, pomocné soubory. Zkopíruje se jednou při zrodu projektu a od té chvíle je to **váš** kód — upravuje se podle potřeby a s knihovnou se už nesynchronizuje. |
| **příklad** | `examples/*`. Referenční text, ne závislost. |
| **fragment** | kus HTML, který se vloží do okna. Obvykle ho složí BFF a pošle rovnou v odpovědi jako obsah příkazu `html`. Příkazem `url` si ho ale může stáhnout i prohlížeč sám — na vlastní adresu, klidně s GET parametry a s hlavičkami sezení, takže i ta adresa smí odpovídat podle toho, kdo se ptá. |
| **okno** | pojmenovaná oblast stránky, na kterou míří příkazy — `left_menu`, `top_frame`, `main`, `overlay`. Zpravidla je to `<div>` s tím `id`. Úmluva, ne pravidlo: cíl je vždy CSS selektor, takže příkaz může mířit i na jednotlivý prvek — třeba na jedno pole formuláře nebo na řádek tabulky. |
| **příkaz** | jedna položka odpovědi: `op` plus jeho parametry. Úplný seznam v [04 — Protokol](04-protokol.md). |

## Jména v kódu

| | |
|---|---|
| `Fw.init({ bff: … })` | adresa BFF. Do 1.6.1 se klíč jmenoval `api`, což bylo matoucí — prohlížeč datové API nikdy nevolá. Starý klíč funguje dál. |
| `BFF_URL`, `BFF_CACHE_DIR` | konfigurace BFF v `examples/library` |
| `API_URL` | adresa **datového** API, kterou volá BFF |
| `in_*` | čtečky vstupu z `io.inc` |
| `is_word()` | predikát, ne čtečka — nic nečte |
| `bff_*` | pomocné funkce BFF v projektech autora |
