<?php
/* @var array $args  filtr výpůjček, veze se s sebou zpátky
   @var string $q    co je zrovna v hledání

   Filtr a výsledky jsou dva divy i uvnitř překryvu: při hledání se
   překresluje jen tabulka a v poli zůstane fokus. */
$nes = ['open' => $args['open'] ?? '', 'overdue' => $args['overdue'] ?? ''];
?>
<div class="fw-overlay" data-fw-esc="overlay">
  <div class="fw-overlay-box">
    <div class="fw-overlay-head">
      <h5 class="mb-0">Vybrat čtenáře</h5>
    </div>
    <div class="fw-overlay-body">
      <form data-fw action="#?function=reader_pick_results" method="post" class="mb-3">
        <?php foreach ($nes as $k => $v): ?>
          <input type="hidden" name="<?= esc($k) ?>" value="<?= esc((string)$v) ?>">
        <?php endforeach; ?>
        <div class="input-group">
          <input type="text" class="form-control" name="q" value="<?= esc($q) ?>"
                 placeholder="jméno nebo přihlašovací jméno" autofocus>
          <button type="submit" class="btn btn-outline-primary"><i class="bx bx-search"></i></button>
        </div>
      </form>
      <div id="reader_pick_results"></div>
    </div>
    <div class="fw-overlay-foot">
      <button type="button" class="btn btn-outline-secondary" data-fw-clear="overlay">Zrušit</button>
    </div>
  </div>
</div>
