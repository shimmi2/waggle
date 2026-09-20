/* =====================================================================
 *  app2.js — lepidlo mezi frameworkem a AdminLTE.
 *
 *  Celá integrace je 20 řádků, protože AdminLTE 4 má initialize()/
 *  teardown() postavené přesně pro tenhle typ částečných překreslení.
 *  teardown() odstřelí listenery přes AbortController, takže je
 *  initialize() idempotentní a nic se nezdvojí.
 * ===================================================================== */
(function () {
'use strict';

/* Jedna dávka může vyměnit tři divy — reinicializuj až jednou po ní. */
/* Co vůbec potřebuje oživit. Když v doručeném kusu nic takového není
   (a to je většina dávek — progress bar mění jen text v <span>), nemá
   smysl volat initialize(). Bez téhle podmínky se AdminLTE během
   progress baru reinicializoval 101x, 10x za sekundu. */
var LIVE = '[data-lte-toggle],[data-bs-toggle],.nav-treeview,.card-tools,.direct-chat';

var pending = null;
function reinit() {
    if (pending) return;
    pending = setTimeout(function () {
        pending = null;
        if (window.adminlte && adminlte.initialize) adminlte.initialize();
        document.querySelectorAll('[data-bs-toggle="tooltip"]').forEach(function (el) {
            if (!bootstrap.Tooltip.getInstance(el)) new bootstrap.Tooltip(el);
        });
        Fw.debug('AdminLTE reinicializován');
    }, 0);
}

/* Před výměnou zlikviduj instance Bootstrapu uvnitř — jinak zůstanou
   viset tooltipy a backdropy přilepené na <body>. */
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

/* ---------------------------------------------------------------------
 *  Hlášení přes Bootstrap toast. AdminLTE 4 vlastní toast nemá,
 *  staví na BS5. Ukázka toho, že hlášení frameworku je přepsatelné.
 * ------------------------------------------------------------------- */
var TOAST = {
    success: ['text-bg-success', 'bi-check-circle-fill',      5000],
    info:    ['text-bg-primary', 'bi-info-circle-fill',       5000],
    warning: ['text-bg-warning', 'bi-exclamation-triangle-fill', 7000],
    error:   ['text-bg-danger',  'bi-x-octagon-fill',         9000]
};

Fw.notify = function (kind, msg) {
    (kind === 'error' ? console.error : console.log)('[fw] ' + kind + ': ' + msg);
    var host = document.getElementById('fw_toasts');
    var cfg  = TOAST[kind] || TOAST.info;
    if (!host || !window.bootstrap) { if (kind === 'error') alert(msg); return; }

    var el = document.createElement('div');
    el.className = 'toast align-items-center ' + cfg[0] + ' border-0';
    el.setAttribute('role', 'alert');
    el.innerHTML = '<div class="d-flex"><div class="toast-body"><i class="bi ' + cfg[1]
                 + ' me-2"></i><span></span></div>'
                 + '<button type="button" class="btn-close btn-close-white me-2 m-auto" '
                 + 'data-bs-dismiss="toast" aria-label="Zavřít"></button></div>';
    el.querySelector('span').textContent = msg;     // textContent = žádné XSS

    host.appendChild(el);
    var t = new bootstrap.Toast(el, { delay: cfg[2] });
    el.addEventListener('hidden.bs.toast', function () { t.dispose(); el.remove(); });
    t.show();
};

Fw.error = function (msg) { Fw.notify('error', msg); };

/* Vlastní operace projektu — důkaz, že registr jde rozšířit zvenčí,
   a že ctx.origin nese informaci, odkud dávka přišla. */
Fw.register('sync_note', function (c, ctx) {
    var n = document.getElementById('sync_note');
    if (!n) return;
    var fromOther = ctx && ctx.origin === 'broadcast';
    n.className = 'badge ' + (fromOther ? 'text-bg-info' : 'text-bg-secondary');
    n.textContent = fromOther ? 'změna přišla z jiného okna' : 'měníte v tomhle okně';
});

Fw.init({ api: '../api/', debug: true, params: { skin: 'lte' } });
})();
