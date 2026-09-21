/* =====================================================================
 *  fw.js — klientská část Waggle
 *  https://github.com/shimmi2/waggle
 *
 *  Tři vrstvy, záměrně oddělené:
 *    transport  (fetch / v budoucnu WebSocket / lokální)  ->  dávky
 *    dispatch   (pořadí, guardy)                          ->  příkazy
 *    ops        (registr operací)                         ->  DOM
 *
 *  Celá cesta je async, aby šlo později vložit guard s dialogem
 *  ("máte neuložená data") bez přepisování dispatcheru.
 * ===================================================================== */
(function (global) {
'use strict';

var RE_WORD  = /^[A-Za-z0-9_-]+$/;
var FW_V     = 1;          // verze protokolu na drátě
var RELEASE  = '1.2.1';    // vydání knihovny, mění se nezávisle na protokolu

var Fw = {
    release: RELEASE,
    cfg:     { api: '../api/', debug: true, channel: null, params: null },
    session: null,        // token; server ho mění příkazem {"op":"session"}
    serial:  null,        // trvalá identifikace instalace v prohlížeči
    ops:     {},          // registr operací
    guards:  [],          // async(cmd, ctx) -> false zruší příkaz (budoucí featura)
    hooks:   { beforeReplace: [], afterReplace: [] },
    _seq:    0
};

/* ---------------------------------------------------------------- DOM */

/* Normalizace cíle: holé slovo = id, cokoli jiného = CSS selektor.
   Vždy pracujeme se VŠEMI shodami, ne jen s první. */
Fw.nodes = function (sel) {
    if (!sel) return [];
    var q = RE_WORD.test(sel) ? '#' + sel : sel, list;
    try { list = document.querySelectorAll(q); }
    catch (e) { Fw.warn('neplatný selektor: ' + sel); return []; }
    if (!list.length) Fw.warn('selektor nic nenašel: ' + sel);
    return Array.prototype.slice.call(list);
};

Fw.on = function (hook, fn) { (Fw.hooks[hook] || (Fw.hooks[hook] = [])).push(fn); };
Fw.emit = function (hook, el) {
    var h = Fw.hooks[hook] || [];
    for (var i = 0; i < h.length; i++) { try { h[i](el); } catch (e) { Fw.warn(e.message); } }
};

/* Jediné místo, kde se mění obsah prvku — sem se bude věšet
   teardown/init cizích widgetů (AdminLTE). */
Fw.setHtml = function (el, html) {
    Fw.emit('beforeReplace', el);
    el.innerHTML = html;
    Fw.emit('afterReplace', el);
};

/* --------------------------------------------------------- REGISTR OPS */

Fw.register = function (op, fn) { Fw.ops[op] = fn; };

Fw.register('html', function (c) {
    Fw.nodes(c.sel).forEach(function (el) { Fw.setHtml(el, c.content || ''); });
});

Fw.register('url', function (c) {
    return Fw.fragment(c.url).then(function (html) {
        Fw.nodes(c.sel).forEach(function (el) { Fw.setHtml(el, html); });
    });
});

Fw.register('append', function (c) {
    Fw.nodes(c.sel).forEach(function (el) { el.insertAdjacentHTML('beforeend', c.content || ''); });
});

Fw.register('remove', function (c) {
    Fw.nodes(c.sel).forEach(function (el) { el.parentNode && el.parentNode.removeChild(el); });
});

Fw.register('attr', function (c) {
    Fw.nodes(c.sel).forEach(function (el) {
        if (c.value === null) el.removeAttribute(c.name); else el.setAttribute(c.name, c.value);
    });
});

Fw.register('value', function (c) {
    Fw.nodes(c.sel).forEach(function (el) { el.value = c.value; });
});

Fw.register('class', function (c) {
    Fw.nodes(c.sel).forEach(function (el) {
        (c.add    || []).forEach(function (x) { el.classList.add(x); });
        (c.remove || []).forEach(function (x) { el.classList.remove(x); });
        (c.toggle || []).forEach(function (x) { el.classList.toggle(x); });
    });
});

Fw.register('session', function (c) {
    Fw.session = c.value || null;
    try {
        if (Fw.session) localStorage.setItem('fw_session', Fw.session);
        else            localStorage.removeItem('fw_session');
    } catch (e) {}
    Fw.debug('session ' + (Fw.session ? 'nastavena' : 'zrušena'));
});

Fw.register('history', function (c, ctx) {
    if (!ctx || ctx.history === false) { Fw.debug('history přeskočena'); return; }
    history.pushState({ fw: { fn: ctx.fn, params: ctx.params } }, '', c.url || location.href);
});

Fw.register('call', function (c) {
    var fn = Fw.resolve(c.fn);
    if (!fn) { Fw.warn('call: neznámá funkce ' + c.fn); return; }
    return fn.apply(null, c.args || []);
});

/* Hlášení uživateli. kind: success | info | warning | error */
Fw.register('notify', function (c) { Fw.notify(c.kind || 'info', c.message); });

/* error je notify druhu 'error' s volitelným HTTP kódem. */
Fw.register('error', function (c) { Fw.notify('error', (c.code ? c.code + ': ' : '') + c.message); });
Fw.register('debug', function (c) { Fw.debug(c.message, c.data); });

/* ------------------------------------------------- ZANEPRÁZDNĚNO

   Překryv „pracuji" nad libovolným prvkem. Na drátě jedna operace:

       {"op":"busy","sel":"main","text":"Připravuji…"}
       {"op":"busy","sel":"main","text":"Počítám…","pct":40}
       {"op":"busy","sel":"main","state":"done","text":"Hotovo"}
       {"op":"busy","sel":"main","state":"off"}

   Chybějící state znamená „běží". Chybějící pct znamená kolečko;
   jakmile pct přijde, kolečko se promění v pruh — proto to nejsou dva
   typy, ale jeden s volitelným číslem.

   Rozděleno na dvě půlky ze stejného důvodu jako notify: Fw.busy je
   řadič (účetnictví, prodlevy, úklid) a nepřepisuje se, Fw.busyRender
   je vykreslení a přepsat se má. Projekt si tak nasadí vlastní vzhled,
   aniž by musel znovu řešit, kdy se co objeví a zmizí.

   Tři věci, které tohle dělá a zvenčí by se dělaly špatně:

   1. Jedno okno = jeden překryv. Volající proto může posílat
      „připravuji / počítám / dokončuji" za sebou bez jakéhokoli stavu
      na klientovi.
   2. Cokoli jiného, co do toho okna přijde, překryv sundá. Hlídá se to
      v Fw.apply, tedy v dispatcheru — a právě proto to patří dovnitř
      frameworku a ne do projektu.
   3. Prvních BUSY_DELAY ms se nekreslí nic. Když operace doběhne za
      osminu vteřiny, problikne jen záblesk a ten působí hůř než nic. */

var BUSY_DELAY = 300;    // než se překryv vůbec ukáže
var BUSY_DONE  = 700;    // jak dlouho svítí „hotovo", než zmizí

Fw._busy = [];

function busyRec(host)  { for (var i=0;i<Fw._busy.length;i++) if (Fw._busy[i].host===host) return Fw._busy[i]; return null; }
function busyDrop(r) {
    clearTimeout(r.timer); clearTimeout(r.doneTimer);
    if (r.box && r.box.parentNode) r.box.parentNode.removeChild(r.box);
    if (r.prevPos !== null && r.host) r.host.style.position = r.prevPos;
    var i = Fw._busy.indexOf(r); if (i >= 0) Fw._busy.splice(i, 1);
}

/* Sundá překryv nad prvkem i nad čímkoli, co v něm leží. Volá se
   z dispatcheru a z beforeReplace, takže překryv zmizí i tehdy, když
   někdo překreslí celé okno. */
Fw.busyClear = function (node) {
    for (var i = Fw._busy.length - 1; i >= 0; i--) {
        var r = Fw._busy[i];
        if (!r.host || !document.contains(r.host)) { busyDrop(r); continue; }
        if (node && (r.host === node || node.contains(r.host))) busyDrop(r);
    }
};

/* Vykreslení. Přepsatelné — dostane hostitele, vlastní box a stav.
   Box je prázdný jen při prvním volání; potom se jen aktualizuje. */
Fw.busyRender = function (host, box, st) {
    var bar = box.querySelector('.fw-busy-bar'), spin = box.querySelector('.fw-busy-spin'),
        txt = box.querySelector('.fw-busy-text');
    if (!txt) {
        /* Obsah je ve vnitřním pásu, ne přímo v překryvu — viz .fw-busy-in
           v busyStyle(). Ten pás se drží ve viditelné části i u divu
           vyššího, než je obrazovka. */
        box.innerHTML = '<div class="fw-busy-in">'
                      + '<div class="fw-busy-spin"></div>'
                      + '<div class="fw-busy-track"><div class="fw-busy-bar"></div></div>'
                      + '<div class="fw-busy-text"></div>'
                      + '</div>';
        bar  = box.querySelector('.fw-busy-bar');
        spin = box.querySelector('.fw-busy-spin');
        txt  = box.querySelector('.fw-busy-text');
    }
    var mapct = typeof st.pct === 'number';
    spin.style.display = mapct ? 'none' : 'block';
    box.querySelector('.fw-busy-track').style.display = mapct ? 'block' : 'none';
    if (mapct) bar.style.width = Math.max(0, Math.min(100, st.pct)) + '%';
    txt.textContent = st.text || '';
    box.className = 'fw-busy' + (st.state === 'done' ? ' fw-busy-done' : '');
};

/* Řadič. sel = cíl, st = {text, pct, state}. */
Fw.busy = function (sel, st) {
    st = st || {};
    var hosts = Fw.nodes(sel || 'main');
    if (!hosts.length) return;

    hosts.forEach(function (host) {
        var r = busyRec(host);

        if (st.state === 'off') { if (r) busyDrop(r); return; }

        if (st.state === 'done') {
            /* Nic se nestihlo ukázat -> nic neblikne. */
            if (!r || !r.box) { if (r) busyDrop(r); return; }
            clearTimeout(r.timer);
            Fw.busyRender(host, r.box, { text: st.text, pct: st.pct, state: 'done' });
            clearTimeout(r.doneTimer);
            r.doneTimer = setTimeout(function () { busyDrop(r); }, BUSY_DONE);
            return;
        }

        if (!r) {
            r = { host: host, box: null, timer: null, doneTimer: null, prevPos: null, st: {} };
            Fw._busy.push(r);
        }
        /* Text a procenta se slučují, aby šlo poslat jen nové pct
           a text zůstal viset. */
        if (st.text !== undefined) r.st.text = st.text;
        if (st.pct  !== undefined) r.st.pct  = st.pct;

        if (r.box) { Fw.busyRender(host, r.box, r.st); return; }
        if (r.timer) return;                       // čeká se na prodlevu

        r.timer = setTimeout(function () {
            r.timer = null;
            if (!document.contains(host)) { busyDrop(r); return; }
            /* Překryv se kotví k hostiteli, ten proto nesmí být static. */
            var pos = getComputedStyle(host).position;
            if (pos === 'static') { r.prevPos = host.style.position; host.style.position = 'relative'; }
            r.box = document.createElement('div');
            r.box.className = 'fw-busy';
            host.appendChild(r.box);
            Fw.busyRender(host, r.box, r.st);
        }, BUSY_DELAY);
    });
};

Fw.register('busy', function (c) { Fw.busy(c.sel, c); });

/* Překreslení obsahu prvku překryv ruší. Pokrývá html i url, protože
   obojí jde přes Fw.setHtml. */
Fw.on('beforeReplace', function (el) { Fw.busyClear(el); });

/* Výchozí vzhled. Vloží se jednou a projekt ho může přebít vlastním
   CSS — třídy jsou stabilní součást rozhraní. */
Fw.busyStyle = function () {
    if (document.getElementById('fw_busy_css')) return;
    var st = document.createElement('style');
    st.id = 'fw_busy_css';
    st.textContent =
      /* Překryv kryje celý prvek, ALE obsah sedí ve vnitřním pásu, který
         je position:sticky. U divu vyššího než obrazovka by jinak text
         přistál v jeho geometrickém středu, tedy klidně mimo výřez —
         uživatel by koukal na rozostřenou plochu bez jediné informace.

         Sticky to řeší bez toho, aby framework musel cokoli měřit:
         prohlížeč sám drží pás u horní hrany výřezu a zároveň ho nepustí
         mimo překryv. Když je div kratší než obrazovka, min() vrátí 100 %
         a chová se to jako dřív — vycentrováno v divu. Když je delší,
         vrátí výšku obrazovky a centruje se v tom, co je vidět.
         Žádné počítání scrollu, žádný resize listener. */
      '.fw-busy{position:absolute;inset:0;z-index:1050;'
    + 'background:rgba(255,255,255,.72);backdrop-filter:blur(1px);'
    + 'font:500 14px/1.4 system-ui,sans-serif;color:#333}'
    + '.fw-busy-in{position:sticky;top:0;height:min(100%,100vh);'
    + 'display:flex;flex-direction:column;align-items:center;justify-content:center;gap:.6rem}'
    + '@supports (height:100dvh){.fw-busy-in{height:min(100%,100dvh)}}'
    + '@media (prefers-color-scheme:dark){.fw-busy{background:rgba(24,24,27,.72);color:#eee}}'
    + '.fw-busy-spin{width:34px;height:34px;border:3px solid currentColor;border-top-color:transparent;'
    + 'border-radius:50%;opacity:.55;animation:fw-busy-rot .8s linear infinite}'
    + '@keyframes fw-busy-rot{to{transform:rotate(360deg)}}'
    + '.fw-busy-track{width:min(260px,70%);height:8px;border-radius:4px;'
    + 'background:currentColor;opacity:.25;overflow:hidden}'
    + '.fw-busy-bar{height:100%;width:0;border-radius:4px;background:#0e6a72;'
    + 'transition:width .25s ease}'
    + '.fw-busy-done .fw-busy-spin{animation:none;border-top-color:currentColor;color:#198754}'
    + '.fw-busy-done .fw-busy-bar{background:#198754;width:100%}'
    + '.fw-busy-done .fw-busy-text{color:#198754}'
    + '@media (prefers-reduced-motion:reduce){.fw-busy-spin{animation:none}'
    + '.fw-busy-bar{transition:none}}';
    document.head.appendChild(st);
};

Fw.resolve = function (path) {
    var p = String(path).split('.'), o = global;
    for (var i = 0; i < p.length && o; i++) o = o[p[i]];
    return typeof o === 'function' ? o : null;
};

/* ----------------------------------------------------------- DISPATCH */

Fw.apply = function (cmd, ctx) {
    var fn = Fw.ops[cmd.op];
    if (!fn) { Fw.warn('neznámá operace: ' + cmd.op); return Promise.resolve(); }
    var chain = Promise.resolve(true);
    Fw.guards.forEach(function (g) {
        chain = chain.then(function (ok) { return ok === false ? false : g(cmd, ctx); });
    });
    return chain.then(function (ok) {
        if (ok === false) { Fw.debug('guard zrušil ' + cmd.op); return; }
        /* Přijde-li do okna cokoli jiného než busy, překryv končí. Tohle
           je ten důvod, proč busy patří do frameworku: zvenčí by se to
           dalo udělat jen opičí záplatou na Fw.ops.*. */
        if (cmd.op !== 'busy' && cmd.sel && Fw._busy.length)
            Fw.nodes(cmd.sel).forEach(function (el) { Fw.busyClear(el); });
        return fn(cmd, ctx);
    });
};

/* Příkazy se aplikují STRIKTNĚ v pořadí pole — session musí platit
   dřív, než se odpálí fetch fragmentu za ní. */
Fw.dispatch = function (batch, ctx) {
    var cmds = (batch && batch.cmds) || [];
    if (!batch || !batch.cmds) { Fw.warn('dávka bez cmds'); return Promise.resolve(); }
    return cmds.reduce(function (p, cmd) {
        return p.then(function () { return Fw.apply(cmd, ctx); });
    }, Promise.resolve());
};

/* Lokálně generovaná dávka — stejná cesta jako ze sítě. */
Fw.local = function (cmds) {
    return Fw.dispatch({ v: FW_V, cmds: cmds }, { origin: 'local', seq: ++Fw._seq });
};

/* ------------------------------------------------------- MEZI OKNY */

/* Třetí transport vedle fetch a (budoucího) push. Stejné dávky, stejný
   dispatcher — liší se jen ctx.origin. Prohlížeč zprávu nedoručí zpět
   odesílateli, takže nemůže vzniknout smyčka. */

Fw.channel = null;

Fw.openChannel = function (name) {
    if (Fw.channel) { Fw.channel.close(); Fw.channel = null; }
    if (!name || typeof BroadcastChannel === 'undefined') return;
    Fw.channel = new BroadcastChannel(name);
    Fw.channel.onmessage = function (ev) {
        Fw.dispatch(ev.data, { origin: 'broadcast', seq: ++Fw._seq });
    };
    Fw.debug('kanál mezi okny: ' + name);
};

/* Rozešle dávku do ostatních oken. includeSelf=false = jen ostatním. */
Fw.broadcast = function (cmds, includeSelf) {
    var batch = { v: FW_V, cmds: cmds };
    if (Fw.channel) Fw.channel.postMessage(batch);
    else Fw.warn('broadcast bez otevřeného kanálu');
    return includeSelf === false ? Promise.resolve()
                                 : Fw.dispatch(batch, { origin: 'local', seq: ++Fw._seq });
};

/* ------------------------------------------------------ PUSH (SSE) */

/* Čtvrtý transport. Server o něm rozhoduje příkazem subscribe, takže
   aplikace nepotřebuje žádnou konfiguraci — přihlášení odběr zapne,
   odhlášení ho zruší.

   EventSource neumí posílat vlastní hlavičky, proto token jde v URL.
   Musí to být jednoúčelový token, nikdy session. Znovupřipojení po
   výpadku řeší EventSource sám, včetně Last-Event-ID. */

Fw.subs = {};                      // name -> EventSource

Fw.subscribe = function (name, url, token) {
    if (!name || !url) { Fw.warn('subscribe bez name/url'); return; }
    Fw.unsubscribe(name);
    if (typeof EventSource === 'undefined') { Fw.warn('EventSource není k dispozici'); return; }

    var u = url + (url.indexOf('?') < 0 ? '?' : '&') + 'token=' + encodeURIComponent(token || '');
    var es;
    try { es = new EventSource(u); }
    catch (e) { Fw.warn('subscribe "' + name + '" selhal: ' + e.message); return; }

    es.onmessage = function (ev) { Fw.line(ev.data, { origin: 'push', sub: name, seq: ++Fw._seq }); };
    es.onopen    = function () { Fw.debug('push "' + name + '" připojen'); };
    es.onerror   = function () { Fw.debug('push "' + name + '" přerušen, EventSource se připojí sám'); };
    Fw.subs[name] = es;
};

Fw.unsubscribe = function (name) {
    var es = Fw.subs[name];
    if (!es) return;
    es.close();
    delete Fw.subs[name];
    Fw.debug('push "' + name + '" odpojen');
};

Fw.register('subscribe', function (c) { Fw.subscribe(c.name, c.url, c.token); });

Fw.register('unsubscribe', function (c) {
    if (c.name) { Fw.unsubscribe(c.name); return; }
    Object.keys(Fw.subs).forEach(function (n) { Fw.unsubscribe(n); });   // bez name = všechny
});

/* ---------------------------------------------------------- TRANSPORT */

Fw.headers = function () {
    var h = {};
    if (Fw.session) h['X-App-Session'] = Fw.session;
    if (Fw.serial)  h['X-App-Serial']  = Fw.serial;
    return h;
};

Fw.send = function (fn, params, opts) {
    opts = opts || {};
    var body = (params instanceof FormData) ? params : Fw.toFormData(params);
    body.set('function', fn);
    /* Parametry aplikace se přidají ke každému požadavku (skin, tenant, jazyk). */
    for (var pk in Fw.cfg.params) if (!body.has(pk)) body.set(pk, Fw.cfg.params[pk]);
    var ctx = { origin: 'response', seq: ++Fw._seq, fn: fn,
                params: Fw.fromFormData(body), history: opts.history !== false };
    Fw.debug('-> ' + fn, ctx.params);
    return fetch(Fw.cfg.api, { method: 'POST', body: body, headers: Fw.headers(),
                               credentials: 'same-origin' })
        .then(function (res) {
            var ct = res.headers.get('content-type') || '';
            /* Chybový stav s NDJSON tělem nese příkaz error -> čti ho normálně.
               Cokoli jiného (500 z webserveru, HTML chybovka) ohlas jako text. */
            if (ct.indexOf('ndjson') < 0 || !res.body)
                return res.text().then(function (t) {
                    if (!res.ok) throw new Error('HTTP ' + res.status + ' ' + t.slice(0, 300));
                    return Fw.text(t, ctx);
                });
            return Fw.readStream(res.body, ctx);
        })
        .catch(function (e) { Fw.error('Spojení se serverem selhalo: ' + e.message); });
};

/* Čtečka NDJSON. Stejná pro HTTP odpověď i pro budoucí WebSocket —
   transport se změní, dispatcher ne. */
Fw.readStream = function (stream, ctx) {
    var reader = stream.getReader(), dec = new TextDecoder(), buf = '';
    function step() {
        return reader.read().then(function (r) {
            if (r.value) buf += dec.decode(r.value, { stream: true });
            var chain = Promise.resolve(), nl;
            while ((nl = buf.indexOf('\n')) >= 0) {
                var line = buf.slice(0, nl); buf = buf.slice(nl + 1);
                chain = chain.then(Fw.line.bind(null, line, ctx));
            }
            if (r.done) return chain.then(function () { return buf.trim() ? Fw.line(buf, ctx) : null; });
            return chain.then(step);
        });
    }
    return step();
};

/* Nestreamovaný text -> rozseká na řádky a prožene stejnou cestou. */
Fw.text = function (txt, ctx) {
    return String(txt).split('\n').reduce(function (p, l) {
        return p.then(function () { return Fw.line(l, ctx); });
    }, Promise.resolve());
};

Fw.line = function (line, ctx) {
    line = line.trim();
    if (!line) return Promise.resolve();
    var batch;
    try { batch = JSON.parse(line); }
    catch (e) { Fw.error('Neplatná odpověď serveru'); Fw.debug('parse fail', line); return Promise.resolve(); }
    Fw.debug('<- dávka (' + (batch.cmds || []).length + ' příkazů)', batch);
    return Fw.dispatch(batch, ctx);
};

/* Fragment stažený přímo (op:url). Session posílá klient sám —
   proto na serveru neexistuje žádné append_session. */
Fw.fragment = function (url) {
    return fetch(url, { headers: Fw.headers(), credentials: 'same-origin' })
        .then(function (r) {
            if (!r.ok) throw new Error('HTTP ' + r.status + ' při načítání ' + url);
            return r.text();
        })
        .catch(function (e) { Fw.error(e.message); return ''; });
};

/* Vyprázdní okno. Totéž co operace html s prázdným obsahem, jen bez
   cesty na server — pro čistě klientské zavření komponenty. */
Fw.clear = function (sel) {
    Fw.nodes(sel).forEach(function (el) { Fw.setHtml(el, ''); });
};

/* -------------------------------------------------------- UDÁLOSTI */

/* Parsuje "#?function=aaa&x=1" i "function=aaa&x=1". */
Fw.parseTarget = function (s) {
    s = String(s || '');
    var i = s.indexOf('?');
    s = (i >= 0) ? s.slice(i + 1) : s.replace(/^#/, '');
    var p = new URLSearchParams(s), out = { fn: p.get('function') || '', params: {} };
    p.forEach(function (v, k) { if (k !== 'function') out.params[k] = v; });
    return out;
};

/* data-busy="text" — překryv po dobu požadavku. Tohle NEPOTŘEBUJE
   server ani push: prohlížeč sám ví, že odeslal a čeká. Pokrývá tím
   ten nejčastější případ („strpení prosím, kompletuji data") bez
   jediného řádku navíc na straně BFF a bez jakékoli infrastruktury.

   Operace busy je pro to druhé: průběh, který zná JEN server —
   procenta, fáze, počty. Ty se pushnout musí.

   Cíl je data-busy-sel, jinak main. Úklid řeší dispatcher ve chvíli,
   kdy do okna dorazí odpověď; state=off za promise je jen pojistka
   pro případ, že odpověď cílí jinam nebo požadavek selže. */
function busyBehem(el, promise) {
    if (!el.hasAttribute('data-busy')) return promise;
    var sel = el.getAttribute('data-busy-sel') || 'main';
    Fw.busy(sel, { text: el.getAttribute('data-busy') || 'Pracuji…' });
    var konec = function () { Fw.busy(sel, { state: 'off' }); };
    if (promise && promise.then) promise.then(konec, konec);
    return promise;
}

Fw.bind = function () {
    /* Delegace na document: přežije jakékoli překreslení, nic se nepřevazuje. */
    document.addEventListener('click', function (ev) {
        var el = ev.target.closest && ev.target.closest('[data-fw]');
        if (!el || el.tagName === 'FORM') return;
        ev.preventDefault();
        /* data-confirm="text" — dotaz před odesláním. Prázdná hodnota
           použije obecný text. */
        if (el.hasAttribute('data-confirm')
            && !confirm(el.getAttribute('data-confirm') || 'Opravdu provést tuto akci?')) return;
        var t = Fw.parseTarget(el.getAttribute('href') || el.getAttribute('data-fw'));
        if (!t.fn) { Fw.warn('data-fw bez function'); return; }
        busyBehem(el, Fw.send(t.fn, t.params, { history: true }));
    });

    document.addEventListener('submit', function (ev) {
        var f = ev.target.closest && ev.target.closest('form[data-fw]');
        if (!f) return;
        ev.preventDefault();
        if (f.hasAttribute('data-confirm')
            && !confirm(f.getAttribute('data-confirm') || 'Opravdu odeslat?')) return;
        var t  = Fw.parseTarget(f.getAttribute('action') || f.getAttribute('data-fw'));
        var fd = new FormData(f);                       // zadarmo funguje i <input type=file>
        for (var k in t.params) if (!fd.has(k)) fd.set(k, t.params[k]);
        busyBehem(f, Fw.send(t.fn, fd, { history: false }));   // POST se do historie nedává
    });

    /* Prvek s data-fw-sync rozešle svou hodnotu do ostatních oken.
       Je to transport, ne aplikační logika, proto to patří sem. */
    document.addEventListener('input', function (ev) {
        var el = ev.target;
        if (!el.id || !el.matches || !el.matches('[data-fw-sync]')) return;
        Fw.broadcast([
            { op: 'value', sel: '#' + el.id,          value: el.value },
            { op: 'html',  sel: '#' + el.id + '_out', content: el.value },
            { op: 'sync_note' }
        ]);
    });

    /* Prvek s data-fw-submit odešle svůj formulář při změně.
       Pozor: form.submit() událost submit NEVYVOLÁ, a delegovaný
       posluchač by ji tedy neviděl — proto requestSubmit(). */
    document.addEventListener('change', function (ev) {
        var el = ev.target;
        if (!el.matches || !el.matches('[data-fw-submit]')) return;
        var f = el.closest('form[data-fw]');
        if (!f) return;
        if (f.requestSubmit) f.requestSubmit();
        else f.dispatchEvent(new Event('submit', { bubbles: true, cancelable: true }));
    });

    /* Prvek s data-fw-clear="selektor" vyprázdní cílové okno bez cesty
       na server. Tím se zavírá komponenta v překryvu: zavřít dialog je
       stav klienta, ne aplikace, a round trip jen kvůli prázdnému divu
       by byl zbytečný. Projde stejnou cestou jako operace html, takže
       se uklidí i tooltipy uvnitř (viz beforeReplace). */
    document.addEventListener('click', function (ev) {
        var el = ev.target.closest && ev.target.closest('[data-fw-clear]');
        if (!el) return;
        ev.preventDefault();
        Fw.clear(el.getAttribute('data-fw-clear'));
    });

    /* Escape zavře okno, které se k tomu přihlásilo. */
    document.addEventListener('keydown', function (ev) {
        if (ev.key !== 'Escape') return;
        var el = document.querySelector('[data-fw-esc]');
        if (el) Fw.clear(el.getAttribute('data-fw-esc'));
    });

    global.addEventListener('popstate', function (ev) {
        var s = ev.state && ev.state.fw;
        if (s) Fw.send(s.fn, s.params, { history: false });
    });
};

/* ------------------------------------------------------------ SERIÁL */

/* Trvalá identifikace instalace. localStorage -> cookie -> vygenerovat. */
Fw.getSerial = function () {
    var K = 'fw_serial', v = null;
    try { v = localStorage.getItem(K); } catch (e) {}
    if (!v) v = Fw.cookie(K);
    if (!v) {
        if (global.crypto && crypto.randomUUID) v = crypto.randomUUID();
        else if (global.crypto && crypto.getRandomValues) {
            var a = new Uint8Array(16); crypto.getRandomValues(a);
            v = Array.prototype.map.call(a, function (b) { return ('0' + b.toString(16)).slice(-2); }).join('');
        } else v = 'r' + Date.now().toString(36) + Math.random().toString(36).slice(2);
    }
    try { localStorage.setItem(K, v); } catch (e) {}
    Fw.cookie(K, v, 3650);
    return v;
};

Fw.cookie = function (k, v, days) {
    if (v === undefined) {
        var m = ('; ' + document.cookie).split('; ' + k + '=');
        return m.length === 2 ? decodeURIComponent(m.pop().split(';').shift()) : null;
    }
    var d = new Date(Date.now() + days * 864e5);
    document.cookie = k + '=' + encodeURIComponent(v) + '; expires=' + d.toUTCString() +
                      '; path=/; SameSite=Lax';
};

/* -------------------------------------------------------- HLÁŠENÍ */

/* Jediné místo, kde se hlásí uživateli. Projekt ho může přepsat
   (viz app2.js a Bootstrap toasty). */
Fw.notify = function (kind, msg) {
    (kind === 'error' ? console.error : console.log)('[fw] ' + kind + ': ' + msg);
    var box = document.getElementById('fw_error');
    if (!box) { if (kind === 'error') alert(msg); return; }
    box.className = 'fw-note fw-note-' + kind;
    box.textContent = msg;
    box.style.display = 'block';
    clearTimeout(Fw._et);
    Fw._et = setTimeout(function () { box.style.display = 'none'; }, kind === 'error' ? 8000 : 4000);
};

Fw.error = function (msg) { Fw.notify('error', msg); };

Fw.warn  = function (msg)       { if (Fw.cfg.debug) console.warn('[fw] ' + msg); };
Fw.debug = function (msg, data) {
    if (!Fw.cfg.debug) return;
    if (data === undefined) console.log('[fw] ' + msg); else console.log('[fw] ' + msg, data);
};

/* --------------------------------------------------------- POMOCNÉ */

Fw.toFormData = function (o) {
    var fd = new FormData();
    for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) fd.set(k, o[k]);
    return fd;
};
Fw.fromFormData = function (fd) {
    var o = {};
    fd.forEach(function (v, k) { if (typeof v === 'string' && k !== 'function') o[k] = v; });
    return o;
};

/* ------------------------------------------------------------- INIT */

Fw.init = function (cfg) {
    for (var k in cfg) Fw.cfg[k] = cfg[k];
    Fw.serial = Fw.getSerial();

    /* Chybový rám nesmí po startu viset prázdný. */
    var box = document.getElementById('fw_error');
    if (box) { box.textContent = ''; box.style.display = 'none'; }

    /* Kanál mezi okny: implicitně podle cesty aplikace, ať si dvě různé
       aplikace na stejném hostu nepřepisují stav. cfg.channel=false vypne. */
    if (Fw.cfg.channel !== false)
        Fw.openChannel(Fw.cfg.channel || ('fw:' + location.pathname));
    try { Fw.session = localStorage.getItem('fw_session') || null; } catch (e) {}
    Fw.busyStyle();
    Fw.bind();
    Fw.debug('init, serial=' + Fw.serial + ', session=' + (Fw.session ? 'ano' : 'ne'));
    return Fw.send('index', {}, { history: false });
};

global.Fw = Fw;
})(window);
