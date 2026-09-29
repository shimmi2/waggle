<?php
/* @var array $book  hodnoty k předvyplnění (u nové knihy výchozí)
   @var array $genres
   @var string $op   'insert' nebo 'update'
   @var string|null $err
   @var array $filtr stav filtru katalogu

   JEDEN fragment pro editaci i přidání. Dva by se rozešly — do jednoho
   by se za měsíc přidalo pole a do druhého ne.

   Chyba z backendu se kreslí ZPÁTKY sem, se vším, co uživatel napsal.
   Proto jsou hodnoty polí vždy z $book, ne z databáze. */
$b    = $book;
$novy = ($op === 'insert');
$roky = (int)date('Y') + 1;
?>
<div class="fw-overlay" data-fw-esc="overlay">
  <!-- Formulář JE ten box. Kdyby byl uvnitř, stal by se jediným flex
       potomkem a hlavička s patičkou by přestaly držet svoje místo.

       data-busy patří na FORMULÁŘ, ne na tlačítko: při odeslání dostane
       busyBehem() právě formulář. Cíl musí být overlay, jinak by se
       rozostřil katalog pod dialogem a ne dialog sám. -->
  <form data-fw action="#?function=do_book_save" method="post"
        data-busy="Ukládám…" data-busy-sel="overlay" class="fw-overlay-box">
      <div class="fw-overlay-head">
        <h5 class="mb-0"><?= $novy ? 'Nová kniha' : 'Upravit knihu' ?></h5>
      </div>

      <div class="fw-overlay-body">
        <?php if (!empty($err)): ?>
          <div class="alert alert-danger" role="alert"><?= esc($err) ?></div>
        <?php endif; ?>

        <!-- Záměr říkáme výslovně. Kdyby se rozhodoval podle přítomnosti
             bo_id, ztracené id by tiše založilo duplikát. -->
        <input type="hidden" name="op"    value="<?= esc($op) ?>">
        <input type="hidden" name="bo_id" value="<?= $novy ? 0 : (int)$b['bo_id'] ?>">
        <?php /* Stav filtru putuje s formulářem, aby se po uložení
                 překreslil týž seznam. */
        foreach ($filtr as $k => $v): ?>
          <input type="hidden" name="f_<?= esc($k) ?>" value="<?= esc((string)$v) ?>">
        <?php endforeach; ?>

        <div class="mb-3">
          <label class="form-label" for="bo_name">Název <span class="text-danger">*</span></label>
          <input type="text" class="form-control" id="bo_name" name="bo_name"
                 maxlength="255" required autofocus value="<?= esc($b['bo_name']) ?>">
        </div>
        <div class="mb-3">
          <label class="form-label" for="bo_author">Autor</label>
          <input type="text" class="form-control" id="bo_author" name="bo_author"
                 maxlength="128" value="<?= esc($b['bo_author']) ?>">
        </div>
        <div class="row">
          <div class="col-sm-7 mb-3">
            <label class="form-label" for="bo_genre">Žánr</label>
            <select class="form-select" id="bo_genre" name="bo_genre">
              <option value="0">— bez žánru —</option>
              <?php foreach ($genres as $g): ?>
                <option value="<?= (int)$g['ge_id'] ?>"
                  <?= (int)$b['bo_genre'] === (int)$g['ge_id'] ? 'selected' : '' ?>>
                  <?= esc($g['ge_name']) ?></option>
              <?php endforeach; ?>
            </select>
          </div>
          <div class="col-sm-5 mb-3">
            <label class="form-label" for="bo_year">Rok vydání</label>
            <input type="number" class="form-control" id="bo_year" name="bo_year"
                   min="1" max="<?= $roky ?>" value="<?= (int)$b['bo_year'] ?: '' ?>">
          </div>
        </div>
        <div class="row">
          <div class="col-sm-6 mb-3">
            <label class="form-label" for="bo_count">Kusů v knihovně</label>
            <input type="number" class="form-control" id="bo_count" name="bo_count"
                   min="0" max="9999" value="<?= (int)$b['bo_count'] ?>">
          </div>
          <div class="col-sm-6 mb-3">
            <label class="form-label" for="bo_price">Cena (Kč)</label>
            <input type="number" class="form-control" id="bo_price" name="bo_price"
                   min="0" step="0.01" value="<?= esc(number_format((float)$b['bo_price'], 2, '.', '')) ?>">
          </div>
        </div>
        <div class="mb-0">
          <label class="form-label" for="bo_description">Popis</label>
          <textarea class="form-control" id="bo_description" name="bo_description"
                    rows="4" maxlength="4000"><?= esc($b['bo_description']) ?></textarea>
        </div>
    </div>

    <div class="fw-overlay-foot">
      <button type="button" class="btn btn-outline-secondary" data-fw-clear="overlay">Zrušit</button>
      <button type="submit" class="btn btn-primary"><?= $novy ? 'Založit' : 'Uložit' ?></button>
    </div>
  </form>
</div>
