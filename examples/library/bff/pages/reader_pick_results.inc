<?php
/* @var array $readers  us_id + us_name, nic víc
   @var int $total @var int $limit @var int $offset
   @var array $args  filtr výpůjček, aby ho výběr nezahodil
   @var string $q

   Odkaz „Vybrat" nese filtr výpůjček s sebou. Kdyby ho nenesl, výběr
   čtenáře by shodil zaškrtnutí „jen nevrácené" a tabulka pod překryvem
   by se překreslila jinak, než jak vypadala předtím. */
$nes = http_build_query([
    'open'    => $args['open']    ?? '',
    'overdue' => $args['overdue'] ?? '',
]);
?>
<?php if (!$readers): ?>
  <div class="alert alert-info mb-0">Nikdo takový tu není. Zkus jiné jméno.</div>
<?php else: ?>
<p class="text-muted small"><?= esc(pocet((int)$total, 'čtenář', 'čtenáři', 'čtenářů')) ?><?php
   if ((int)$total > (int)$limit) echo ', zobrazeno ' . count($readers); ?></p>
<div class="table-responsive text-nowrap">
  <table class="table table-sm table-hover">
    <thead><tr><th>jméno</th><th class="fw-num">id</th><th></th></tr></thead>
    <tbody class="table-border-bottom-0">
    <?php foreach ($readers as $r): ?>
      <tr>
        <td class="fw-semibold"><?= esc($r['us_name']) ?></td>
        <td class="fw-num text-muted"><small><?= (int)$r['us_id'] ?></small></td>
        <td class="fw-num"><a class="btn btn-sm btn-primary"
           href="#?function=reader_choose&us_id=<?= (int)$r['us_id'] ?>&<?= esc($nes) ?>"
           data-fw>Vybrat</a></td>
      </tr>
    <?php endforeach; ?>
    </tbody>
  </table>
</div>
<?php /* Stránkování míří na reader_pick_results, tedy na tentýž div uvnitř
         překryvu — překryv zůstane otevřený a překreslí se jen tabulka. */ ?>
<?= pager('reader_pick_results', (int)$total, (int)$limit, (int)$offset,
          ['q' => $q, 'open' => $args['open'] ?? '', 'overdue' => $args['overdue'] ?? '']) ?>
<?php endif; ?>
