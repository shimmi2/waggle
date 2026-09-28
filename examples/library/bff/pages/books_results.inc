<?php /* @var array $books @var int $total @var int $limit @var int $offset @var array $args */ ?>
<?php /* total se počítá BEZ filtru dostupnosti, protože ten je drahý.
         Hláška to tedy musí umět říct, jinak vyjde „3 knihy v katalogu"
         a hned pod tím „nic tu nemáme", což je protimluv. */
$jen_volne = !empty($args['available']);

/* Filtr putuje s KAŽDÝM odkazem, který vede na zápisový dialog. Tenhle
   fragment je jediné místo, které zná jeho aktuální podobu — hlavička s
   rozbalovátky se po změně filtru nepřekresluje, takže odkaz v ní by
   nesl stav, který už neplatí.

   Stejně to dělá pager pod tabulkou; není to výjimka pro formulář. */
$filtr = ['q' => $args['q'] ?? '', 'genre' => $args['genre'] ?? 0,
          'available' => $args['available'] ?? '', 'order' => $args['order'] ?? '',
          'offset' => $args['offset'] ?? 0];
$qf = http_build_query($filtr);
$spravuje = may('manage_books'); ?>
<div class="card">
  <div class="card-head">
    <h3><?= esc(pocet((int)$total, 'kniha', 'knihy', 'knih')) ?> v katalogu<?php
        if ($jen_volne && $books) echo ', zobrazeny volné'; ?></h3>
    <?php /* Kreslení není autorizace: kdyby si někdo tlačítko podvrhl,
             dostane z backendu 403. Tady se rozhoduje jen o tom, co má
             smysl nabízet. */
    if ($spravuje): ?>
      <a class="btn" href="#?function=book_form&<?= esc($qf) ?>" data-fw>Přidat knihu</a>
    <?php endif; ?>
  </div>
  <?php if (!$books && $jen_volne && $total > 0): ?>
    <p class="hint">Filtru odpovídá <?= esc(pocet((int)$total, 'kniha', 'knihy', 'knih')) ?>,
       ale žádná není právě volná. Zkus vypnout „jen volné".</p>
  <?php elseif (!$books): ?>
    <p class="hint">Nic takového tu nemáme. Zkus jiný filtr.</p>
  <?php else: ?>
  <table>
    <thead><tr><th>název</th><th>autor</th><th>žánr</th><th class="r">rok</th>
               <th class="r">dostupnost</th><th class="r">cena</th>
               <?php if ($spravuje): ?><th></th><?php endif; ?></tr></thead>
    <tbody>
    <?php foreach ($books as $b):
        $vol = (int)$b['bo_available']; $kusu = (int)$b['bo_count']; ?>
      <tr>
        <td><a href="#?function=book_detail&bo_id=<?= (int)$b['bo_id'] ?>&<?= esc($qf) ?>"
               data-fw><?= esc($b['bo_name']) ?></a></td>
        <td><?= esc($b['bo_author']) ?></td>
        <td><?= esc($b['ge_name'] ?? '—') ?></td>
        <td class="r"><?= $b['bo_year'] ? (int)$b['bo_year'] : '—' ?></td>
        <td class="r">
          <?php /* Stav odznakem, ne barvou řádku — obarvený řádek se
                   v dlouhé tabulce čte špatně. */
          if ($vol <= 0)            echo badge('vše půjčeno', 'danger');
          elseif ($vol === 1)       echo badge('poslední kus', 'warn');
          else                      echo badge($vol . ' z ' . $kusu, 'ok'); ?>
        </td>
        <td class="r"><?= esc(kc($b['bo_price'])) ?></td>
        <?php if ($spravuje): ?>
          <td class="r"><a class="btn-small"
             href="#?function=book_form&bo_id=<?= (int)$b['bo_id'] ?>&<?= esc($qf) ?>"
             data-fw>Upravit</a></td>
        <?php endif; ?>
      </tr>
    <?php endforeach; ?>
    </tbody>
  </table>
  <?= pager('books_results', (int)$total, (int)$limit, (int)$offset,
            ['q' => $args['q'] ?? '', 'genre' => $args['genre'] ?? 0,
             'available' => $args['available'] ?? '', 'order' => $args['order'] ?? '']) ?>
  <?php endif; ?>
</div>
