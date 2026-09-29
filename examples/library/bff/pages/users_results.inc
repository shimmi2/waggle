<?php /* @var array $users @var int $total @var int $limit @var int $offset @var array $args */ ?>
<div class="card">
  <h5 class="card-header"><?= esc(pocet((int)$total, 'uživatel', 'uživatelé', 'uživatelů')) ?><?php
      if ((int)$total > (int)$limit) echo ', zobrazeno ' . count($users); ?></h5>
  <?php if (!$users): ?>
    <div class="card-body"><div class="alert alert-info mb-0">
      Nikdo takový tu není. Zkus jiné jméno.</div></div>
  <?php else: ?>
  <div class="table-responsive text-nowrap">
    <table class="table table-hover">
      <thead><tr><th>jméno</th><th>login</th><th>kontakt</th><th>oprávnění</th></tr></thead>
      <tbody class="table-border-bottom-0">
      <?php foreach ($users as $u):
          $prava = array_filter(explode(',', (string)$u['us_acl'])); ?>
        <tr>
          <td class="fw-semibold"><?= esc($u['us_name']) ?></td>
          <td><code><?= esc($u['us_login']) ?></code></td>
          <td class="text-muted"><small><?= esc($u['us_email']) ?><?php
              if ($u['us_phone']) echo ' · ' . esc($u['us_phone']); ?></small></td>
          <td>
            <?php if (!$prava) echo badge('čtenář', 'neutral');
                  else foreach ($prava as $p) echo badge($p, 'ok') . ' '; ?>
          </td>
        </tr>
      <?php endforeach; ?>
      </tbody>
    </table>
  </div>
  <div class="card-body pt-0">
    <?= pager('users_results', (int)$total, (int)$limit, (int)$offset, ['q' => $args['q'] ?? '']) ?>
  </div>
  <?php endif; ?>
</div>
