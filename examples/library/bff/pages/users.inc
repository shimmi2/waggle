<?php
/* @var array $acls  číselník oprávnění pro legendu
   @var array $args  filtr, jak ho sestavil dispatcher

   Filtr a výsledky jsou DVA divy. Filtr pak zůstane stát i s fokusem
   a překresluje se jen tabulka pod ním, takže se dá po odeslání rovnou
   psát dál. Legenda oprávnění patří sem, ne do výsledků — je to popis
   obrazovky, ne součást odpovědi, a při hledání se nemění. */
$f_q = (string)($args['q'] ?? '');
?>
<div class="card">
  <h2>Uživatelé</h2>
  <p class="hint">Obsluha i čtenáři jsou <b>titíž uživatelé</b>. Liší se výhradně
     oprávněním — žádné dvě aplikace, žádné dvě přihlašovací obrazovky.</p>
  <form data-fw action="#?function=users_results" method="post" class="filter">
    <input type="hidden" name="offset" value="0">
    <div class="filter-row">
      <label>Hledat<input type="text" name="q" value="<?= esc($f_q) ?>"
             placeholder="jméno, login nebo e-mail"></label>
      <button type="submit">Hledat</button>
    </div>
  </form>
</div>
<div id="users_results"></div>
<div class="card">
  <h3>Co která oprávnění znamenají</h3>
  <table><thead><tr><th>klíč</th><th>název</th><th>popis</th></tr></thead><tbody>
    <?php foreach ($acls as $a): ?>
      <tr><td><code><?= esc($a['ac_key']) ?></code></td><td><?= esc($a['ac_name']) ?></td>
          <td><small><?= esc($a['ac_description']) ?></small></td></tr>
    <?php endforeach; ?>
  </tbody></table>
  <p class="hint">Oprávnění jsou <b>řádky v číselníku</b>, ne <code>ENUM</code>.
     Nová funkce v aplikaci je jeden <code>INSERT</code>, nikdy <code>ALTER TABLE</code>.</p>
</div>
