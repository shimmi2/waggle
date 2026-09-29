<?php /* @var array $rentals @var int $total @var array $mine @var string $scope @var array $args */ ?>
<div class="card">
  <h5 class="card-header d-flex align-items-center gap-2">
    <?= esc(pocet((int)$total, 'výpůjčka', 'výpůjčky', 'výpůjček')) ?>
    <?php if ($scope === 'own') echo badge('jen moje', 'neutral'); ?>
  </h5>
  <?php if (!$rentals): ?>
    <div class="card-body"><div class="alert alert-info mb-0">Nic tu není.</div></div>
  <?php else: ?>
  <div class="table-responsive text-nowrap">
    <table class="table table-hover">
      <thead><tr><th>kniha</th><th>čtenář</th><th>půjčeno</th><th>vrátit do</th>
                 <th>stav</th><th class="fw-num">cena</th>
                 <?php if (may('rent_books')): ?><th></th><?php endif; ?></tr></thead>
      <tbody class="table-border-bottom-0">
      <?php foreach ($rentals as $r): ?>
        <tr>
          <td><span class="fw-semibold"><?= esc($r['bo_name']) ?></span>
              <br><small class="text-muted"><?= esc($r['bo_author']) ?></small></td>
          <td><?= esc($r['us_name']) ?></td>
          <td><?= den($r['re_date']) ?></td>
          <td><?= den($r['re_allowed_return_date']) ?></td>
          <td>
            <?php /* Stav odznakem, ne barvou řádku. */
            if (!empty($r['returned_at']))   echo badge('vráceno ' . den($r['returned_at']), 'ok');
            elseif (!empty($r['is_overdue'])) echo badge('po termínu', 'danger');
            else                              echo badge('půjčeno', 'warn'); ?>
          </td>
          <td class="fw-num"><?= esc(kc($r['re_price'])) ?></td>
          <?php if (may('rent_books')): ?>
            <td class="fw-num">
              <?php if (empty($r['returned_at'])): ?>
                <a href="#?function=do_return&rental=<?= (int)$r['re_id'] ?>" data-fw
                   data-confirm="Zapsat vrácení téhle knihy?"
                   class="btn btn-sm btn-outline-primary">vrátit</a>
              <?php endif; ?>
            </td>
          <?php endif; ?>
        </tr>
      <?php endforeach; ?>
      </tbody>
    </table>
  </div>
  <div class="card-body pt-0">
    <?= pager('rentals_results', (int)$total, (int)($limit ?? 50), (int)($offset ?? 0),
              ['open' => $args['open'] ?? '', 'overdue' => $args['overdue'] ?? '',
               'user' => $args['user'] ?? 0]) ?>
  </div>
  <?php endif; ?>
</div>
