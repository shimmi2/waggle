<?php /* @var string|null $err @var string|null $login */ ?>
<div class="row justify-content-center">
  <div class="col-md-6 col-lg-5">
    <div class="card">
      <div class="card-body">
        <h4 class="mb-2">Přihlášení do knihovny</h4>
        <?php if (!empty($err)): ?>
          <div class="alert alert-danger" role="alert"><?= esc($err) ?></div>
        <?php endif; ?>
        <form data-fw action="#?function=do_login" method="post" data-busy="Přihlašuji…">
          <div class="mb-3">
            <label for="l" class="form-label">Jméno</label>
            <input type="text" class="form-control" id="l" name="login"
                   autocomplete="username" value="<?= esc($login ?? '') ?>" autofocus>
          </div>
          <div class="mb-3">
            <label for="p" class="form-label">Heslo</label>
            <input type="password" class="form-control" id="p" name="password"
                   autocomplete="current-password">
          </div>
          <button type="submit" class="btn btn-primary w-100">Přihlásit se</button>
        </form>
        <?php if (UNSAFE_DEMO): ?>
          <div class="alert alert-warning mt-3 mb-0 small" role="alert">
            Ukázková instalace. Obsluha <code>hlavata</code>, čtenář <code>novakova</code>,
            heslo u obou <code>demo</code>. Rozdíl mezi nimi je jen v oprávnění.
          </div>
        <?php endif; ?>
      </div>
    </div>
  </div>
</div>
