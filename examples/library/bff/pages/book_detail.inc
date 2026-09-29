<?php
/* @var array $book  @var array $filtr  stav filtru katalogu

   Kreslí se do okna overlay. Zavření je stav klienta, ne aplikace —
   proto data-fw-clear a žádná cesta na server.

   Není to bootstrapí .modal: ten potřebuje JavaScript, který mu řekne
   show/hide, a tím by se stav dialogu přestěhoval na klienta. Vypadá to
   stejně, řídí to server. */
$b = $book; $vol = (int)$b['bo_available'];
$qf = http_build_query($filtr);
?>
<div class="fw-overlay" data-fw-esc="overlay">
  <div class="fw-overlay-box">
    <div class="fw-overlay-head">
      <h5 class="mb-1"><?= esc($b['bo_name']) ?></h5>
      <div class="text-muted small"><?= esc($b['bo_author']) ?><?= $b['bo_year'] ? ', ' . (int)$b['bo_year'] : '' ?>
        &middot; <?= esc($b['ge_name'] ?? 'bez žánru') ?></div>
    </div>
    <div class="fw-overlay-body">
      <?php if (!empty($b['bo_description'])): ?>
        <p><?= esc($b['bo_description']) ?></p>
      <?php endif; ?>
      <dl class="row mb-0">
        <dt class="col-5 fw-normal text-muted">kusů v knihovně</dt>
        <dd class="col-7"><?= (int)$b['bo_count'] ?></dd>
        <dt class="col-5 fw-normal text-muted">právě volných</dt>
        <dd class="col-7"><?= $vol > 0 ? $vol : badge('vše půjčeno', 'danger') ?></dd>
        <dt class="col-5 fw-normal text-muted">cena</dt>
        <dd class="col-7 mb-0"><?= esc(kc($b['bo_price'])) ?></dd>
      </dl>
      <?php if (may('rent_books') && $vol > 0): ?>
        <p class="text-muted small mt-3 mb-0">Půjčit může obsluha na obrazovce výpůjček.</p>
      <?php endif; ?>
    </div>
    <div class="fw-overlay-foot">
      <button type="button" class="btn btn-outline-secondary" data-fw-clear="overlay">Zavřít</button>
      <?php /* Editace překreslí týž overlay, takže dialog nevrství. */
      if (may('manage_books')): ?>
        <a class="btn btn-primary"
           href="#?function=book_form&bo_id=<?= (int)$b['bo_id'] ?>&<?= esc($qf) ?>" data-fw>Upravit</a>
      <?php endif; ?>
    </div>
  </div>
</div>
