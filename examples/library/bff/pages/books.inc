<?php /* @var array $genres
         Filtr a výsledky jsou dva SAMOSTATNÉ divy. Filtr pak zůstane
         stát i s fokusem a překresluje se jen tabulka pod ním — takže
         uživatel může po odeslání rovnou psát dál.

         Hodnoty filtru se předvyplňují z požadavku, protože na tuhle
         obrazovku se dá přijít i s hotovým filtrem — třeba kliknutím na
         žánr. Bez toho by tabulka byla filtrovaná, ale rozbalovátko by
         tvrdilo „všechny", což je ta nejhorší kombinace: uživatel vidí
         výsledek, kterému neodpovídá ovládání. */
$f_q     = req('q', 64);
$f_zanr  = req_int('genre');
$f_order = req('order', 32);
$f_avail = req('available', 1) === '1';
?>
<div class="card">
  <h2>Katalog</h2>
  <form data-fw action="#?function=books_results" method="post" class="filter">
    <input type="hidden" name="offset" value="0">
    <div class="filter-row">
      <label>Hledat<input type="text" name="q" placeholder="název nebo autor"
             value="<?= esc($f_q) ?>"></label>
      <!-- data-fw-submit odešle formulář při změně. Jde to proto, že se
           překresluje jen #books_results. -->
      <label>Žánr
        <select name="genre" data-fw-submit>
          <option value="0">— všechny —</option>
          <?php foreach ($genres as $g): ?>
            <option value="<?= (int)$g['ge_id'] ?>"
              <?= $f_zanr === (int)$g['ge_id'] ? 'selected' : '' ?>><?= esc($g['ge_name']) ?></option>
          <?php endforeach; ?>
        </select>
      </label>
      <label>Řazení
        <select name="order" data-fw-submit>
          <?php foreach (['name' => 'podle názvu', 'author' => 'podle autora',
                          'year' => 'nejnovější', 'price' => 'nejdražší'] as $k => $v): ?>
            <option value="<?= $k ?>" <?= $f_order === $k ? 'selected' : '' ?>><?= $v ?></option>
          <?php endforeach; ?>
        </select>
      </label>
      <label class="check">
        <input type="checkbox" name="available" value="1" data-fw-submit
               <?= $f_avail ? 'checked' : '' ?>> jen volné
      </label>
      <!-- type="submit" je výchozí, ale Enter v textovém poli stojí právě
           na tom, že tu submit tlačítko JE. -->
      <button type="submit">Hledat</button>
    </div>
  </form>
</div>
<div id="books_results"></div>
