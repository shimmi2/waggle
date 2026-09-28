# 12 — Nový projekt z příkladu

[08 — Migrace](08-migrace.md) je pro projekt, který už existuje. Tahle
kapitola je pro nový, a hlavní myšlenka je stejně nudná jako účinná:
**nezakládej prázdný adresář.** Vezmi `examples/library`, kde už jsou tři
vrstvy, sezení, oprávnění, omezení pokusů, dva instaláky a úklidový cron —
a vyměň knihy za to, co doopravdy potřebuješ.

Co se mění, je **domé­na problému**. Architektura ne.

Tahle kapitola je napsaná i pro model. Když řekneš „přidej editaci
uživatelů", má z ní být jasné, do kterých souborů a v jakém pořadí, aniž by
se cokoli vymýšlelo znovu.

## Krok 1 — rozkopírovat

```bash
cd examples/library
./install.sh --conf=nasazeni.conf
```

Tři docrooty, runtime účet databáze bez DDL, `UNSAFE_DEMO=false`,
`FW_DEBUG=false`, vypsané direktivy pro Apache i nginx. Podrobnosti v
[README knihovny](../examples/library/README.md).

Pak smaž knihy: `sql/03-books.sql`, `sql/gen-rentals.*`, `api/inc/api_books.inc`,
`api/inc/api_rentals.inc`, `bff/pages/book*`, `bff/pages/rentals*`. Zůstane
kostra, která umí přihlásit člověka a nakreslit obrazovku — a to je přesně
to, co nechceš psát znovu.

## Krok 2 — co se nesmí měnit

| | |
|---|---|
| `fw.inc`, `fw.js` | **knihovna**. Rozváží `tools/fwdeploy.sh`. Každá úprava patří do nové verze Waggle, ne do projektu. |
| `api/inc/boot.inc`, `input.inc`, `api_session.inc`, `api_throttle.inc` | kostra. Funguje. Sáhni tam jen s důvodem, který umíš vyslovit. |
| `bff/inc/api_client.inc`, `session_cache.inc` | totéž. Zejména obsluhu 401 v `api_call()`. |

Tvoje území: `sql/*`, `api/inc/api_<modul>.inc`, `api/index.php` (jen switch),
`bff/index.php` (jen switch), `bff/pages/*`, `bff/inc/ui.inc`, `app/app.css`.

## Krok 3 — schéma

Tabulka na entitu, **předpona sloupců podle tabulky**: `bo_` books, `us_`
users, `re_` rentals. Není to obřad — díky tomu se joiny čtou bez aliasů a
`grep bo_count` najde všechna místa, která ten sloupec berou do ruky.

Oprávnění jsou **řádky v číselníku** `acls` a v `users.us_acl` seznam
oddělený čárkami. Primitivní schválně. Nové právo = jeden `INSERT` do
číselníku a jeho jméno v kódu.

## Krok 4 — endpoint na backendu

Jedna funkce `ep_<co>()` na endpoint, jeden soubor `api_<modul>.inc` na
modul, jeden řádek do switche v `api/index.php`. A **vždycky tohle pořadí**:

```php
function ep_neco_save(): void {
    /* 1. vstupy */      $x = in_str('x', 64); $id = in_int('id');
    /* 2. sémantika */   if ($x === '') api_error(400, 'X je povinné');
    /* 3. sezení */      $me = session_require();
    /* 4. oprávnění */   acl_require($me, 'manage_neco');
    /* 5. práce */       q("UPDATE …");
    /* 6. odpověď */     api_out(['ok' => true, 'id' => $id]);
}
```

Není to zvyk, je to **podmínka**. Jediná přípustná výjimka je ověření sezení
společné pro celý modul. Proč tak přísně: kroky 1 a 2 jsou levné a bez
následků, takže je můžou být první; všechno od kroku 5 už něco dělá. Mezi
tím stojí otázka „kdo se ptá a smí to".

Vstupy **jen** přes `in_str/in_int/in_float/in_word/in_rows` — mají stropy a
bez stropu je jediný parametr DoS. Chyby **jen** přes `api_error()` s HTTP
kódem: 400 vyplnění, 401 sezení, 403 oprávnění, 404 neexistuje, 429 příliš
mnoho pokusů. Žádné `{"ok":false}` s dvěstěkou.

