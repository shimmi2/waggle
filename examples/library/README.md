# Knihovna — startovací balík Waggle

Celá aplikace ve třech vrstvách: frontend, BFF a backendové API. Půjčovna
knih, protože na ní je vidět všechno, co firemní aplikace potřebuje —
katalog s filtrem, výpůjčky se stavy, statistika, uživatelé a oprávnění.

**Nečti to jako demo.** `examples/app` a `examples/app-adminlte` jsou dema,
ta ukazují protokol. Tady se **začíná nový projekt**: vezmeš to, přejmenuješ
domény a knihy vyměníš za to, co doopravdy potřebuješ. Jak na to krok za
krokem je v [12 — Nový projekt z příkladu](../../docs/12-novy-projekt.md).

---

## Než to zkusíš přes `php -S`

```bash
PHP_CLI_SERVER_WORKERS=4 php -S 127.0.0.1:8080 -t api
PHP_CLI_SERVER_WORKERS=4 php -S 127.0.0.1:8081 -t bff
```

**Bez `PHP_CLI_SERVER_WORKERS` to zamrzne.** Vývojový server PHP obsluhuje
jeden požadavek v jednu chvíli a BFF volá API po HTTP — takže BFF čeká na
API, které se nemá kdo chopit. Vypadá to jako zatuhlá síť a hledá se to
hodinu. Na Apache ani nginxu se to stát nemůže, tam běží víc procesů.

---

## Instalace

Dvě cesty a dělí je to, jestli mají být vrstvy **doopravdy** oddělené.

### `install.php` — z webu, do místa

Nahraj adresář na hosting, otevři `install.php` v prohlížeči, vyplň přístup
k databázi. Zprovozní to, co jsi nahrál: vytvoří schéma, naplní data,
napíše tři konfigurace, zkontroluje se a **smaže se**.

Všechny tři vrstvy zůstanou pod jednou doménou, takže backendové API je
dosažitelné z internetu. Instalace o tom ví a nese o tom červený odznak
(`UNSAFE_DEMO`). Pro vyzkoušení dobré, pro provoz ne.

### `install.sh` — z příkazové řádky, do tří domén

```bash
./install.sh --conf=nasazeni.conf
```

Tahle cesta rozkopíruje kód do tří docrootů, založí databázový účet, který
umí **jen** `SELECT/INSERT/UPDATE/DELETE` (žádné DDL — aplikace za provozu
nemá co měnit strukturu), zhasne `UNSAFE_DEMO`, vypne `FW_DEBUG` a vypíše
direktivy pro Apache i nginx s doplněnými cestami. Přihlašovací údaje, se
kterými zakládal schéma, se nikam nezapsají.

Bez argumentů vypíše, co všechno chce.

### Požadavky

| | |
|---|---|
| PHP | **8.1** nebo novější |
| `pdo_mysql` | povinné |
| `curl` | povinné — BFF s ním volá API |
| `redis` | volitelné; bez něj jdou sezení do MySQL |
| nchan | volitelné; bez něj jen nejde push |

---

## Přihlášení

Admina s heslem zakládá instalák a heslo ti jednou vypíše. Ukázková konta
mají heslo **prázdné**, tedy nepřihlašitelná — `password_verify()` proti
prázdnému hashi vždycky selže, takže ze SQL souborů sama o sobě nevznikne
žádná cesta dovnitř.

Webový instalák umí na požádání nastavit heslo `demo` dvěma kontům:

| konto | role | co smí |
|---|---|---|
| `hlavata` | vedoucí půjčovny | půjčovat, vidět všechny výpůjčky, statistiku, **spravovat katalog** |
| `novakova` | čtenářka | svoje výpůjčky a katalog, nic víc |

`dvorak` je taky obsluha, ale `manage_books` nemá. Rozdíl proti Hlavaté je
**jedno slovo v `us_acl`** a nic jiného — na tom je vidět, jak jsou role
v téhle aplikaci udělané.

---

## Co je kde

```
app/          frontend — index.html, app.js, app.css, fw.js
              Nesahá na databázi ani na API. Zná jedinou adresu: BFF.
bff/          řízení aplikace, mluví Waggle
  fw.inc        KNIHOVNA — protokol; io.inc si načítá sám
  io.inc        KNIHOVNA — hygiena vstupu a výstupu
  index.php     jeden switch, jedna obrazovka na case
  pages/        fragmenty HTML (13)
  inc/          api_client.inc, session_cache.inc, ui.inc
api/          backendové API, jediná vrstva u databáze
  index.php     dispatcher
  inc/io.inc    KNIHOVNA — táž hygiena vstupů, BEZ protokolu
  inc/          boot, session, throttle + api_books/rentals/users
sql/          schéma, číselníky, 60 knih, 14 uživatelů, generátor historie
cronjobs/      cleaner.php — úklid prošlých sezení a záznamů o pokusech
install.php    instalace z webu (maže se po sobě)
install.sh     instalace do tří domén
```

