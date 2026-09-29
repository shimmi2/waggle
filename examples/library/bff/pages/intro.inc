<?php
$dlazdice = [
    ['books',   'bx-book',            'Katalog',    'Hledání v šedesáti knihách, filtr na žánr a dostupnost.'],
    ['rentals', 'bx-transfer',        'Výpůjčky',   'Kdo co má, co je po termínu, vracení jedním kliknutím.'],
    ['genres',  'bx-collection',      'Žánry',      'Číselník, na kterém stojí filtr katalogu.'],
];
if (may('see_statistics'))
    $dlazdice[] = ['statistics', 'bx-bar-chart-alt-2', 'Statistika', 'Pět dotazů, průběh přes push.'];
if (may('manage_users'))
    $dlazdice[] = ['users', 'bx-user', 'Uživatelé', 'Obsluha i čtenáři, liší se jen oprávněním.'];
?>
<div class="row g-4">
  <?php foreach ($dlazdice as [$fn, $ik, $nad, $pop]): ?>
    <div class="col-md-6 col-xl-4">
      <a href="#?function=<?= esc($fn) ?>" data-fw class="card h-100 text-decoration-none">
        <div class="card-body">
          <div class="d-flex align-items-center mb-2">
            <span class="badge bg-label-primary p-2 me-2 rounded"><i class="bx <?= esc($ik) ?> fs-5"></i></span>
            <h5 class="mb-0"><?= esc($nad) ?></h5>
          </div>
          <p class="text-muted mb-0 small"><?= esc($pop) ?></p>
        </div>
      </a>
    </div>
  <?php endforeach; ?>
</div>
