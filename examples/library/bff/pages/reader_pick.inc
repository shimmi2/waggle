<?php
/* @var array $args  filtr výpůjček, veze se s sebou zpátky
   @var string $q    co je zrovna v hledání

   Výběrový překryv. Filtr a výsledky jsou DVA divy, stejně jako na
   obrazovce katalogu: při hledání se překresluje jen tabulka a v poli
   zůstane fokus, takže se dá psát dál.

   Zavření je stav klienta, ne aplikace — proto data-fw-clear a žádná
   cesta na server. */
$nes = ['open' => $args['open'] ?? '', 'overdue' => $args['overdue'] ?? ''];
?>
<div class="modal" data-fw-esc="overlay">
  <div class="modal-box">
    <h2>Vybrat čtenáře</h2>

    <form data-fw action="#?function=reader_pick_results" method="post" class="filter">
      <?php foreach ($nes as $k => $v): ?>
        <input type="hidden" name="<?= esc($k) ?>" value="<?= esc((string)$v) ?>">
      <?php endforeach; ?>
      <div class="filter-row">
        <label>Hledat<input type="text" name="q" value="<?= esc($q) ?>"
               placeholder="jméno nebo přihlašovací jméno" autofocus></label>
        <button type="submit">Hledat</button>
      </div>
    </form>

    <div id="reader_pick_results"></div>

    <div class="modal-foot">
      <button type="button" class="ghost" data-fw-clear="overlay">Zrušit</button>
    </div>
  </div>
</div>
