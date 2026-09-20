---
name: waggle-migrate
description:
  "Převod starého PHP projektu (4/5/7) na framework a na PHP 8 — mlčící
  démoni, fatály z odstraněných funkcí, tiché ztráty dat. Načti při
  'převeď stránku', 'portuj z legacy', 'démon neběží', 'skript spadl',
  'Call to undefined function/method', 'Call to a member function on
  null', 'each()', 'array_merge(): Argument #1', při převodu mysql /
  mysqli na PDO, nebo když se má ověřit, že starý skript pod osmičkou
  doopravdy funguje."
---

# Převod starého projektu

Dvě věci naráz: **jiný běh** (celý reload → příkazy frameworku) a **jiné
PHP**. Dělat obojí v jednom kroku bez testů je cesta k tichým ztrátám dat.
Postup převodu je v `docs/08-migrace.md`, tady je řemeslo a katalog pastí.

## Pravidlo: stejné dotazy, stejná sémantika, jiný markup

Převáděná stránka se má chovat **přesně** jako originál, včetně jeho
podivností. Co vypadá jako chyba, se nejdřív ověří proti datům a nahlásí
— neopravuje se mimochodem. Jinak se nedá poznat, jestli rozdíl ve
výstupu způsobil převod, nebo oprava.

Před psaním SQL si vždycky ověř skutečná jména sloupců proti
`information_schema`. Staré stránky běžně zapisují do sloupců, které
v tabulce nejsou — a MySQL to roky mlčky polykalo.

## Než cokoli spustíš

Skripty starých systémů fakturují, párují platby, odpojují klienty
a **rozesílají e-maily**. Odeslaný e-mail se nevrací.

```bash
grep -nE "curl_|fsockopen|mail\(|exec\(|SoapClient" skript.php
grep -nE "INSERT|UPDATE|DELETE|REPLACE" skript.php
```

Zjisti, co je v datech připravené (kolik řádků čeká na odeslání), a
testuj přes obal, který zápisy jen vypíše — do **kopie** skriptu, nikdy
do originálu.

## Katalog pastí

### 1. Knihovna se připojovala sama při includu

Staré knihovny mívají na konci souboru, mimo funkce, `mysql_connect()`.
PDO varianta to nedělá → `$db` je `NULL` a první dotaz hlásí
`Call to a member function tsquery() on null`.

Zákeřnější varianta: spojení se naváže správně a **až potom se přepíše**,
protože někde v řetězci includů je stará knihovna, která si connect dělá
sama. Hláška je pak `Call to undefined method mysqli::tsquery()`.

Hledej podle **jména knihovny**, ne podle tvaru volání — automatické
přepisy minou `require_once $root.'/lib/db.inc'`, když hledaly
`require_once("$root/lib/db.inc")`.

### 2. `each()` — zrušeno v PHP 8.0

`while (list($k,$v) = each($ar))` → `foreach`. Bonus: odpadne sdílený
vnitřní ukazatel pole, přes který si rekurzivní volání šlapala navzájem.

### 3. Konstruktory ve stylu PHP 4 — zrušeny v PHP 8.0

```php
class Client { function Client($p = array()){ … } }   // nikdy se nezavolá
```

Objekt vznikne, ale s výchozími hodnotami z `var`. **Proto to vypadá, že
je všechno v pořádku** — jen se nenastaví to, co dělal konstruktor.
Typicky zůstane prázdné jméno a heslo a klient se pak diví 401.

### 4. Funkce vracející `false` místo pole

V PHP 7 varování, v osmičce **TypeError a konec skriptu**:

```php
$r = dns_get_record($d, DNS_MX);   // při chybě resolveru false
$r = array_merge($r, $rad);        // fatál
```

Skript doběhne do poloviny a zbytek se **nikdy neudělá**. Nikde to
nehlásí. Za každé takové volání patří `?: []`.

### 5. Zavináč nepotlačí `Error`

`@` tlumí varování, ne `Error` z neexistující funkce. Volání do
rozšíření, které pro osmičku neexistuje, spadne i se zavináčem.

### 6. Dělení nulou je fatál

`DivisionByZeroError`, ne varování. Ověř v datech, jestli nula nastat
může.

### 7. Nedosazené proměnné v SQL

```php
$sql = "UPDATE t SET $set WHERE $where id=$id";   // $where nikde nevzniká
```

Vyrobí `WHERE  id=5` — funguje, ale ta podmínka měla něco omezovat.
Časté u omezení na vlastní záznamy, které navíc bývá použité **uvnitř
funkce, kde není `global`**, takže je prázdné a omezení neplatí. Na
frameworku je tohle horší než v původním kódu: stránka běží uvnitř
funkce, takže i globály, které dřív „prostě byly", musí být deklarované.

### 8. `disabled` není kontrola

Prohlížeč `disabled` pole neodešle, útočník ano. A pozor na opak:
`readonly` se odešle, `disabled` ne — pokud hodnota musí přežít uložení,
patří tam `readonly`.

### 9. Rovnost se zástupným znakem proti číselnému sloupci

```sql
WHERE zip = '37005%'
```

Vypadá jako chyba. Ale když je `zip` typu `int`, MySQL řetězec přetypuje
na `37005` a **funguje to i použije index**. „Oprava" na `LIKE '37005%'`
index shodí.

**Poučení: zjisti typ sloupce, než něco prohlásíš za chybu.** Tenhle bod
je v katalogu proto, že jsem se spletl já a pravdu měl uživatel.

### 10. `floor()` nad prázdným řetězcem = TypeError

```php
floor($_REQUEST['pocet']);      // prázdné políčko formuláře -> fatál
```

Formuláře posílají prázdná číselná políčka úplně běžně, takže to spadne
při prvním uložení. Obalit `floatval()` / `intval()`. Hledej to
v **sdílené** funkci pro import formulářů — tam zasáhne každý editační
formulář projektu naráz.

### 11. Nezaškrtnutý checkbox se neposílá

Import, který chybějící pole přeskočí, umí hodnotu nastavit, ale ne
zrušit. Sloupec zůstane, jak byl, a nikdo si toho roky nevšimne. Na
checkbox patří příznak „existence", ne „číslo".

## Jak hlásit

Reprodukovat, ukázat stack trace nebo `EXPLAIN`, teprve pak opravovat.
U cizích knihoven říct, že se oprava při aktualizaci ztratí. Nález, který
znamená **tichou ztrátu dat**, hlásit dřív než kosmetiku — ten nikdo
nereklamuje, protože o něm neví.
