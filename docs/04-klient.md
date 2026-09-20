# 04 — Klient (`fw.js`)

Jeden soubor, žádné závislosti, žádný build. Tři vrstvy, záměrně oddělené:

```
transport  (fetch | stream | broadcast | push | local)  →  dávky
dispatch   (pořadí, guardy)                             →  příkazy
registr    (Fw.ops)                                     →  DOM
```

Transport se dá vyměnit, aniž by se dotkl dispatcheru. Operace se dá přidat,
aniž by se dotkla jádra.

## Inicializace

```js
Fw.init({
    api:     '../api/',            // jediná povinná informace
    debug:   true,                 // logování do konzole
    params:  { skin: 'lte' },      // přidá se ke KAŽDÉMU požadavku
    channel: 'fw:moje-app'         // kanál mezi okny; false = vypnout
});
```

Bez `channel` se použije `'fw:' + location.pathname`, aby si dvě aplikace
na stejném hostu nepřepisovaly stav.

`init()` vyprázdní a schová `#fw_error`, vyzvedne sériové číslo instalace,
obnoví session z `localStorage`, naváže posluchače a pošle `index`.

## Odesílání

```js
Fw.send('show_user', { id: 42 });            // objekt
Fw.send('save', new FormData(formElement));  // nebo FormData
Fw.send('nav', {}, { history: false });      // bez záznamu do historie
```

## Události

Delegace na `document`, takže se nic nepřevazuje po překreslení.

```html
<a href="#?function=show_user&id=42" data-fw>Detail</a>
<form data-fw action="#?function=save_form" method="post">…</form>
<input type="range" id="s1" data-fw-sync>   <!-- synchronizace mezi okny -->

<a href="#?function=smazat&id=7" data-fw data-confirm="Opravdu smazat?">Smazat</a>
<select name="filtr" data-fw-submit>…</select>   <!-- odešle formulář při změně -->

<button data-fw-clear="#overlay">Zavřít</button>  <!-- vyprázdní okno, bez serveru -->
<div data-fw-esc="#overlay">…</div>               <!-- a totéž udělá Escape -->
```

`data-fw-clear` je jediný atribut, který nejde na server: vyprázdní cílové
okno na místě. Zavřít dialog je stav klienta, ne aplikace, a round trip jen
kvůli prázdnému divu by byl zbytečný. Prochází stejnou cestou jako operace
`html`, takže se uklidí i to, co je uvnitř (viz hook `beforeReplace`).
`data-fw-esc` na obalu dělá totéž na klávesu Escape.

`data-confirm` na odkazu nebo formuláři se zeptá před odesláním; prázdná
hodnota použije obecný text.

Parametry se čtou z `href` / `action` za `?` nebo `#`. `function` je název
endpointu, všechno ostatní jsou parametry — nikde se nedeklarují.

U formulářů se navíc přibalí `new FormData(form)`, takže soubory fungují samy.

Proč `data-fw` a ne `onclick`: parametr s apostrofem nerozbije stránku,
budoucí featury (spinner, hlídání neuložených dat) se přidají na jednom
místě, a funguje to pod přísnou CSP. `Fw.send()` zůstává jako úniková cesta.

## Sériové číslo instalace

`X-App-Serial` se drží v `localStorage` i v cookie. Když neexistuje, vyrobí
se z `crypto.randomUUID()`, případně z `getRandomValues`, v nouzi z času
a náhody. Demo na něj váže session, takže odcizený token na jiném zařízení
neprojde.

## Historie

Příkaz `history` udělá `pushState` s popisem požadavku. Při `popstate` se
požadavek **přehraje**. Po odeslání formuláře se `history` ignoruje —
přehrávat POST tlačítkem Zpět je vždycky špatně.

## Mezi okny

```js
Fw.broadcast(cmds);         // tohle okno i všechna ostatní
Fw.broadcast(cmds, false);  // jen ostatní okna
Fw.local(cmds);             // jen tohle okno
```

`BroadcastChannel` zprávu nedoručí zpět odesílateli, takže smyčka nemůže
vzniknout. Nastavení `.value` nevyvolá událost `input`, takže se
synchronizace neodrazí zpátky.

## Push (SSE)

Otevírá ho **server** příkazem `subscribe`, aplikace nepotřebuje konfiguraci.

```js
Fw.subscribe('main', 'https://host:8443/nchan/sub', 'token…');
Fw.unsubscribe('main');
```

`name` je lokální klíč, takže lze poslouchat víc kanálů i víc serverů
zároveň. Znovupřipojení po výpadku řeší `EventSource` sám včetně
`Last-Event-ID`. Token jde v URL, protože `EventSource` neumí vlastní
hlavičky — proto musí být jednoúčelový, nikdy session.

## Hooky

```js
Fw.on('beforeReplace', function (el) { /* teardown widgetů uvnitř */ });
Fw.on('afterReplace',  function (el) { /* init widgetů */ });
```

Jediné místo, kde se mění obsah prvku, je `Fw.setHtml()` — proto stačí
navěsit se sem a pokrýt všechny operace. Viz [07 — Integrace](07-integrace.md).

## Guardy

```js
Fw.guards.push(async function (cmd, ctx) {
    if (cmd.sel === '#main' && jsouNeulozenaData())
        return confirm('Opravdu opustit?');   // false = příkaz se zahodí
});
```

Guard běží před každou operací a smí být asynchronní. Kvůli tomu je celý
dispatcher `async` od začátku.

## Hlášení

```js
Fw.notify('success', '…');   // success | info | warning | error
Fw.error('…');               // zkratka pro notify('error', …)
Fw.warn('…');                // konzole, jen při debug
Fw.debug('…', data);         // konzole, jen při debug
```

Výchozí implementace píše do `#fw_error` a přidá třídu `fw-note-<kind>`,
takže barvu řeší CSS. `app2.js` ukazuje, jak `Fw.notify` nahradit
Bootstrap toastem s ikonou a dobou zobrazení podle druhu.

## Přehled API

| | |
|---|---|
| `Fw.init(cfg)` | start |
| `Fw.send(fn, params, opts)` | požadavek na API |
| `Fw.local(cmds)` / `Fw.broadcast(cmds, self)` | lokální / mezi okny |
| `Fw.subscribe(name, url, token)` / `Fw.unsubscribe(name)` | push |
| `Fw.register(op, fn)` | vlastní operace |
| `Fw.on(hook, fn)` / `Fw.guards` | lifecycle a guardy |
| `Fw.nodes(sel)` | normalizovaný `querySelectorAll` |
| `Fw.dispatch(batch, ctx)` / `Fw.line(text, ctx)` | ruční vstup do pipeline |
