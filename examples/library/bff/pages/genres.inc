<?php /* @var array $genres */ ?>
<div class="card">
  <h5 class="card-header">Žánry</h5>
  <div class="table-responsive text-nowrap">
    <table class="table table-hover">
      <thead><tr><th>žánr</th><th>popis</th><th class="fw-num">knih</th></tr></thead>
      <tbody class="table-border-bottom-0">
      <?php foreach ($genres as $g): ?>
        <tr>
          <td><a href="#?function=books&genre=<?= (int)$g['ge_id'] ?>" data-fw
                 class="fw-semibold"><?= esc($g['ge_name']) ?></a></td>
          <td class="text-muted"><?= esc($g['ge_description']) ?></td>
          <td class="fw-num"><?= (int)($g['ge_books'] ?? 0) ?></td>
        </tr>
      <?php endforeach; ?>
      </tbody>
    </table>
  </div>
  <div class="card-body pt-0">
    <p class="text-muted small mb-0">Proklik na žánr otevře katalog s nastaveným filtrem —
       celou obrazovku, ne jen tabulku, protože jinam než na katalog ten filtr nepatří.</p>
  </div>
</div>
