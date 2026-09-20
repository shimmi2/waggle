<div class="row"><div class="col-md-7">
<div class="card card-success card-outline">
  <div class="card-header"><h3 class="card-title"><i class="bi bi-check-circle me-1"></i>Uloženo</h3></div>
  <div class="card-body">
    <p>Server přijal od <b><?= esc($user['name']) ?></b> tyto hodnoty:</p>
    <table class="table table-sm">
      <tr><th style="width:35%">Název</th><td><?= esc($nazev) ?></td></tr>
      <tr><th>Počet</th><td><?= (int)$pocet ?></td></tr>
      <tr><th>Poznámka</th><td><?= esc($poznamka) ?></td></tr>
    </table>
  </div>
  <div class="card-footer">
    <a href="#?function=form_test" data-fw class="btn btn-outline-secondary btn-sm">Zpět na formulář</a>
  </div>
</div></div></div>
