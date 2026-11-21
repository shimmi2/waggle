/* app.js — lepidlo projektu. Framework nezná ani vzhled, ani stránky. */

/* Vlastní operace projektu: důkaz, že registr jde rozšířit zvenčí,
   a že ctx.origin nese informaci, odkud dávka přišla. */
Fw.register('sync_note', function (c, ctx) {
    var n = document.getElementById('sync_note');
    if (!n) return;
    n.textContent = (ctx && ctx.origin === 'broadcast')
                  ? 'změna přišla z jiného okna' : 'měníte v tomhle okně';
});

/* Poznámku rozesílá projekt sám — knihovna o ní nic neví. Do jiného okna
   projde jen díky rozšíření access.broadcast níže; výchozí tabulka vlastní
   operace z broadcastu nepouští. Fw.broadcast ji provede i tady (local). */
document.addEventListener('input', function (ev) {
    var el = ev.target;
    if (el.matches && el.matches('[data-fw-sync]')) Fw.broadcast([{ op: 'sync_note' }]);
});

Fw.init({ bff: '../bff/', debug: true, params: { skin: 'plain' },
          access: { broadcast: ['value', 'text', 'class', 'notify', 'sync_note'] } });
