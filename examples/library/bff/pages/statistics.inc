<?php /* @var array $monthly @var array $genres @var array $top_books
         @var array $top_readers @var array $duration

         Pět přehledů, každý z vlastního volání datového API. Postup mezi
         nimi hlásí BFF, takže API nemusí vědět nic o prohlížeči. */
$max = fn(array $rows, string $k) => max(1, ...array_map(fn($r) => (float)$r[$k], $rows ?: [[$k => 1]]));
?>
<div class="card mb-4">
  <div class="card-body">
    <h5 class="mb-1">Statistika</h5>
    <p class="text-muted mb-0">Pět přehledů, pět volání datového API. Ukazatel postupu
       mezi nimi není ozdoba — každé procento znamená hotovou část.</p>
  </div>
</div>

<div class="card mb-4">
  <h5 class="card-header">Výpůjčky po měsících</h5>
  <div class="card-body">
    <?php $m = array_slice($monthly, 0, 24); $mx = $max($m, 'n'); ?>
    <div class="fw-bars">
      <?php foreach (array_reverse($m) as $r): ?>
        <div class="fw-bar" title="<?= (int)$r['y'] ?>/<?= (int)$r['m'] ?>: <?= (int)$r['n'] ?>">
          <span style="height:<?= (int)round((int)$r['n'] / $mx * 100) ?>%"></span>
          <em><?= (int)$r['m'] ?></em>
        </div>
      <?php endforeach; ?>
    </div>
  </div>
</div>

<div class="row g-4 mb-4">
  <div class="col-lg-6">
    <div class="card h-100">
      <h5 class="card-header">Podle žánrů</h5>
      <div class="table-responsive text-nowrap">
        <table class="table table-sm">
          <thead><tr><th>žánr</th><th class="fw-num">výpůjček</th><th class="fw-num">výnos</th></tr></thead>
          <tbody class="table-border-bottom-0">
          <?php foreach ($genres as $r): ?>
            <tr><td><?= esc($r['ge_name']) ?></td>
                <td class="fw-num"><?= (int)$r['n'] ?></td>
                <td class="fw-num"><?= esc(kc($r['v'])) ?></td></tr>
          <?php endforeach; ?>
          </tbody>
        </table>
      </div>
    </div>
  </div>
  <div class="col-lg-6">
    <div class="card h-100">
      <h5 class="card-header">Doba držení po letech</h5>
      <div class="table-responsive text-nowrap">
        <table class="table table-sm">
          <thead><tr><th>rok</th><th class="fw-num">průměr</th><th class="fw-num">nejdéle</th></tr></thead>
          <tbody class="table-border-bottom-0">
          <?php foreach (array_slice($duration, 0, 10) as $r): ?>
            <tr><td><?= (int)$r['y'] ?></td>
                <td class="fw-num"><?= esc($r['days_avg']) ?> dní</td>
                <td class="fw-num"><?= (int)$r['days_max'] ?> dní</td></tr>
          <?php endforeach; ?>
          </tbody>
        </table>
      </div>
    </div>
  </div>
</div>

<div class="row g-4">
  <div class="col-lg-6">
    <div class="card h-100">
      <h5 class="card-header">Nejpůjčovanější</h5>
      <div class="table-responsive text-nowrap">
        <table class="table table-sm">
          <thead><tr><th>kniha</th><th class="fw-num">×</th></tr></thead>
          <tbody class="table-border-bottom-0">
          <?php foreach (array_slice($top_books, 0, 10) as $r): ?>
            <tr><td><?= esc($r['bo_name']) ?><br>
                    <small class="text-muted"><?= esc($r['bo_author']) ?></small></td>
                <td class="fw-num"><?= (int)$r['n'] ?></td></tr>
          <?php endforeach; ?>
          </tbody>
        </table>
      </div>
    </div>
  </div>
  <div class="col-lg-6">
    <div class="card h-100">
      <h5 class="card-header">Nejaktivnější čtenáři</h5>
      <div class="table-responsive text-nowrap">
        <table class="table table-sm">
          <thead><tr><th>čtenář</th><th class="fw-num">×</th><th class="fw-num">zaplatil</th></tr></thead>
          <tbody class="table-border-bottom-0">
          <?php foreach (array_slice($top_readers, 0, 10) as $r): ?>
            <tr><td><?= esc($r['us_name']) ?></td>
                <td class="fw-num"><?= (int)$r['n'] ?></td>
                <td class="fw-num"><?= esc(kc($r['v'])) ?></td></tr>
          <?php endforeach; ?>
          </tbody>
        </table>
      </div>
    </div>
  </div>
</div>