## Krok 5 — obrazovka v BFF

Totéž pořadí, jen tenčí — skutečná práva hlídá backend, tady se rozhoduje o
tom, **co se nakreslí**.

```php
case 'neco':
    $r = api_call('neco_list', ['q' => req('q', 64)]);
    send_answer(jen_main('neco', $r));
    send_answer(['op' => 'history', 'url' => '#?function=neco']);
    break;
```

`jen_main()` posílá jen `main`. `obrazovka()` posílá i `left_menu` a
`top_frame` a používá se **jen** při přihlášení, odhlášení a na úvodu, kdy se
mění stav aplikace. Posílat menu u každé obrazovky znamená překreslovat dvě
třetiny stránky kvůli jedné tabulce.

Filtr a výsledky patří do **dvou samostatných divů**. Filtr pak zůstane stát
i s fokusem a překresluje se jen tabulka pod ním, takže člověk může po
odeslání rovnou psát dál. Vzor je `books` → `books_results`.

## Krok 6 — fragment

`esc()` na všechno, co přišlo od člověka nebo z databáze. Stav odznakem, ne
barvou řádku — obarvený řádek se v dlouhé tabulce čte špatně. Hodnoty filtru
se **předvyplňují z požadavku**, jinak tabulka filtruje a rozbalovátko tvrdí
„všechny", což je ta nejhorší kombinace.

## Krok 7 — zápisová obrazovka

Tohle je recept, který stačí přepsat jmény. Vzor v kódu: `ep_book_save()`,
`bff/pages/book_form.inc`, casy `book_form` a `do_book_save`.

**Na backendu**

1. `$op = in_str('op', 16)` a v kroku 2 ověřit, že si záměr neodporuje s daty:
   `op` mimo `insert|update` → 400, `update` bez id → 400, `insert` **s** id → 400.
   Nikdy neodvozovat záměr z přítomnosti id: ztracené id pak tiše založí
   duplikát a odpoví `ok`.
2. Ověřit i to, co databáze uhlídat nemůže — třeba že počet kusů nesmí
   klesnout pod to, co je právě půjčené.
3. Vrátit `['ok' => true, '<tabulka>_id' => $id]`.

**V BFF dva casy, oba v pořadí 1–6**

`<co>_form` kreslí dialog:

```php
case 'neco_form':
    /* 1. vstupy */    $id = req_int('ne_id');
    /* 3. sezení */    if (me(true) === null) { … 401 … break; }
    /* 4. oprávnění */ if (!may('manage_neco')) { … 403 … break; }
    /* 5. + 6. */      $b = $id > 0 ? api_call('neco_detail', …)['neco'] : prazdne_neco();
                       send_answer(['op' => 'html', 'sel' => 'overlay',
                                    'content' => frag('neco_form', [… 'op' => $id > 0 ? 'update' : 'insert' …])]);
    break;
```

`me(true)` je tu **schválně**: cache blok o přihlášeném platí minutu a
otevřít formulář nad mrtvým sezením znamená, že člověk vyplní celou stránku
a přijde o ni až při ukládání. Jedno volání navíc je proti tomu levné. Čtecí
obrazovky cache věří.

`do_<co>_save` ukládá, a tady je celá pointa v tom, **kam jde která chyba**:

| co se stalo | odpověď | proč |
|---|---|---|
| chyba vyplnění (400) | **fragment znovu do `overlay`** s `err` a se vším, co člověk napsal | opravitelné, a nesmí přijít o rozepsané |
| `op` si odporuje s id | `error` 400 | rozbitý formulář, ne chyba vyplnění — člověk to neopraví, špatné pole ani nevidí |
| chybí právo (403) | `error` 403 | překreslený formulář by lhal, že to po opravě půjde |
| vypršelé sezení (401) | `cache_forget()` + `session: null` + `error` 401 | jinak si klient dál nosí mrtvý token |
| hotovo (200) | `html sel=overlay content=''` + `notify` + překreslit výsledky | zavřít, říct to, ukázat změnu |

Proto se u zápisu volá **`api_raw()`, ne `api_call()`**. `api_call()` mění
každou nedvěstovku na `error` a skončí — což je správně pro obrazovku a
špatně pro formulář, protože člověku zmizí, co napsal.

