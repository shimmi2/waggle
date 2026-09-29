<?php
/* @var array $readers  us_id + us_name, nic víc
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
<?php endif; ?>
