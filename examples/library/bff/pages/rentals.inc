<?php
/* @var array $args   filtr, jak ho sestavil dispatcher
   @var string $jmeno jméno vybraného čtenáře, doplněné ze serveru

   Zaškrtávátka se kreslí z FILTRU, ne natvrdo. Pevné `checked` je past
   ze dvou stran: při prvním příchodu tvrdí, že se filtruje, zatímco
   dotaz běží bez filtru — a po odškrtnutí se samo vrací zpátky. */
$f_open    = !empty($args['open']);
$f_overdue = !empty($args['overdue']);
$f_user    = (int)($args['user'] ?? 0);
$jmeno     = $jmeno ?? '';
$nes = http_build_query(['open' => $args['open'] ?? '', 'overdue' => $args['overdue'] ?? '']);
?>
<div class="card mb-4">
  <h5 class="card-header">Výpůjčky</h5>
  <div class="card-body">
    <?php if (!may('see_all_rentals')): ?>
      <div class="alert alert-info" role="alert">Vidíte jen svoje výpůjčky. To omezení dělá
        <b>server</b>, ne tenhle filtr — poslat si cizí <code>user</code> nepomůže,
        datové API ho přepíše.</div>
    <?php endif; ?>
    <form data-fw action="#?function=rentals_results" method="post">
      <input type="hidden" name="offset" value="0">
      <div class="row g-3 align-items-end">
        <div class="col-auto">
          <div class="form-check">
            <input class="form-check-input" type="checkbox" id="f_open" name="open" value="1"
                   data-fw-submit <?= $f_open ? 'checked' : '' ?>>
            <label class="form-check-label" for="f_open">jen nevrácené</label>
          </div>
          <div class="form-check">
            <input class="form-check-input" type="checkbox" id="f_overdue" name="overdue" value="1"
                   data-fw-submit <?= $f_overdue ? 'checked' : '' ?>>
            <label class="form-check-label" for="f_overdue">jen po termínu</label>
          </div>
        </div>
        <?php if (may('see_all_rentals')): ?>
          <div class="col-md-4">
            <label class="form-label" for="f_user_name">Čtenář</label>
            <?php /* Jméno je jen na čtení a do formuláře se NEODESÍLÁ —
                     disabled prvky prohlížeč neposílá, a je to tak
                     správně: filtruje se podle id, ne podle textu. */ ?>
            <div class="input-group">
              <input type="text" class="form-control" id="f_user_name" disabled
                     placeholder="kdokoli" value="<?= esc($jmeno) ?>">
              <a class="btn btn-outline-primary" href="#?function=reader_pick&<?= esc($nes) ?>"
                 data-fw><i class="bx bx-search"></i></a>
              <?php if ($f_user): ?>
                <a class="btn btn-outline-secondary" title="zrušit výběr"
                   href="#?function=reader_choose&us_id=0&<?= esc($nes) ?>" data-fw>
                   <i class="bx bx-x"></i></a>
              <?php endif; ?>
            </div>
            <input type="hidden" name="user" id="f_user" value="<?= $f_user ?: '' ?>">
          </div>
        <?php endif; ?>
        <div class="col-auto">
          <button type="submit" class="btn btn-primary btn-sm">Zobrazit</button>
        </div>
      </div>
    </form>
  </div>
</div>
<div id="rentals_results"></div>
