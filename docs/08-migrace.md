# 08 — Migrace starého projektu

Hlavní myšlenka: **neměň logiku, změň jen způsob doručení.** Migrace jde
po stránkách, aplikace mezitím funguje.

## Krok 1 — kostra

Vezmi existující layout a nech v něm jen to, co se nikdy nemění. Obsah,
který se překresluje, dostane `id` a vyprázdní se.

```html
<div id="left_menu"></div>
<div id="top_frame"></div>
<div id="main"></div>
<script src="fw.js"></script>
<script>Fw.init({ api: '/api/' });</script>
```

## Krok 2 — API jako obálka kolem starých souborů

Nepřepisuj stránky. Nech je vypisovat HTML jako dosud a jen zachyť výstup:

```php
if (strncmp($function, 'page_', 5) === 0) {
    if (!is_word($function)) throw_http_error(400, 'Neplatný název');
    start_direct_answer(['sel' => 'main']);
    require "$PAGES/$function.inc";        // původní soubor, beze změny
    finish_direct_answer();
    finish_answer();
    exit;
}
```

Od téhle chvíle je každá stará stránka dostupná jako `function=page_xxx`
a doručí se do `#main` bez reloadu.

## Krok 3 — odkazy

Tohle je celý refaktoring na straně HTML:

```html
<!-- bylo -->
<a href="/detail.php?id=42">Detail</a>

<!-- je -->
<a href="#?function=page_detail&id=42" data-fw>Detail</a>
```

Formuláře stejně:

```html
<form data-fw action="#?function=save_form" method="post">
```

Žádné `onclick`, žádná jména funkcí navíc. `href` zůstává smysluplný.

## Krok 4 — vstupy

V každém převedeném endpointu nahraď přímé sahání na `$_REQUEST`:

```php
// bylo
$id = addslashes($_REQUEST['id'] ?? '');

// je
$id = req_int('id');            // číslo
$nazev = req('nazev', 128);     // string
```

Proč ne `addslashes()`: escapuje čtyři znaky pro jeden kontext a používá se,
jako by byl univerzální. V číselném kontextu (`WHERE id = $id`) neudělá nic,
proti traversalu nic, proti XSS nic, a na PHP 8 shodí endpoint na `TypeError`,
když přijde `?id[]=1`. Podrobně v [09 — Bezpečnost](09-bezpecnost.md).

## Krok 5 — session

Ruční `?session=…` v URL zahoď. Klient posílá `X-App-Session` sám, ke
každému požadavku včetně fetche fragmentů.

```php
$session = req_header('X-App-Session');
```

Server ji mění příkazem:

```php
send_answer(['op' => 'session', 'value' => $novy]);   // null = odhlášení
```

## Krok 6 — postupné zjemňování

Až sem stačilo přidávat atributy. Teprve teď se vyplatí měnit stránky tak,
aby překreslovaly míň:

```php
// místo překreslení celého #main
send_answer([
    ['op' => 'html',  'sel' => '#pocet_polozek', 'content' => $n],
    ['op' => 'class', 'sel' => '#radek_' . $id, 'add' => ['zmeneno']],
]);
```

Tady se teprve projeví hlavní přínos — a je to práce, kterou lze dělat
po jednotlivých obrazovkách podle toho, kde to nejvíc pálí.

## Krok 7 — automatický přepis, ale tokenizerem

U tisíce volání se ruční přepis nevyplatí. **Regex nad PHP zdrojákem ale
nefunguje** — nevidí hranice řetězců a komentářů. Používej `token_get_all()`:

```php
foreach (token_get_all($src) as $t) {
    if (is_array($t) && $t[0] === T_STRING && strtolower($t[1]) === 'mysql_query')
        { $out .= '$db->tsquery'; continue; }
    $out .= is_array($t) ? $t[1] : $t;
}
```

Tři pasti, které stály nejvíc času:

**Závorky uvnitř řetězců.** V interpolovaném SQL `"… $wh)"` vznikne
samostatný token s textem `)`, který závorka **není**. Při párování počítej
jen tokeny kódu (nepolové tokeny), jinak se argument vloží doprostřed dotazu.

**Druhý argument staré funkce.** `mysql_query($sql, $link)` bralo spojení
jako druhý parametr. Pouhé přejmenování z něj udělá popis místa. Projdi
po přepisu všechna volání, kde je druhým argumentem proměnná spojení.

**Skládání hlášek do popisu.** Když po dotazu následovalo `dberror("popis")`,
patří ten popis do nové funkce jako argument:

```php
$res = mysql_query($sql);          $res = $db->tsquery($sql, "získat kategorii");
dberror("získat kategorii");   →
```

Ale pozor: kde už nové API popis mělo, stane se ze složené hlášky
**třetí** argument — u nás příznak „nehas chybu fatálně". Po přepisu ověř,
že třetí argument nikde není řetězec.

## Krok 8 — PHP 8

Nekvótované identifikátory byly v PHP 7 řetězce s notice, v PHP 8 jsou
fatální. Vyskytují se ve **třech** podobách a je snadné najít jen první dvě:

```php
$_REQUEST[od]          →  $_REQUEST['od']       klíč pole
case go:               →  case 'go':            návěští switche
acl_check(admin_konta) →  acl_check('admin_konta')   hodnota
$w = user;             →  $w = 'user';
```

