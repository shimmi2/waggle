<div class="row"><div class="col-md-9">
<div class="card card-primary card-outline">
  <div class="card-header"><h3 class="card-title">
    <i class="bi bi-hourglass me-1"></i>Zaneprázdněno — jedna operace, čtyři chování</h3></div>

  <div class="card-body">
    <p class="text-secondary small mb-3">
      Na drátě je to vždycky <code>{"op":"busy"}</code>. Překryv si framework
      vyrobí sám nad libovolným prvkem — stránka si pro něj nic nepřipravuje.
      Porovnej s <a href="#?function=progress" data-fw>Průběhem zpracování</a>,
      kde si bar drží stránka a server sahá na jeho atributy.</p>

    <div id="busy_zone" class="border rounded p-4 text-center bg-body-tertiary" style="min-height:190px">
      <div class="display-6 text-secondary"><i class="bi bi-table"></i></div>
      <div class="text-secondary">Sem se vejde obsah, nad kterým překryv leží.</div>
      <div id="busy_out" class="mt-2 small">&nbsp;</div>
    </div>
  </div>

  <div class="card-footer d-flex flex-wrap gap-2">
    <a href="#?function=busy_quick"  data-fw class="btn btn-outline-secondary btn-sm">
      <i class="bi bi-lightning me-1"></i>Krátká operace (150 ms)</a>
    <a href="#?function=busy_phases" data-fw class="btn btn-outline-primary btn-sm">
      <i class="bi bi-three-dots me-1"></i>Tři fáze, kolečko</a>
    <a href="#?function=busy_bar"    data-fw class="btn btn-primary btn-sm">
      <i class="bi bi-bar-chart-steps me-1"></i>Kolečko → pruh s procenty</a>
    <a href="#?function=busy_clear"  data-fw class="btn btn-outline-danger btn-sm">
      <i class="bi bi-eraser me-1"></i>Překreslení okna překryv sundá</a>
    <a href="#?function=busy_slow"   data-fw class="btn btn-dark btn-sm"
       data-busy="Kompletuji data…" data-busy-sel="#busy_zone">
      <i class="bi bi-mouse me-1"></i>data-busy — bez serveru</a>
  </div>
</div>

<div class="alert alert-light border mt-2 mb-0 small">
  <i class="bi bi-stopwatch me-1"></i>
  <b>Krátká operace neukáže nic.</b> Prvních 300 ms se nekreslí. Kdyby problikla,
  působila by pomaleji, než ve skutečnosti je — záblesk mozek čte jako zádrhel.
</div>

<div class="alert alert-light border mt-2 mb-0 small">
  <i class="bi bi-arrow-repeat me-1"></i>
  <b>Tři fáze jsou tři obyčejná volání</b> — „připravuji", „počítám", „dokončuji".
  Jedno okno = jeden překryv, takže se nehromadí a klient si nic nepamatuje.
</div>

<div class="alert alert-light border mt-2 mb-0 small">
  <i class="bi bi-bar-chart-steps me-1"></i>
  <b>Kolečko a pruh nejsou dva typy.</b> Bez <code>pct</code> se točí, s <code>pct</code>
  se to promění v pruh. Proto jde začít „prohledávám…" a přepnout na procenta
  ve chvíli, kdy je konečně známé, kolik toho je.
</div>

<div class="alert alert-primary border mt-2 mb-0 small">
  <i class="bi bi-mouse me-1"></i>
  <b>Poslední tlačítko neposílá žádný busy.</b> Má jen
  <code>data-busy="Kompletuji data…"</code> a překryv si pustí prohlížeč sám —
  ví přece, že odeslal a čeká. Tímhle se pokryje ten nejčastější případ bez
  jediného řádku na straně serveru a bez nchanu. Operace <code>busy</code>
  je pro to, co zná <b>jen</b> server: procenta, fáze, počty.
</div>

<div class="alert alert-success border mt-2 mb-0 small">
  <i class="bi bi-broadcast me-1"></i>
  <b>Doručuje se pushem, ne streamem.</b> PHP si drží request a mezi kroky volá
  <code>fw_publish()</code> — krátký POST na nchan na localhostu. Apache v té
  cestě vůbec není, takže jeho buffering nic nedrží. Proto tady nejsou potřeba
  ani <code>fw_stream_start()</code>, ani worker na pozadí.
</div>
</div></div>
