<?php /* @var array $users @var array $acls */ ?>
<div class="card">
  <h2>Uživatelé</h2>
  <p class="hint">Obsluha i čtenáři jsou <b>titíž uživatelé</b>. Liší se výhradně
     oprávněním — žádné dvě aplikace, žádné dvě přihlašovací obrazovky.</p>
  <table>
    <thead><tr><th>jméno</th><th>login</th><th>kontakt</th><th>oprávnění</th></tr></thead>
    <tbody>
    <?php foreach ($users as $u):
        $prava = array_filter(explode(',', (string)$u['us_acl'])); ?>
      <tr>
        <td><?= esc($u['us_name']) ?></td>
        <td><code><?= esc($u['us_login']) ?></code></td>
        <td><small><?= esc($u['us_email']) ?><?= $u['us_phone'] ? ' · ' . esc($u['us_phone']) : '' ?></small></td>
        <td>
          <?php if (!$prava) echo badge('čtenář', 'neutral');
                else foreach ($prava as $p) echo badge($p, 'ok') . ' '; ?>
        </td>
      </tr>
    <?php endforeach; ?>
    </tbody>
  </table>
</div>
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