**Jeden fragment, dva vstupní stavy.** `$op` řídí titulek i text tlačítka,
výchozí hodnoty žijí v jedné funkci `prazdne_<co>()`. Dva fragmenty by se
rozešly — do jednoho by se za měsíc přidalo pole a do druhého ne.

**Stav filtru se veze s sebou.** Odkazy na dialog nesou aktuální filtr a
formulář ho vrací zpátky jako `f_*`, aby se po uložení překreslil týž
seznam. Ty odkazy patří do **fragmentu s výsledky**, protože ten je jediný,
kdo aktuální filtr zná — hlavička s rozbalovátky se po změně filtru
nepřekresluje a nesla by neplatný stav.

**`data-busy` patří na `<form>`, ne na tlačítko** (`busyBehem()` dostává při
odeslání formulář) a `data-busy-sel="overlay"`, jinak se rozostří obsah pod
dialogem místo dialogu.

## Krok 8 — ověřit

```bash
# každá obrazovka × každá role, s E_ALL — nula varování je podmínka
# API musí fungovat i bez BFF; tím se pozná, že práva hlídá on a ne BFF
curl -s -X POST "$API/?fn=neco_save" -H "X-App-Session: $S" -d "op=insert&ne_id=7"
```

Tři věci, které odhalí nejvíc: projet všechny obrazovky pro **každou roli**
(403 musí padat právě tam, kde chybí ACL), zkusit uložit jméno
`"><script>alert(1)</script>` a zavolat zápisový endpoint **přímo**, mimo
BFF, s podvrženým `op`.

## Když ti řeknu „přidej editaci uživatelů"

Konkrétně, protože přesně tenhle úkol má být nudný:

1. **API: nic nebo osm řádků.** `ep_user_save()` už `op` má a ukládá jen ta
   práva, která jsou v číselníku. `ep_users_list()` už vrací i `acls` a celé
   řádky, takže předvyplnit se dá z něj. U velké tabulky přidej
   `ep_user_detail()` podle `ep_book_detail()` a řádek do switche.
2. **`bff/pages/user_form.inc`** podle `book_form.inc`: `op` a `us_id` jako
   hidden, `us_login`, `us_name`, `us_phone`, `us_email`, `us_note`,
   zaškrtávátka práv z `acls`, heslo jako `password` s poznámkou
   „prázdné = nemění se".
3. **`bff/index.php`**: casy `user_form` a `do_user_save` podle
   `book_form`/`do_book_save`, ACL `manage_users`, po uložení překreslit
   `#users_results`.
4. **`bff/pages/users.inc`**: tlačítko „Přidat uživatele" a „Upravit" na
   řádku, obojí za `may('manage_users')`, obojí nesoucí filtr. Pokud tam
   ještě není samostatný div s výsledky, rozdělit obrazovku na filtr a
   výsledky jako u knih.
5. Projet krok 8.

Heslo má jeden vlastní zvyk: **prázdné pole znamená „neměnit"**, ne
„nastavit prázdné". `ep_user_save()` to tak už dělá a formulář to musí říct
nahlas, jinak si někdo omylem vyrobí nepřihlašitelné konto.

## Čeho se nedopustit

* **`api_call()` u zápisu formuláře.** Sežere člověku, co napsal.
* **Odvozovat `insert`/`update` z přítomnosti id.** Tichý duplikát.
* **Zkontrolovat `may()` dřív, než víš, že sezení vůbec je.** Komu vypršelo,
  dostane hlášku o oprávnění místo přihlášení — a to je běžná situace.
* **Věřit statické cache v `me()` po přihlášení v témže požadavku.** Je to
  memoizace na celý požadavek; jednou vrátila `null` a bude ji vracet dál.
* **Deklarovat funkci mezi `case` návěštími.** Switch na návěští skočí a
  deklarace se nikdy nevykoná. Pomocné funkce patří nad switch.
* **Překreslovat kontejner, který na dané obrazovce není.** `#neco_results`
  existuje jen tam, kde ho fragment vykreslil; odjinud odkazuj na celou
  obrazovku.
* **Sahat na `fw.inc` nebo `fw.js` v projektu.** Rozejde se to s knihovnou a
  příští `fwdeploy.sh` to přepíše.
* **Databáze v BFF.** Tím se dělení vrstev změní na ozdobu.
