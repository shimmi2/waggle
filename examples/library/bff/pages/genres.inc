<?php /* @var array $genres */ ?>
<div class="card">
  <h2>Žánry</h2>
  <table>
    <thead><tr><th>žánr</th><th>popis</th><th class="r">knih</th></tr></thead>
    <tbody>
    <?php foreach ($genres as $g): ?>
      <tr>
        <?php /* Na books, NE na books_results. books_results je částečné
                 překreslení a předpokládá, že #books_results v DOMu už je —
                 což na obrazovce žánrů není. Odkaz odjinud musí vždy mířit
                 na celou obrazovku; ta si svoje podokna vyrobí sama a pak
                 do nich propadne. */ ?>
        <td><a href="#?function=books&genre=<?= (int)$g['ge_id'] ?>" data-fw
               data-busy="Hledám v katalogu…"><?= esc($g['ge_name']) ?></a></td>
        <td><?= esc($g['ge_description']) ?></td>
        <td class="r"><?= (int)$g['ge_books'] ?></td>
      </tr>
    <?php endforeach; ?>
    </tbody>
  </table>
  <?php if (!may('manage_books')): ?>
    <p class="hint">Zakládat a měnit žánry může jen kdo má oprávnění <code>manage_books</code>.</p>
  <?php endif; ?>
</div>
