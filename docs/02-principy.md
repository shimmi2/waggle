# 02 — Principy

Dvanáct pravidel. Většina z nich existuje proto, aby bylo možné později
přidat featuru bez přepisování toho, co už běží.

## 1. Server rozhoduje, klient vykonává

Klient nemá stav aplikace, nemá router a neví, co která funkce znamená.
Umí jediné: vzít dávku příkazů a aplikovat ji. Veškerá logika je v PHP.

## 2. Cíl je vždy selektor

Každý příkaz míří na `sel`, což je CSS selektor. Holé slovo se normalizuje
na `#id`, cokoli jiného se předá `querySelectorAll` — a aplikuje se na
**všechny** shody.

Kdyby cíl byl „id divu", nešlo by později adresovat formuláře, atributy
ani jednotlivé prvky. Takhle je budoucí rozšíření jen **nový `op`**.

## 3. `sel` znamená právě jednu věc

Nikdy se nepoužije pro nic jiného než DOM selektor. Proto má `subscribe`
pole `name`, ne `sel`, i když by se to nabízelo.

## 4. Žádný příkaz se neodkazuje na „prvek, který akci vyvolal"

Dávka může přijít z odpovědi API, z jiného okna nebo pushem ze serveru.
V posledních dvou případech žádný spouštěč neexistuje. Cokoli relativního
musí klient přeložit na explicitní selektor **při odesílání**.

Tohle je nejsnáz porušitelné pravidlo a jeho porušení rozbije push.

## 5. Jeden formát, čtyři transporty

`fetch`, NDJSON stream, `BroadcastChannel` mezi okny, SSE push. Všechny
doručují **tutéž dávku** a končí ve stejném dispatcheru. Liší se jen
`ctx.origin`. Přidat pátý transport znamená napsat čtečku, ne protokol.

## 6. Příkazy se aplikují v pořadí pole

Striktně sekvenčně, s `await`. `session` musí platit dřív, než se odpálí
požadavek za ní.

## 7. Celý řetězec je asynchronní

Dispatcher, guardy i operace vracejí promise. Je to tak od prvního řádku
kvůli budoucí featuře „máte neuložená data, opravdu odejít?" — ta potřebuje
počkat na dialog uprostřed aplikace dávky. Dodělat asynchronnost později
by znamenalo přepsat všechno.

## 8. Odpověď je atomická

`send_answer()` jen řadí do fronty. Dokud skript nedoběhne, neodešel ani
bajt, takže do posledního okamžiku lze vrátit poctivý HTTP status.
Buď se povede všechno, nebo se nepošle nic.

## 9. Chyba na vstupu, ne v částech výstupu

Endpoint nejdřív všechno ověří a udělá, teprve pak posílá příkazy.
Nikdy nepošle „překresli menu" a pak zjistí, že uživatel nemá oprávnění.

## 10. Framework garantuje hygienu, ne bezpečnost

`req()` zaručí skalární string omezené délky bez NUL. Nic víc.
Escapování je vlastnost **cíle**, ne hodnoty — patří do místa použití.
Viz [09 — Bezpečnost](09-bezpecnost.md).

## 11. Měň prvky, ne kontejnery

Progress bar sahá na dva atributy a jeden `<span>`. Kdyby překresloval celý
`#main`, zabil by CSS přechod, reinicializoval šablonu stokrát za deset
sekund a zahodil rozepsaný obsah polí jinde v divu.

Platí obecně: nejmenší cíl, který stačí.

## 12. Framework neví nic o projektu ani o technologii kolem

Žádná databáze, žádné šablony, žádná autentizace. Vzhled, stránky
i přihlášení jsou v `api/` a v aplikaci. Proto může `fw.js` obsluhovat
holé HTML i AdminLTE beze změny jediného řádku.

Totéž platí o technologiích. Tvrdá závislost je **jedna** — HTML, CSS
a JavaScript v prohlížeči. Volitelná je **jedna** — nchan, a jen kvůli
pushi. AdminLTE je příklad, PHP je referenční implementace serveru,
naše projekty do repozitáře nepatří vůbec. Příklady a nástroje si smí
dovolit cokoli, protože je nikdo nemusí použít; knihovna ne.
Rozvedeno v [01 — Motivace](01-motivace.md).
