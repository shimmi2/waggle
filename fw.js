/* =====================================================================
 *  fw.js — klientská část frameworku
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
var RELEASE  = '1.0.0';    // vydání knihovny, mění se nezávisle na protokolu

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
        Fw.send(t.fn, t.params, { history: true });
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
        Fw.send(t.fn, fd, { history: false });          // POST se do historie nikdy nedává
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
    Fw.bind();
    Fw.debug('init, serial=' + Fw.serial + ', session=' + (Fw.session ? 'ano' : 'ne'));
    return Fw.send('index', {}, { history: false });
};

global.Fw = Fw;
})(window);
