<div class="row"><div class="col-md-6">
<div class="card card-info card-outline">
  <div class="card-header"><h3 class="card-title">Čas serveru</h3></div>
  <div class="card-body">
    <p>Generovaná stránka <code>page_time.inc</code> — stejná zkratka, ale soubor se
       includuje, takže může počítat.</p>
    <table class="table table-sm mb-0">
      <tr><th style="width:40%">Datum a čas</th><td><?= esc(date('Y-m-d H:i:s')) ?></td></tr>
      <tr><th>PHP</th><td><?= esc(PHP_VERSION) ?></td></tr>
      <tr><th>Unix time</th><td><?= time() ?></td></tr>
    </table>
  </div>
</div></div></div>
