# 07 — Integrace šablony

## Obecný postup

Platí pro AdminLTE, Bootstrap Admin, Metronic i vlastní layout.

### 1. Shell je statický a nikdy se nepřekresluje

V `index.html` zůstane navbar, sidebar, footer a mřížka. Po drátě jdou
jen **vnitřky** pojmenovaných prvků.

Jména `left_menu`, `top_frame` a `main` jsou **zvyklost, ne požadavek**
— `sel` bere libovolný selektor a oken může být kolik chceš. Proč se to
takhle dělí a co z toho plyne pro řízení aplikace, je v
[06 — Aplikace](06-aplikace.md); tady je podstatné jen to, že ta jména
musí sedět na id v šabloně.

```html
<ul class="navbar-nav ms-auto" id="top_frame"></ul>
<ul class="nav sidebar-menu"   id="left_menu"></ul>
<div class="container-fluid"   id="main"></div>
```

Cíl nemusí být div — protokol adresuje selektorem, ne typem prvku.

Posílat celý `<body>` znamená reinicializovat celou šablonu při každém
kliknutí, ztratit scroll, stav sbaleného menu i focus. Dává to smysl jen
jako vzácná, vědomá **výměna layoutu**.

### 2. Widgety v doručeném obsahu oživit hooky

Většina šablon si při startu naváže listenery na prvky, které po výměně
`innerHTML` přestanou existovat.

```js
Fw.on('beforeReplace', function (el) { /* teardown uvnitř el */ });
Fw.on('afterReplace',  function (el) { /* init uvnitř el */ });
```

### 3. Reinicializovat jen když je co

Tohle se snadno přehlédne a stojí to výkon. Progress bar mění jen text
v jednom `<span>` — reinicializovat kvůli tomu celou šablonu stokrát
za deset sekund nemá smysl.

```js
var LIVE = '[data-lte-toggle],[data-bs-toggle],.nav-treeview,.card-tools';

Fw.on('afterReplace', function (el) {
    if (el.matches(LIVE) || el.querySelector(LIVE)) reinit();
});
```

### 4. Debounce na dávku

Jedna dávka může vyměnit tři prvky. Reinicializace má proběhnout jednou:

```js
var pending = null;
function reinit() {
    if (pending) return;
    pending = setTimeout(function () { pending = null; /* init */ }, 0);
}
```

### 5. Žádné `<script>` ve fragmentech

`innerHTML` je neprovede. Na spuštění kódu je příkaz `call`, poslaný
za příkazem `html`.

### 6. Vlastní hlášení chyb

```js
Fw.error = function (msg) { /* toast, modal, cokoli */ };
```

## AdminLTE 4

Vyšlo to nejlíp, protože AdminLTE 4 má `initialize()` / `teardown()`
postavené přesně pro tenhle typ částečných překreslení (cílí na Turbo).
`teardown()` odstřelí listenery přes `AbortController`, takže je volání
idempotentní a nic se nezdvojí.

Celá integrace je `app2/app2.js` — zhruba čtyřicet řádků:

```js
function reinit() { /* debounce */ adminlte.initialize(); /* + tooltipy */ }

Fw.on('beforeReplace', function (el) {
    el.querySelectorAll('[data-bs-toggle="tooltip"]').forEach(function (n) {
        var t = bootstrap.Tooltip.getInstance(n); if (t) t.dispose();
    });
    el.querySelectorAll('.modal').forEach(function (n) {
        var m = bootstrap.Modal.getInstance(n); if (m) m.dispose();
    });
});

Fw.on('afterReplace', function (el) {
    if (el.matches(LIVE) || el.querySelector(LIVE)) reinit();
});
```

`beforeReplace` je tam kvůli tomu, že Bootstrap věší backdropy modalů
a tooltipy přímo na `<body>` — bez úklidu by po výměně zůstaly sirotci.

### Co čtyřka nemá

AdminLTE 4 vyhodilo přibalené pickery, které měla trojka (tempusdominus,
bootstrap-datepicker). **Datepicker se nedoplňuje** — použij nativní
`<input type="date">`. Prohlížeč nakreslí kalendář sám, na mobilu
systémový, a podle specifikace posílá vždycky ISO `YYYY-MM-DD` bez ohledu
na jazyk prohlížeče. Locale se tím řešit nemusí vůbec.

### Dialog bez Bootstrap JS

Překryvné okno nepotřebuje `bootstrap.Modal`. Stačí CSS:

```html
<div class="modal-backdrop fade show"></div>
<div class="modal fade show d-block" tabindex="-1">
  <div class="modal-dialog modal-xl modal-dialog-centered">…</div>
</div>
```

Zavření = vyprázdnit okno, ve kterém to leží. Odpadá celý životní cyklus
Bootstrapu (instance, backdropy přilepené na `<body>`), který se s
částečným překreslováním snáší špatně.

Dvě věci k tomu:

- `.modal` má `width/height: 100%` a kreslí **přes** backdrop. Kliknutí
  na `.modal-backdrop` se k němu proto nedostane — zavírání „kliknutím
  mimo" musí viset na `.modal`, ne na backdropu.
- z-indexy: backdrop 1050, modal 1055, toasty 1090.

### Hledání v menu

Stačí jeden input, zbytek dělá AdminLTE. Filtruje i položky, které
přišly po drátě.

```html
<input type="search" data-lte-toggle="sidebar-search">
<div data-lte-search-empty>Nic nenalezeno</div>
```

### AdminLTE 3

Princip je stejný, ale v3 (jQuery + Bootstrap 4) lifecycle API nemá.
V `afterReplace` se musí ručně zavolat init konkrétních widgetů
(`$('[data-widget="treeview"]').Treeview('init')` a podobně).
Framework se nemění.

## Co si ohlídat u jakékoli šablony

| | |
|---|---|
| tooltipy a popovery | v injektovaném HTML se neinicializují samy |
| modaly | backdrop na `<body>` přežije výměnu obsahu |
| select2, DataTables | nutný teardown, jinak tečou listenery |
| CSS přechody | výměna prvku je zabije — měň atribut, ne kontejner |
| ikony jako fonty | cesta k fontům je relativní k CSS, ne k stránce |
