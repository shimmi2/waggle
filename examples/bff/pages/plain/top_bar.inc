<?php if ($user === null): ?>
  <b>Nejste přihlášen</b>
<?php else: ?>
  Přihlášen jako <b><?= esc($user['name']) ?></b>
  — <a href="#?function=do_logout" data-fw>odhlásit</a>
<?php endif; ?>
