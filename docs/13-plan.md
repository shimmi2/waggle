# 13 — Co je v plánu

## Framework zůstane jednoduchý

To je podmínka, ne ambice. Knihovna jsou tři soubory a nic z toho, co
následuje, se do nich nedostane. Hranice z
[01 — Motivace](01-motivace.md) platí dál: tvrdá závislost je jediná
(HTML, CSS, JavaScript), volitelná jedna (nchan).

Co se rozroste, jsou **příklady**. Z `examples/library` se postupně stane
větší systém — ne proto, aby ho někdo **musel** nasadit celý, ale aby si
z něj šlo brát po částech. Na nic z toho není projekt vázaný: co si
nevezmete, to tam prostě nebude, a cokoli jiného vám tam agent zasadí
podle vašeho zadání.

## Komponenty, ne framework

Plánuje se sada hotových stavebních dílů, které dnes musí každý projekt
napsat znovu: **mapy, statistiky a grafy, platební brány** a podobné
agendy, kde je devadesát procent práce pokaždé stejných.

Pravidlo, které u nich platí od začátku: **všechno musí být
oddělitelné.** Komponenta smí předpokládat protokol a smí předpokládat
konvenci endpointu, ale nesmí předpokládat jinou komponentu. Kdo chce jen
mapy, vezme si mapy a nic dalšího mu do projektu nepřijde.

Rozšiřují se i šablony pro rozjezd, ve dvou variantách podle toho, jak je
projekt postavený:

* **Dvouvrstvý model**, kam obvykle dospěje převod dvacet let starého
  monolitu: frontend a BFF, které sahá rovnou na data. Samostatná datová
  vrstva by u něj byla práce navíc bez užitku.
* **Třívrstvý model**, který je v [12 — Nový projekt](12-novy-projekt.md)
  popsaný jako výchozí: frontend, BFF a pod ním čisté datové API, sdílené
  i s jinými klienty — mobilními aplikacemi a podobně.

## Integrace s AI

Největší kus plánované práce. Vychází ze zkušeností autora z nasazení u
velkého telekomunikačního operátora. Model je pochopitelně to první, bez
čeho se agent nehne — ale sám o sobě z něj užitečného pomocníka neudělá.
Rozdíl mezi hračkou a nástrojem dělá **přístup k datům a k obrazovce**:
k tomu, co firma opravdu ví, a k možnosti výsledek rovnou ukázat.

### MCP server nad API

Endpointy aplikace se agentům zpřístupní přes MCP server. Ten se staví nad
už existující konvenci endpointu — jméno funkce, vstupy přes `in_*`,
oprávnění přes ACL — takže popis nástrojů pro agenta vzniká z toho, co
v projektu stejně je, a ne jako druhá, ručně udržovaná definice.

Oprávnění se tím **nemění ani neobcházejí**. Agent volá tytéž endpointy
jako prohlížeč, se sezením toho, kdo se ptá, a dostane 403 na totéž, na co
by ho dostal člověk.

### Vlastní miniagent

K tomu malý agent, který umí zapojit náš MCP server, cizí MCP servery,
skilly a v omezené míře další nástroje. Záměrně **malý**: není to
konkurence velkým agentním prostředím, je to to, co se dá zabudovat do
firemní aplikace a provozovat bez dramatu.

### Agentický pomocník v aplikaci

Cíl, ke kterému obojí směřuje: do projektů půjde vložit pomocníka, který
umí prohledat naše i cizí data, něco udělat — a pomocí frameworku
**přesměrovat rovnou UI klienta** na výsledek.

Tohle je ta část, kde Waggle dává agentovi něco, co jinde chybí. Agent
nemusí odpověď popisovat slovy a nechat člověka, aby si ji naklikal. Umí
poslat tytéž příkazy, jaké posílá kterýkoli endpoint — takže výstupem
agenta může být **hotová obrazovka**, ne odstavec textu.

Ukázkový průběh nad knihovnou z `examples/library`:

```
uživatel:  „Najdi knihy o lásce."

agent:     zavolá books_list přes MCP, projde výsledky, vybere
           relevantní

           pošle příkaz html do okna main   -> seznam nalezených knih
           pošle příkaz notify              -> krátké shrnutí
           do reportu připíše komentář, proč vybral zrovna tyhle
```

Uživatel se nedívá na popis výsledku. Dívá se na výsledek — na téže
obrazovce, se stejným filtrem a stejnými odkazy, jaké by dostal, kdyby si
ho vyklikal sám. Komentář agenta jde vedle toho, ne místo toho.

## Co se tím nemění

* Protokol zůstává `v1`. Nic z výše uvedeného si nevyžádá nový příkaz;
  agent posílá tytéž příkazy jako server.
* Oprávnění zůstávají na backendu. Agent je klient jako každý jiný.
* Knihovna zůstává oddělitelná. Kdo nechce nic z téhle kapitoly, nedostane
  z ní do projektu ani řádek.