42 souborů, 360 kB. Výpůjčky se **generují** (`sql/gen-rentals.php`), takže
v balíku nejsou — dvě stě tisíc řádků historie by ho utopilo.

---

## Co je v tom naschvál

Tohle nejsou náhody a při úpravách to nechtěj „zjednodušit".

**Tři vrstvy jdou vždy po HTTP.** I když běží na jednom stroji. Kdyby BFF
sahalo na databázi, nedá se mezi ně dát jiné oprávnění a celé dělení je na
ozdobu.

**Backendové API nenačítá `fw.inc`.** Waggle je protokol mezi BFF a
prohlížečem; backend o něm vědět nemá, jinak by se nedal přepsat do
Pythonu, aniž by se s ním tahal i ten protokol. Bere si z knihovny jediný
soubor, `io.inc` — hygienu vstupů, kterou potřebuje stejně jako kdokoli
jiný. Obojí v jednom procesu být nesmí: sdílejí jména.

**Backend ověřuje sezení vždy, interně, u každého volání** kromě přihlášení.
Nevěří BFF, že už to udělalo. Ověřuje to `session_require()` a je to krok 3
z šesti, které má každý endpoint ve stejném pořadí:

```
1. vstupy   2. sémantika   3. sezení   4. oprávnění   5. práce   6. odpověď
```

Není to zvyk, je to podmínka. Jediná přípustná výjimka je ověření sezení
společné pro celý modul API.

**BFF je bez stavu**, až na šedesátisekundovou cache bloku o přihlášeném
člověku. Cache je jen proto, aby se `session_check` nevolal osmkrát za
obrazovku. Zápisový dialog si ji vynutí přeskočit — otevřít formulář nad
mrtvým sezením znamená, že člověk vyplní celou knihu a přijde o ni až při
ukládání.

**Kreslení není autorizace.** BFF se podle `may()` rozhoduje, co *nakreslit*.
O tom, co se *smí*, rozhoduje `acl_require()` na backendu — a rozhodne to i
pro klienta, který BFF nikdy neviděl.

**Chyby jdou přes HTTP status.** 400 vyplnění, 401 sezení, 403 oprávnění,
404 neexistuje, 429 příliš mnoho pokusů. Žádné `{"ok":false}` s dvěstěkou.

**Oprávnění jsou řádky v číselníku** `acls` plus `us_acl` jako seznam
oddělený čárkami. Primitivní schválně: přehledné a bez jointů. `ep_user_save()`
ukládá jen ta práva, která v číselníku opravdu jsou.

**Zápis říká záměr výslovně.** `book_save`, `genre_save` i `user_save`
dostávají `op=insert|update` a server ověří, že si to s id neodporuje.
Odvozovat záměr z přítomnosti id znamená, že ztracené id tiše založí
duplikát — a odpoví `ok`.

---

## Úklid

```
*/15 * * * * php /cesta/cronjobs/cleaner.php --api-config=/cesta/api/config.inc
```

Maže prošlá sezení a staré záznamy o nezdařených přihlášeních. Bez toho to
funguje dál, jen tabulky rostou.

---

## Na co si dát pozor

**`UNSAFE_DEMO` se nevypíná v konfiguraci.** Ten příznak nastavil instalák,
protože zjistil, že vrstvy nejsou oddělené. Přepnutím na `false` ten problém
nezmizí, jen ta informace o něm. Chceš-li ho zhasnout právem, nasaď přes
`install.sh`.

**`FW_DEBUG` posílá podrobnosti o chybách do prohlížeče**, včetně cest na
disku. Na provozu `false`.

**Demo hesla jsou `demo`.** Pokud jsi instalaci nechal na internetu, zruš je.

**Generovaná data jsou hodnověrná, ne pravdivá.** Jsou tam výpůjčky po
termínu, knihy, které se nikdy nevrátily, i čtenáři s osmi knihami
najednou — aby bylo na čem zkoušet filtry a statistiku.

---

Licence Apache 2.0, stejně jako Waggle. Knihy v `sql/03-books.sql` jsou
volná díla.
