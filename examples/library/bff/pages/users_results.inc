<?php
/* @var array $users  řádky jedné stránky
   @var int   $total  kolik jich filtru odpovídá CELKEM
   @var int   $limit
   @var int   $offset
   @var array $args   filtr, aby ho stránkování neztratilo */
?>
<div class="card">
  <h3><?= esc(pocet((int)$total, 'uživatel', 'uživatelé', 'uživatelů')) ?><?php
      if ((int)$total > (int)$limit) echo ', zobrazeno ' . count($users); ?></h3>
  <?php if (!$users): ?>
    <p class="hint">Nikdo takový tu není. Zkus jiné jméno.</p>
  <?php else: ?>
  <table>
    <thead><tr><th>jméno</th><th>login</th><th>kontakt</th><th>oprávnění</th></tr></thead>
    <tbody>
    <?php foreach ($users as $u):
        $prava = array_filter(explode(',', (string)$u['us_acl'])); ?>
      <tr>
        <td><?= esc($u['us_name']) ?></td>
        <td><code><?= esc($u['us_login']) ?></code></td>
        <td><small><?= esc($u['us_email']) ?><?= $u['us_phone'] ? ' · ' . esc($u['us_phone']) : '' ?></small></td>
        <td>
          <?php /* Stav odznakem, ne barvou řádku. */
          if (!$prava) echo badge('čtenář', 'neutral');
          else foreach ($prava as $p) echo badge($p, 'ok') . ' '; ?>
        </td>
      </tr>
    <?php endforeach; ?>
    </tbody>
  </table>
  <?= pager('users_results', (int)$total, (int)$limit, (int)$offset,
            ['q' => $args['q'] ?? '']) ?>
  <?php endif; ?>
</div>
