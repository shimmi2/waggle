<div class="card">
  <h2>Zaneprázdněno — jedna operace, čtyři chování</h2>
  <p>Na drátě je to vždycky <code>{"op":"busy"}</code>. Překryv si framework vyrobí
     sám nad libovolným prvkem — stránka si pro něj nic nepřipravuje. Porovnej
     s <a href="#?function=progress" data-fw>Průběhem zpracování</a>, kde si bar
     drží stránka a server sahá na jeho atributy.</p>

  <div id="busy_zone" style="border:1px solid #ccc;padding:24px;text-align:center;min-height:150px">
    Sem se vejde obsah, nad kterým překryv leží.
    <div id="busy_out"><small>&nbsp;</small>  <h3>Div vyšší než obrazovka</h3>
  <p><small>Klikni na „Vysoký div", odscrolluj kamkoli a text je pořád vidět.
     Obsah překryvu sedí ve vnitřním pásu s <code>position:sticky</code>, takže
     ho prohlížeč sám drží ve výřezu. Bez toho by přistál v geometrickém středu
     téhle roury — tedy nikde.</small></p>
  <div id="busy_tall" style="border:1px solid #ccc;height:2400px;padding:12px">
    začátek roury
    <div style="margin-top:2300px">konec roury</div>
  </div>
</div>
  </div>

  <p>
    <a href="#?function=busy_quick"  data-fw>Krátká operace (150 ms)</a> &nbsp;
    <a href="#?function=busy_phases" data-fw>Tři fáze, kolečko</a> &nbsp;
    <a href="#?function=busy_bar"    data-fw>Kolečko → pruh s procenty</a> &nbsp;
    <a href="#?function=busy_clear"  data-fw>Překreslení okna překryv sundá</a> &nbsp;
    <a href="#?function=busy_slow"   data-fw
       data-busy="Kompletuji data…" data-busy-sel="#busy_zone">data-busy — bez serveru</a> &nbsp;
    <a href="#?function=busy_slow"   data-fw
       data-busy="Hledej mě — jsem vidět i tady dole." data-busy-sel="#busy_tall">Vysoký div</a>
  </p>

  <p><small>
    <b>Krátká operace neukáže nic</b> — prvních 300 ms se nekreslí, protože záblesk
    působí pomaleji než ticho. <b>Tři fáze</b> jsou tři obyčejná volání; jedno okno
    = jeden překryv, takže se nehromadí. <b>Kolečko a pruh nejsou dva typy</b> —
    bez <code>pct</code> se točí, s <code>pct</code> se promění v pruh.
  </small></p>

  <p><small><b>Poslední odkaz neposílá žádný busy.</b> Má jen
     <code>data-busy</code> a překryv si pustí prohlížeč sám — ví přece, že
     odeslal a čeká. Bez serveru, bez nchanu. Operace <code>busy</code> je pro
     to, co zná jen server: procenta, fáze, počty.</small></p>

  <p><small><b>Ostatní se doručuje pushem, ne streamem.</b> PHP si drží request a mezi kroky
     volá <code>fw_publish()</code>, což je krátký POST na nchan na localhostu.
     Apache v té cestě není, takže jeho buffering nic nedrží — nejsou proto potřeba
     ani <code>fw_stream_start()</code>, ani worker na pozadí. Bez nchanu vrátí
     endpoint 503.</small></p>
</div>