Uvnitř řetězců se **nesahá** — `"$sl[nazev]"` je v PHP 8 legální.
Skutečné konstanty projektu i vestavěné je nutné přeskočit; stejně tak
`namespace`, typové deklarace a názvy tříd.

Dál hledej odstraněné funkce. U nás to byl `money_format()` (pryč od PHP 8.0).
Polyfill chráněný `function_exists()` je bezpečnější než přepis volání —
na staré verzi se nezmění vůbec nic. Referenční chování si vytáhni ze staré
verze PHP a lať proti němu bajt po bajtu; nám by jinak utekly dvě věci:
zaokrouhlování **na sudou** a oddělovač tisíců U+202F, který `strrev()`
rozbil tím, že obrátil i jeho bajty.

A pozor na `DivisionByZeroError`: `$a/$b` s nulou byla na PHP 7 warning,
na PHP 8 je to fatální chyba. Hledej dělení hodnotou z databáze.

## Krok 9 — bezpečnostní revize při portování

Portování je nejlepší příležitost si kód přečíst. Vzorce, které jsme našli
a které vypadají jako regrese, ale jsou to opravy:

| nález | proč je to problém |
|---|---|
| kontrola vlastnictví jen ve větvi `edit`, ne v `save` | `save` se volá POSTem přímo, cizí `id` projde |
| `if ($opw=!$us->heslo || …)` | přiřazení místo porovnání zruší kontrolu hesla |
| `acl_check(admin_honta)` | překlep v názvu práva — kontrola nikdy neprojde |
| `stripos(…)!==FALSE` u zákazu | obrácená podmínka; projevila by se, kdyby výsledek někam šel |
| ladicí `echo` uprostřed validace | tiskne se do stránky |

Vlastnictví patří **i do podmínky dotazu**, nejen do kontroly nad ním:

```php
$own = $supa ? '' : ' AND mu_cust_id=' . intval($sid->uid);
$db->tsquery("UPDATE mail_users SET $set WHERE mu_id=$id$own", 'uložení');
```

Když opravuješ chybu originálu, **napiš to nahlas**. Tiše změněné chování
vypadá při testování jako regrese.

## Krok 10 — jak port ověřit

Portovaná stránka se nedá spolehlivě ověřit tím, že se na ni podíváš.
Ukládací větve mají odbočky, na které se v prohlížeči jen tak nedostaneš,
a zkoušet je na ostrých datech znamená zakládat faktury a měnit klientům
stavy.

Postav si proto harness, který stránku vykreslí mimo HTTP a umí jí
zakázat zápis:

- session se nezakládá, `$sid` se poskládá z tabulky uživatelů
- spojení se obalí objektem, který u `INSERT`/`UPDATE`/`DELETE` dotaz
  jen vypíše a vrátí náhradu za `PDOStatement`
- na výstup jde HTML, na chybový výstup operace protokolu a diagnostika

Pak se dá číst **vygenerované SQL**. Tam je vidět to, co z prohlížeče
nepoznáš: že se uložil prázdný řetězec místo hodnoty, že v podmínce
chybí omezení na vlastníka, nebo že podvržené pole prošlo.

Projít je potřeba i odmítavé cesty: bez oprávnění, cizí záznam,
hodnota mimo ENUM. A nakonec uklidit testovací předvolby.

Co se ověřit nedá — třeba větev, pro kterou v datech neexistuje
kombinace práv a záznamů — se nemá hlásit jako ověřené.

## Na co narazíš

| problém | řešení |
|---|---|
| `<script>` uvnitř staré stránky | `innerHTML` ho neprovede → příkaz `call` |
| stránka vypisuje `<html>` a `<head>` | ořež na obsah; hlavička patří do shellu |
| přesměrování `header('Location:')` | nahraď příkazem `html` nebo `history` |
| `exit`/`die` uprostřed stránky | ukončí i odpověď → nahraď návratem |
| relativní cesty k obrázkům | teď se počítají od stránky aplikace, ne od skriptu |
| tlačítko Zpět | přidej příkaz `history` u navigací |

### Katalog toho, co jsme opravdu našli

Za zhruba dvacet portovaných stránek se opakovalo tohle:

| nález | dopad |
|---|---|
| ACL bez `return` (`echo "nemate opravneni";` a pokračuje) | kdokoli přihlášený mohl zakládat konta |
| stránka bez kontroly oprávnění vůbec | fakturované položky komukoli |
| omezení na vlastní záznamy jen v HTML (`disabled`) | POSTem šlo přepsat částku bankovního pohybu |
| `$whlim` použité uvnitř funkce, kde není globální | omezení se nevztahovalo na položky |
| nedosazená proměnná v `WHERE $where id=$id` | podmínka nic neomezovala |
| sloupec, který v tabulce není | formulář nabízel pole, které nešlo uložit |
| překlep v názvu proměnné při ukládání | datum se nikdy neuložilo |
| hodnota z requestu do `ORDER BY` | řazení šlo podstrčit |
| `$count` použité dřív, než se naplní | stránkování se vždy vrátilo na první stranu |

Nic z toho nebylo vidět na první pohled a většina se projevila až na
vygenerovaném SQL. Proto Krok 10.

## Co nemigrovat

Endpointy, které vracejí soubory (PDF, CSV, obrázky), nech být. Jsou to
obyčejná stažení, framework s nimi nemá co dělat — odkaž na ně přímo.
