<div class="row">
  <div class="col-lg-7">
    <!-- Karta s tlačítky sbalit/zavřít = AdminLTE CardWidget.
         Funguje jen tehdy, když po výměně obsahu proběhl initialize(). -->
    <div class="card card-primary card-outline">
      <div class="card-header">
        <h3 class="card-title"><i class="bi bi-person-badge me-1"></i>Přehled</h3>
        <div class="card-tools">
          <button type="button" class="btn btn-tool" data-lte-toggle="card-collapse"
                  data-bs-toggle="tooltip" title="Sbalit kartu">
            <i data-lte-icon="expand" class="bi bi-plus-lg"></i>
            <i data-lte-icon="collapse" class="bi bi-dash-lg"></i></button>
          <button type="button" class="btn btn-tool" data-lte-toggle="card-remove"
                  data-bs-toggle="tooltip" title="Zavřít kartu"><i class="bi bi-x-lg"></i></button>
        </div>
      </div>
      <div class="card-body">
        <table class="table table-sm mb-0">
          <tr><th style="width:40%">Uživatel</th>
              <td><?= esc($user['name']) ?> (<code><?= esc($user['login']) ?></code>)</td></tr>
          <tr><th>Oprávnění</th><td><?= esc(implode(', ', $user['acl']) ?: '—') ?></td></tr>
          <tr><th>Seriál instalace</th><td><code class="small"><?= esc($serial) ?></code></td></tr>
          <tr><th>Čas serveru</th><td><?= esc(date('H:i:s')) ?></td></tr>
        </table>
      </div>
    </div>
  </div>

  <div class="col-lg-5">
    <div class="card" id="ping_card">
      <div class="card-header"><h3 class="card-title">Jemné adresování</h3></div>
      <div class="card-body">
        <p class="text-secondary small mb-2">
          Server nepřekresluje celou kartu — sáhne jen na <code>#ping_out</code>,
          přidá třídu kartě a změní atribut <code>title</code> tlačítka.</p>
        <a href="#?function=ping" data-fw id="ping_btn" class="btn btn-sm btn-outline-secondary"
           data-bs-toggle="tooltip" title="zatím neproběhlo">
          <i class="bi bi-arrow-repeat me-1"></i>Ping</a>
        <span id="ping_out" class="ms-2 text-secondary small">—</span>
      </div>
    </div>
  </div>
</div>

<div class="alert alert-light border mt-2 mb-0">
  <i class="bi bi-lightbulb me-1"></i>
  Shell (navbar, sidebar, footer) se od načtení stránky <b>ani jednou nepřekreslil</b>.
  Zkuste sbalit sidebar, pak proklikat menu — zůstane sbalený. Funguje i tlačítko Zpět.
</div>
