<div class="row"><div class="col-md-8">
<div class="card card-primary card-outline">
  <div class="card-header"><h3 class="card-title"><i class="bi bi-hourglass-split me-1"></i>Průběh zpracování</h3></div>
  <div class="card-body">
    <p class="text-secondary small">
      Server pošle <b>jednu NDJSON dávku na každé procento</b> a mezi nimi spí 0,1 s.
      Jedno HTTP spojení, 101 dávek, žádný polling.</p>

    <div class="progress mb-2" style="height:26px">
      <div id="pb" class="progress-bar progress-bar-striped progress-bar-animated"
           role="progressbar" style="width: 0%"
           aria-valuenow="0" aria-valuemin="0" aria-valuemax="100"><span id="pb_pct">0 %</span></div>
    </div>

    <div id="pb_note" class="small">&nbsp;</div>
  </div>
  <div class="card-footer">
    <a href="#?function=progress_run" data-fw id="run_btn" class="btn btn-outline-primary">
      <i class="bi bi-play-fill me-1"></i>Spustit streamem</a>
    <a href="#?function=progress_push" data-fw id="run_push" class="btn btn-primary ms-1">
      <i class="bi bi-broadcast me-1"></i>Spustit pushem</a>
    <span class="text-secondary small ms-2">obojí trvá ~10 s</span>
  </div>
</div>

<div class="alert alert-light border mt-2 mb-0 small">
  <i class="bi bi-lightbulb me-1"></i>
  Div <code>#main</code> se během běhu <b>ani jednou nepřekreslí</b>. Server sahá jen na
  atribut <code>style</code> a <code>aria-valuenow</code> u <code>#pb</code> a na obsah
  <code>#pb_pct</code>. Proto zůstane plynulý CSS přechod i pruhovaná animace.
</div>

<div class="alert alert-warning border mt-2 mb-0 small">
  <i class="bi bi-exclamation-triangle me-1"></i>
  <b>Streamem</b> bar na tomhle serveru poskočí po ~4 s blocích. Není to chyba
  frameworku — PHP posílá dávky přesně po 0,1 s (ověřeno), ale Apache
  <code>mod_proxy_fcgi</code> je drží. PHP navíc celých 10 s blokuje jeden
  Apache worker.
</div>

<div class="alert alert-success border mt-2 mb-0 small">
  <i class="bi bi-broadcast me-1"></i>
  <b>Pushem</b> je to opačně: PHP spustí workera na pozadí, odpoví za ~20 ms
  a skončí. Procenta doručuje nginx přes SSE, takže Apache v cestě streamu
  vůbec není a buffering zmizí. Vyžaduje nchan — dokud neběží, vrátí
  endpoint <code>503</code> s vysvětlením.
</div>
</div></div>
