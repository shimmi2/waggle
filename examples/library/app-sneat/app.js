/* =====================================================================
 *  app.js — klientská část knihovny, varianta se šablonou Sneat
 *
 *  Proti app/app.js přibyl JEDINÝ blok: lepidlo na šablonu. Zbytek je
 *  znak po znaku totéž, protože se nemění aplikace, jen kabát.
 * ===================================================================== */
(function () {
'use strict';

/* ---------------------------------------------------------------------
 *  1) Vypršené sezení
 *
 *  Backend vrátí 401 a BFF ho předá jako příkaz error s kódem 401.
 *  Výchozí obsluha by jen ukázala červenou hlášku a uživatel by zůstal
 *  koukat na obrazovku, která už mu nepatří. Přepíšeme ji tedy tak, aby
 *  aplikace spadla zpátky na přihlášení.
 *
 *  Tohle je ten registr operací z kapitoly 04 — žádný zásah do
 *  frameworku, jen jiné chování jedné operace.
 * ------------------------------------------------------------------- */
Fw.register('error', function (c) {
    if (c.code === 401) {
        Fw.notify('error', c.message || 'Byl jste odhlášen.');
        Fw.session = null;
        try { localStorage.removeItem('fw_session'); } catch (e) {}
        /* Překryv musí zmizet PRVNÍ. Přihlašovací obrazovka se kreslí do
           okna main, takže otevřený dialog by zůstal ležet nad ní — se
           svým tmavým pozadím přes celou plochu. Uživatel by koukal na
           formulář, který se nikdy neuloží, a přihlásit se nemohl,
           protože login by byl pod tím.

           Je to cena za to, že okna jsou nezávislá: kdo překreslí jedno,
           musí si vzpomenout na ostatní. */
        Fw.clear('overlay');
        return Fw.send('index', {}, { history: false });
    }
    Fw.notify('error', (c.code ? c.code + ': ' : '') + c.message);
});

/* ---------------------------------------------------------------------
 *  2) Hlášení uživateli
 *
 *  Výchozí Fw.notify umí jeden box. Tady z toho děláme krátkodobé
 *  bubliny, protože po vrácení knihy jich může přijít víc za sebou.
 * ------------------------------------------------------------------- */
var box = document.getElementById('fw_error');
Fw.notify = function (kind, msg) {
    (kind === 'error' ? console.error : console.log)('[lib] ' + kind + ': ' + msg);
    if (!box) { if (kind === 'error') alert(msg); return; }
    var el = document.createElement('div');
    el.className = 'note note-' + kind;
    el.textContent = msg;                       // textContent = žádné XSS
    box.style.display = 'block';
    box.appendChild(el);
    setTimeout(function () {
        el.classList.add('gone');
        setTimeout(function () {
            el.remove();
            if (!box.children.length) box.style.display = 'none';
        }, 300);
    }, kind === 'error' ? 6000 : 3000);
};


/* ---------------------------------------------------------------------
 *  3) Lepidlo na šablonu
 *
 *  Tohle je ten postup z kapitoly 08. Šablona si při startu proleze DOM
 *  a navěsí se na to, co najde; když pak framework kus DOMu vymění,
 *  o výměně neví. Dvě věci je proto potřeba udělat ručně.
 *
 *  Pozor na pořadí: tooltip se musí ZRUŠIT před výměnou, ne až po ní.
 *  Bootstrap si drží odkaz na prvek, který už ve stránce nebude, a
 *  bublina by zůstala viset nad prázdným místem.
 * ------------------------------------------------------------------- */
Fw.on('beforeReplace', function (el) {
    if (!window.bootstrap) return;
    el.querySelectorAll('[data-bs-toggle="tooltip"]').forEach(function (n) {
        var t = bootstrap.Tooltip.getInstance(n);
        if (t) t.dispose();
    });
});

Fw.on('afterReplace', function (el) {
    if (window.bootstrap)
        el.querySelectorAll('[data-bs-toggle="tooltip"]').forEach(function (n) {
            if (!bootstrap.Tooltip.getInstance(n)) new bootstrap.Tooltip(n);
        });

    /* Menu má vlastní scrollbar, který počítá s výškou obsahu. Po výměně
       položek (tedy při přihlášení a odhlášení) ji musí přepočítat. */
    if (el.closest && el.closest('#layout-menu') && window.Helpers) {
        try { window.Helpers.mainMenu && window.Helpers.mainMenu.update(); } catch (e) {}
    }
});

/* Adresa BFF přichází z config.js, který vyrobil instalák. Kdyby chyběl,
   je lepší to říct nahlas než tiše volat vlastní adresu a dostávat 404. */
if (!window.LIB_BFF_URL) {
    document.getElementById('main').innerHTML =
        '<div class="card"><h2>Chybí konfigurace</h2><p>Nenašel jsem ' +
        '<code>config.js</code> s adresou BFF. Zkopíruj ' +
        '<code>config.example.js</code> a doplň ji, nebo pusť instalák.</p></div>';
} else {
    Fw.init({ bff: window.LIB_BFF_URL,
              access: window.LIB_FW_ACCESS || undefined });
}

})();
