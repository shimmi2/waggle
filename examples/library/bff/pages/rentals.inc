<?php /* @var array $args  filtr, jak ho sestavil dispatcher

         Zaškrtávátka se kreslí z NĚJ, ne natvrdo. Dřív tu bylo pevné
         `checked`, takže obrazovka tvrdila „jen nevrácené" nad tabulkou,
         ve které byly i vrácené — a po odškrtnutí a překreslení se
         zaškrtnutí samo vracelo. */
$f_open    = !empty($args['open']);
$f_overdue = !empty($args['overdue']);
$f_user    = (int)($args['user'] ?? 0);
$jmeno     = $jmeno ?? '';

/* Odkaz na výběr nese stav filtru s sebou. Bez toho by výběr čtenáře
   shodil „jen nevrácené" a tabulka by se překreslila jinak, než jak
   vypadala předtím. */
$nes = http_build_query(['open' => $args['open'] ?? '', 'overdue' => $args['overdue'] ?? '']);
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
        <?php /* Jméno je jen na čtení a do formuláře se NEODESÍLÁ — disabled
                 prvky prohlížeč neposílá, a je to tak správně: filtruje se
                 podle id, ne podle textu. Id proto jede zvlášť, skrytě. */ ?>
        <label>Čtenář
          <input type="text" id="f_user_name" size="18" disabled
                 placeholder="kdokoli" value="<?= esc($jmeno) ?>">
        </label>
        <input type="hidden" name="user" id="f_user" value="<?= $f_user ?: '' ?>">
        <a class="btn-small" href="#?function=reader_pick&<?= esc($nes) ?>" data-fw>Vybrat</a>
        <?php if ($f_user): ?>
          <a class="btn-small ghost" title="zrušit výběr"
             href="#?function=reader_choose&us_id=0&<?= esc($nes) ?>" data-fw>×</a>
        <?php endif; ?>
      <?php endif; ?>
      <button type="submit">Zobrazit</button>
    </div>
  </form>
</div>
<div id="rentals_results"></div>
