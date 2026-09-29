<?php
/* @var array $acls  číselník oprávnění pro legendu
   @var array $args  filtr, jak ho sestavil dispatcher

   Filtr a výsledky jsou dva divy. Legenda oprávnění patří sem, ne do
   výsledků — je to popis obrazovky a při hledání se nemění. */
$f_q = (string)($args['q'] ?? '');
?>
<div class="card mb-4">
  <h5 class="card-header">Uživatelé</h5>
  <div class="card-body">
    <p class="text-muted">Obsluha i čtenáři jsou <b>titíž uživatelé</b>. Liší se výhradně
       oprávněním — žádné dvě aplikace, žádné dvě přihlašovací obrazovky.</p>
    <form data-fw action="#?function=users_results" method="post">
      <input type="hidden" name="offset" value="0">
      <div class="row g-3 align-items-end">
        <div class="col-md-5">
          <label class="form-label" for="u_q">Hledat</label>
          <input type="text" class="form-control" id="u_q" name="q"
                 placeholder="jméno, login nebo e-mail" value="<?= esc($f_q) ?>">
        </div>
        <div class="col-auto">
          <button type="submit" class="btn btn-primary btn-sm">Hledat</button>
        </div>
      </div>
    </form>
  </div>
</div>
<div id="users_results"></div>
<div class="card mt-4">
  <h5 class="card-header">Co která oprávnění znamenají</h5>
  <div class="table-responsive text-nowrap">
    <table class="table">
      <thead><tr><th>klíč</th><th>název</th><th>popis</th></tr></thead>
      <tbody class="table-border-bottom-0">
      <?php foreach ($acls as $a): ?>
        <tr><td><code><?= esc($a['ac_key']) ?></code></td>
            <td><?= esc($a['ac_name']) ?></td>
            <td class="text-muted"><small><?= esc($a['ac_description']) ?></small></td></tr>
      <?php endforeach; ?>
      </tbody>
    </table>
  </div>
  <div class="card-body pt-0">
    <p class="text-muted small mb-0">Oprávnění jsou <b>řádky v číselníku</b>, ne <code>ENUM</code>.
       Nová funkce v aplikaci je jeden <code>INSERT</code>, nikdy <code>ALTER TABLE</code>.</p>
  </div>
</div>
