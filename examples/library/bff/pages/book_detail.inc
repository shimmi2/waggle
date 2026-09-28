<?php /* @var array $book
         @var array $filtr  stav filtru katalogu, putuje dál do editace

         Kreslí se do okna overlay. Zavření je stav klienta, ne
         aplikace — proto data-fw-clear a žádná cesta na server. */
$b = $book; $vol = (int)$b['bo_available'];
/* Filtr jde s odkazem na editaci dál, aby se po uložení překreslil týž
   seznam, ze kterého uživatel na detail kliknul. */
$qf = http_build_query($filtr); ?>
<div class="modal" data-fw-esc="overlay">
  <div class="modal-box">
    <h2><?= esc($b['bo_name']) ?></h2>
    <p class="modal-sub"><?= esc($b['bo_author']) ?><?= $b['bo_year'] ? ', ' . (int)$b['bo_year'] : '' ?>
       &middot; <?= esc($b['ge_name'] ?? 'bez žánru') ?></p>
    <?php if (!empty($b['bo_description'])): ?><p><?= esc($b['bo_description']) ?></p><?php endif; ?>
    <table class="kv">
      <tr><th>kusů v knihovně</th><td><?= (int)$b['bo_count'] ?></td></tr>
      <tr><th>právě volných</th><td><?= $vol > 0 ? $vol : badge('vše půjčeno', 'danger') ?></td></tr>
      <tr><th>cena</th><td><?= esc(kc($b['bo_price'])) ?></td></tr>
    </table>
    <div class="modal-foot">
      <?php if (may('rent_books') && $vol > 0): ?>
        <span class="hint">Půjčit může obsluha na obrazovce výpůjček.</span>
      <?php endif; ?>
      <button data-fw-clear="overlay">Zavřít</button>
      <?php /* Editace překreslí týž overlay, takže dialog nevrství. */
      if (may('manage_books')): ?>
        <a class="btn"
           href="#?function=book_form&bo_id=<?= (int)$b['bo_id'] ?>&<?= esc($qf) ?>"
           data-fw>Upravit</a>
      <?php endif; ?>
    </div>
  </div>
</div>
