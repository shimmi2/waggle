<?php /* @var array $args  filtr, jak ho sestavil dispatcher

         Zaškrtávátka se kreslí z NĚJ, ne natvrdo. Dřív tu bylo pevné
         `checked`, takže obrazovka tvrdila „jen nevrácené" nad tabulkou,
         ve které byly i vrácené — a po odškrtnutí a překreslení se
         zaškrtnutí samo vracelo. */
$f_open    = !empty($args['open']);
$f_overdue = !empty($args['overdue']);
$f_user    = (int)($args['user'] ?? 0);
?>
<div class="card">
  <h2>Výpůjčky</h2>
  <?php if (!may('see_all_rentals')): ?>
    <p class="hint">Vidíte jen svoje výpůjčky. To omezení dělá <b>server</b>, ne tenhle
       filtr — poslat si cizí <code>user</code> nepomůže, backend ho přepíše.</p>
  <?php endif; ?>
  <form data-fw action="#?function=rentals_results" method="post" class="filter">
    <input type="hidden" name="offset" value="0">
    <div class="filter-row">
      <label class="check"><input type="checkbox" name="open" value="1"
             <?= $f_open ? 'checked' : '' ?> data-fw-submit> jen nevrácené</label>
      <label class="check"><input type="checkbox" name="overdue" value="1"
             <?= $f_overdue ? 'checked' : '' ?> data-fw-submit> jen po termínu</label>
      <?php if (may('see_all_rentals')): ?>
        <label>Čtenář (id)<input type="text" name="user" size="6"
               value="<?= $f_user ?: '' ?>"></label>
      <?php endif; ?>
      <button type="submit">Zobrazit</button>
    </div>
  </form>
</div>
<div id="rentals_results"></div>
