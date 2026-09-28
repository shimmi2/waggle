<?php
/* @var array       $book    hodnoty k předvyplnění (u nové knihy výchozí)
   @var array       $genres  seznam žánrů pro rozbalovátko
   @var string      $op      'insert' nebo 'update'
   @var string|null $err     hláška z backendu, pokud se uložení nepovedlo
   @var array       $filtr   stav filtru katalogu, ať se po uložení vrátí týž seznam

   JEDEN fragment pro editaci i přidání. Dva fragmenty by se rozešly —
   do jednoho by se za měsíc přidalo pole a do druhého ne. Rozdíl mezi
   „upravit" a „přidat" je jen v $op, v titulku a v tom, čím je $book
   naplněné.

   Kreslí se do okna overlay. Chyba z backendu se kreslí ZPÁTKY sem, se
   vším, co uživatel napsal — proto jsou hodnoty polí vždy z $book a ne
   z databáze. Kdyby se chyba vypsala jen jako notifikace, uživatel by
   při překlepu v počtu kusů přišel o celý popis knihy.

   Práva neřeší tenhle soubor. Jeho zobrazení je rozhodnutí o kreslení;
   skutečné „smíš" hlídá backend a vrátí 403. */
$b     = $book;
$novy  = ($op === 'insert');
$roky  = (int)date('Y') + 1;
?>
<div class="modal" data-fw-esc="overlay">
  <div class="modal-box">
    <h2><?= $novy ? 'Nová kniha' : 'Upravit knihu' ?></h2>

    <?php if (!empty($err)): ?>
      <p class="form-err"><?= esc($err) ?></p>
    <?php endif; ?>

    <!-- data-busy patří na FORMULÁŘ, ne na tlačítko: při odeslání dostane
         busyBehem() právě formulář. A cíl musí být overlay, jinak by se
         rozostřil katalog pod dialogem a ne dialog sám. -->
    <form data-fw action="#?function=do_book_save" method="post" class="form"
          data-busy="Ukládám…" data-busy-sel="overlay">
      <!-- Záměr říkáme výslovně. Kdyby se rozhodoval podle toho, jestli
           přišlo bo_id, ztracené id by znamenalo tiché založení duplikátu. -->
      <input type="hidden" name="op"    value="<?= esc($op) ?>">
      <input type="hidden" name="bo_id" value="<?= $novy ? 0 : (int)$b['bo_id'] ?>">

      <!-- Stav filtru putuje s formulářem, aby se po uložení překreslil
           týž seznam, na který uživatel kouká. Bez toho by se filtr při
           každém uložení resetoval. -->
      <?php foreach ($filtr as $k => $v): ?>
        <input type="hidden" name="f_<?= esc($k) ?>" value="<?= esc((string)$v) ?>">
      <?php endforeach; ?>

      <label>Název <span class="req">*</span>
        <input type="text" name="bo_name" maxlength="255" required autofocus
               value="<?= esc($b['bo_name']) ?>">
      </label>

      <label>Autor
        <input type="text" name="bo_author" maxlength="128"
               value="<?= esc($b['bo_author']) ?>">
      </label>

      <div class="form-row">
        <label>Žánr
          <select name="bo_genre">
            <option value="0">— bez žánru —</option>
            <?php foreach ($genres as $g): ?>
              <option value="<?= (int)$g['ge_id'] ?>"
                <?= (int)$b['bo_genre'] === (int)$g['ge_id'] ? 'selected' : '' ?>>
                <?= esc($g['ge_name']) ?></option>
            <?php endforeach; ?>
          </select>
        </label>

        <label>Rok vydání
          <input type="number" name="bo_year" min="1" max="<?= $roky ?>"
                 value="<?= (int)$b['bo_year'] ?: '' ?>">
        </label>
      </div>

      <div class="form-row">
        <label>Kusů v knihovně
          <input type="number" name="bo_count" min="0" max="9999"
                 value="<?= (int)$b['bo_count'] ?>">
        </label>

        <label>Cena (Kč)
          <input type="number" name="bo_price" min="0" step="0.01"
                 value="<?= esc(number_format((float)$b['bo_price'], 2, '.', '')) ?>">
        </label>
      </div>

      <label>Popis
        <textarea name="bo_description" rows="4" maxlength="4000"><?= esc($b['bo_description']) ?></textarea>
      </label>

      <div class="modal-foot">
        <button type="button" class="ghost" data-fw-clear="overlay">Zrušit</button>
        <button type="submit"><?= $novy ? 'Založit' : 'Uložit' ?></button>
      </div>
    </form>
  </div>
</div>
