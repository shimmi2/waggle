<?php /* @var array $monthly @var array $genres @var array $top_books
         @var array $top_readers @var array $duration

         Pět přehledů, každý z vlastního volání backendu. Postup mezi
         nimi hlásí BFF, takže backend nemusí vědět nic o prohlížeči. */
$max = fn(array $rows, string $k) => max(1, ...array_map(fn($r) => (float)$r[$k], $rows ?: [[$k => 1]]));
?>
<div class="card">
  <h2>Statistika</h2>
  <p class="hint">Pět přehledů, pět volání backendu. Ukazatel postupu mezi nimi
     není ozdoba — každé procento znamená hotovou část.</p>
</div>

<div class="card">
  <h3>Výpůjčky po měsících</h3>
  <?php $m = array_slice($monthly, 0, 24); $mx = $max($m, 'n'); ?>
  <div class="bars">
    <?php foreach (array_reverse($m) as $r): ?>
      <div class="bar" title="<?= (int)$r['y'] ?>/<?= (int)$r['m'] ?>: <?= (int)$r['n'] ?>">
        <span style="height:<?= (int)round((int)$r['n'] / $mx * 100) ?>%"></span>
        <em><?= (int)$r['m'] ?></em>
      </div>
    <?php endforeach; ?>
  </div>
</div>

<div class="card two">
  <div>
    <h3>Podle žánrů</h3>
    <table><thead><tr><th>žánr</th><th class="r">výpůjček</th><th class="r">výnos</th></tr></thead><tbody>
      <?php foreach ($genres as $r): ?>
        <tr><td><?= esc($r['ge_name']) ?></td><td class="r"><?= (int)$r['n'] ?></td>
            <td class="r"><?= esc(kc($r['v'])) ?></td></tr>
      <?php endforeach; ?>
    </tbody></table>
  </div>
  <div>
    <h3>Doba držení po letech</h3>
    <table><thead><tr><th>rok</th><th class="r">průměr</th><th class="r">nejdéle</th></tr></thead><tbody>
      <?php foreach (array_slice($duration, 0, 10) as $r): ?>
        <tr><td><?= (int)$r['y'] ?></td><td class="r"><?= esc($r['days_avg']) ?> dní</td>
            <td class="r"><?= (int)$r['days_max'] ?> dní</td></tr>
      <?php endforeach; ?>
    </tbody></table>
  </div>
</div>

<div class="card two">
  <div>
    <h3>Nejpůjčovanější</h3>
    <table><thead><tr><th>kniha</th><th class="r">×</th></tr></thead><tbody>
      <?php foreach (array_slice($top_books, 0, 10) as $r): ?>
        <tr><td><?= esc($r['bo_name']) ?><br><small><?= esc($r['bo_author']) ?></small></td>
            <td class="r"><?= (int)$r['n'] ?></td></tr>
      <?php endforeach; ?>
    </tbody></table>
  </div>
  <div>
    <h3>Nejaktivnější čtenáři</h3>
    <table><thead><tr><th>čtenář</th><th class="r">×</th><th class="r">zaplatil</th></tr></thead><tbody>
      <?php foreach (array_slice($top_readers, 0, 10) as $r): ?>
        <tr><td><?= esc($r['us_name']) ?></td><td class="r"><?= (int)$r['n'] ?></td>
            <td class="r"><?= esc(kc($r['v'])) ?></td></tr>
      <?php endforeach; ?>
    </tbody></table>
  </div>
</div>
