<?php
/* @var array $genres

   Filtr a výsledky jsou dva samostatné divy. Filtr pak zůstane stát
   i s fokusem a překresluje se jen tabulka pod ním.

   Hodnoty se předvyplňují z požadavku, protože na tuhle obrazovku se dá
   přijít i s hotovým filtrem — třeba kliknutím na žánr. */
$f_q     = in_str('q', 64);
$f_zanr  = in_int('genre');
$f_order = in_str('order', 32);
$f_avail = in_str('available', 1) === '1';
?>
<div class="card mb-4">
  <h5 class="card-header">Katalog</h5>
  <div class="card-body">
    <form data-fw action="#?function=books_results" method="post">
      <input type="hidden" name="offset" value="0">
      <div class="row g-3 align-items-end">
        <div class="col-md-4">
          <label class="form-label" for="f_q">Hledat</label>
          <input type="text" class="form-control" id="f_q" name="q"
                 placeholder="název nebo autor" value="<?= esc($f_q) ?>">
        </div>
        <div class="col-md-3">
          <label class="form-label" for="f_genre">Žánr</label>
          <!-- data-fw-submit odešle formulář při změně. Jde to proto, že
               se překresluje jen #books_results. -->
          <select class="form-select" id="f_genre" name="genre" data-fw-submit>
            <option value="0">— všechny —</option>
            <?php foreach ($genres as $g): ?>
              <option value="<?= (int)$g['ge_id'] ?>"
                <?= $f_zanr === (int)$g['ge_id'] ? 'selected' : '' ?>><?= esc($g['ge_name']) ?></option>
            <?php endforeach; ?>
          </select>
        </div>
        <div class="col-md-3">
          <label class="form-label" for="f_order">Řazení</label>
          <select class="form-select" id="f_order" name="order" data-fw-submit>
            <?php foreach (['name' => 'podle názvu', 'author' => 'podle autora',
                            'year' => 'nejnovější', 'price' => 'nejdražší'] as $k => $v): ?>
              <option value="<?= $k ?>" <?= $f_order === $k ? 'selected' : '' ?>><?= $v ?></option>
            <?php endforeach; ?>
          </select>
        </div>
        <div class="col-md-2">
          <div class="form-check mb-2">
            <input class="form-check-input" type="checkbox" id="f_avail" name="available"
                   value="1" data-fw-submit <?= $f_avail ? 'checked' : '' ?>>
            <label class="form-check-label" for="f_avail">jen volné</label>
          </div>
          <!-- type="submit" je výchozí, ale Enter v textovém poli stojí
               právě na tom, že tu submit tlačítko JE. -->
          <button type="submit" class="btn btn-primary btn-sm w-100">Hledat</button>
        </div>
      </div>
    </form>
  </div>
</div>
<div id="books_results"></div>
