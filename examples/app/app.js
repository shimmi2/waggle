/* app.js — lepidlo projektu. Framework nezná ani vzhled, ani stránky. */

/* Vlastní operace projektu: důkaz, že registr jde rozšířit zvenčí,
   a že ctx.origin nese informaci, odkud dávka přišla. */
Fw.register('sync_note', function (c, ctx) {
    var n = document.getElementById('sync_note');
    if (!n) return;
    n.textContent = (ctx && ctx.origin === 'broadcast')
                  ? 'změna přišla z jiného okna' : 'měníte v tomhle okně';
});

Fw.init({ api: '../api/', debug: true, params: { skin: 'plain' } });
