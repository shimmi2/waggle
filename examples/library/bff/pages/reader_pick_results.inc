<?php
/* @var array $readers  us_id + us_name, nic víc
   @var int   $total    kolik jich filtru odpovídá CELKEM, ne na stránce
   @var int   $limit
   @var int   $offset
   @var array $args     filtr výpůjček, aby ho výběr nezahodil
   @var string $q       co se hledalo

   Odkaz „Vybrat" nese filtr výpůjček s sebou. Kdyby ho nenesl, výběr
   čtenáře by shodil zaškrtnutí „jen nevrácené" a tabulka pod překryvem
   by se překreslila jinak, než jak vypadala předtím. */
$nes = http_build_query([
    'open'    => $args['open']    ?? '',
    'overdue' => $args['overdue'] ?? '',
]);
?>
<?php if (!$readers): ?>
  <p class="hint">Nikdo takový tu není. Zkus jiné jméno<?= $q !== '' ? '' : ', nebo napiš aspoň kousek' ?>.</p>
<?php else: ?>
<p class="hint"><?= esc(pocet((int)$total, 'čtenář', 'čtenáři', 'čtenářů')) ?><?php
   if ((int)$total > (int)$limit) echo ', zobrazeno ' . count($readers); ?></p>
<table>
  <thead><tr><th>jméno</th><th class="r">id</th><th></th></tr></thead>
  <tbody>
  <?php foreach ($readers as $r): ?>
    <tr>
      <td><?= esc($r['us_name']) ?></td>
      <td class="r"><small><?= (int)$r['us_id'] ?></small></td>
      <td class="r"><a class="btn-small"
         href="#?function=reader_choose&us_id=<?= (int)$r['us_id'] ?>&<?= esc($nes) ?>"
         data-fw>Vybrat</a></td>
    </tr>
  <?php endforeach; ?>
  </tbody>
</table>
<?php /* Stránkování míří na reader_pick_results, tedy na tentýž div uvnitř
         překryvu — překryv zůstane otevřený a překreslí se jen tabulka.
         Hledání i filtr výpůjček jdou s odkazem dál. */ ?>
<?= pager('reader_pick_results', (int)$total, (int)$limit, (int)$offset,
          ['q'       => $q,
           'open'    => $args['open']    ?? '',
           'overdue' => $args['overdue'] ?? '']) ?>
<?php endif; ?>
