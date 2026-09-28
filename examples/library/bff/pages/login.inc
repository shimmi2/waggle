<?php /* @var string|null $err @var string|null $login */ ?>
<div class="card card-narrow">
  <h2>Přihlášení do knihovny</h2>
  <?php if (!empty($err)): ?>
    <p class="err"><?= esc($err) ?></p>
  <?php endif; ?>
  <form data-fw action="#?function=do_login" method="post" data-busy="Přihlašuji…">
    <label for="l">Jméno</label>
    <input type="text" id="l" name="login" autocomplete="username" value="<?= esc($login ?? '') ?>" autofocus>
    <label for="p">Heslo</label>
    <input type="password" id="p" name="password" autocomplete="current-password">
    <button type="submit">Přihlásit se</button>
  </form>
  <?php if (UNSAFE_DEMO): ?>
    <p class="hint">Ukázková instalace. Obsluha <code>hlavata</code>, čtenář <code>novakova</code>,
       heslo u obou <code>demo</code>. Rozdíl mezi nimi je jen v oprávnění.</p>
  <?php endif; ?>
</div>
