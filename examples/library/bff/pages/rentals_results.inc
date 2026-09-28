<?php /* @var array $rentals @var int $total @var array $mine @var string $scope @var array $args */ ?>
<div class="card">
  <h3><?= esc(pocet((int)$total, 'výpůjčka', 'výpůjčky', 'výpůjček')) ?>
      <?php if ($scope === 'own'): ?><?= badge('jen moje', 'neutral') ?><?php endif; ?></h3>
  <?php if (!$rentals): ?>
    <p class="hint">Nic tu není.</p>
  <?php else: ?>
  <table>
    <thead><tr><th>kniha</th><th>čtenář</th><th>půjčeno</th><th>vrátit do</th>
               <th>stav</th><th class="r">cena</th><?php if (may('rent_books')): ?><th></th><?php endif; ?></tr></thead>
    <tbody>
    <?php foreach ($rentals as $r): ?>
      <tr>
        <td><?= esc($r['bo_name']) ?><br><small><?= esc($r['bo_author']) ?></small></td>
        <td><?= esc($r['us_name']) ?></td>
        <td><?= den($r['re_date']) ?></td>
        <td><?= den($r['re_allowed_return_date']) ?></td>
        <td>
          <?php if (!empty($r['returned_at'])) echo badge('vráceno ' . den($r['returned_at']), 'ok');
                elseif (!empty($r['is_overdue'])) echo badge('po termínu', 'danger');
                else echo badge('půjčeno', 'warn'); ?>
        </td>
        <td class="r"><?= esc(kc($r['re_price'])) ?></td>
        <?php if (may('rent_books')): ?>
          <td class="r">
            <?php if (empty($r['returned_at'])): ?>
              <a href="#?function=do_return&rental=<?= (int)$r['re_id'] ?>" data-fw
                 data-confirm="Zapsat vrácení téhle knihy?" class="btn-small">vrátit</a>
            <?php endif; ?>
          </td>
        <?php endif; ?>
      </tr>
    <?php endforeach; ?>
    </tbody>
  </table>
  <?= pager('rentals_results', (int)$total, (int)($limit ?? 50), (int)($offset ?? 0),
            ['open' => $args['open'] ?? '', 'overdue' => $args['overdue'] ?? '',
             'user' => $args['user'] ?? 0]) ?>
  <?php endif; ?>
</div>
