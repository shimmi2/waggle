<?php
/* @var array $books @var int $total @var int $limit @var int $offset @var array $args

   total se počítá BEZ filtru dostupnosti, protože ten je drahý. Hláška
   to tedy musí umět říct, jinak vyjde „3 knihy v katalogu" a hned pod
   tím „nic tu nemáme", což je protimluv. */
$jen_volne = !empty($args['available']);
/* Filtr putuje s KAŽDÝM odkazem, který vede na dialog. Tenhle fragment
   je jediné místo, které zná jeho aktuální podobu — hlavička s filtrem
   se po změně nepřekresluje a nesla by stav, který už neplatí. */
$filtr = ['q' => $args['q'] ?? '', 'genre' => $args['genre'] ?? 0,
          'available' => $args['available'] ?? '', 'order' => $args['order'] ?? '',
          'offset' => $args['offset'] ?? 0];
$qf = http_build_query($filtr);
$spravuje = may('manage_books');
?>
<div class="card">
  <div class="card-header d-flex justify-content-between align-items-center">
    <h5 class="mb-0"><?= esc(pocet((int)$total, 'kniha', 'knihy', 'knih')) ?> v katalogu<?php
        if ($jen_volne && $books) echo ', zobrazeny volné'; ?></h5>
    <?php /* Kreslení není autorizace: kdo si tlačítko podvrhne, dostane
             z datového API 403. */
    if ($spravuje): ?>
      <a class="btn btn-primary btn-sm" href="#?function=book_form&<?= esc($qf) ?>" data-fw>
        <i class="bx bx-plus me-1"></i>Přidat knihu</a>
    <?php endif; ?>
  </div>

  <?php if (!$books && $jen_volne && $total > 0): ?>
    <div class="card-body"><div class="alert alert-info mb-0">
      Filtru odpovídá <?= esc(pocet((int)$total, 'kniha', 'knihy', 'knih')) ?>,
      ale žádná není právě volná. Zkus vypnout „jen volné".</div></div>
  <?php elseif (!$books): ?>
    <div class="card-body"><div class="alert alert-info mb-0">
      Nic takového tu nemáme. Zkus jiný filtr.</div></div>
  <?php else: ?>
  <div class="table-responsive text-nowrap">
    <table class="table table-hover">
      <thead><tr><th>název</th><th>autor</th><th>žánr</th><th class="fw-num">rok</th>
                 <th class="fw-num">dostupnost</th><th class="fw-num">cena</th>
                 <?php if ($spravuje): ?><th></th><?php endif; ?></tr></thead>
      <tbody class="table-border-bottom-0">
      <?php foreach ($books as $b):
          $vol = (int)$b['bo_available']; $kusu = (int)$b['bo_count']; ?>
        <tr>
          <td><a href="#?function=book_detail&bo_id=<?= (int)$b['bo_id'] ?>&<?= esc($qf) ?>"
                 data-fw class="fw-semibold"><?= esc($b['bo_name']) ?></a></td>
          <td><?= esc($b['bo_author']) ?></td>
          <td class="text-muted"><?= esc($b['ge_name'] ?? '—') ?></td>
          <td class="fw-num"><?= $b['bo_year'] ? (int)$b['bo_year'] : '—' ?></td>
          <td class="fw-num">
            <?php /* Stav odznakem, ne barvou řádku — obarvený řádek se
                     v dlouhé tabulce čte špatně. */
            if ($vol <= 0)      echo badge('vše půjčeno', 'danger');
            elseif ($vol === 1) echo badge('poslední kus', 'warn');
            else                echo badge($vol . ' z ' . $kusu, 'ok'); ?>
          </td>
          <td class="fw-num"><?= esc(kc($b['bo_price'])) ?></td>
          <?php if ($spravuje): ?>
            <td class="fw-num"><a class="btn btn-icon btn-sm btn-outline-secondary"
               href="#?function=book_form&bo_id=<?= (int)$b['bo_id'] ?>&<?= esc($qf) ?>"
               data-fw title="Upravit"><i class="bx bx-edit"></i></a></td>
          <?php endif; ?>
        </tr>
      <?php endforeach; ?>
      </tbody>
    </table>
  </div>
  <div class="card-body pt-0">
    <?= pager('books_results', (int)$total, (int)$limit, (int)$offset,
              ['q' => $args['q'] ?? '', 'genre' => $args['genre'] ?? 0,
               'available' => $args['available'] ?? '', 'order' => $args['order'] ?? '']) ?>
  </div>
  <?php endif; ?>
</div>
