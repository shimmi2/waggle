<div class="row justify-content-center"><div class="col-md-5">
<div class="card card-danger card-outline">
  <div class="card-header"><h3 class="card-title">Přihlášení se nezdařilo</h3></div>
  <form data-fw action="#?function=do_login" method="post">
    <div class="card-body">
      <div class="alert alert-danger py-2 mb-3">
        Uživatel <b><?= esc($login) ?></b> — neplatné jméno nebo heslo.</div>
      <div class="input-group mb-3">
        <span class="input-group-text"><i class="bi bi-person"></i></span>
        <input type="text" class="form-control" name="user" value="<?= esc($login) ?>"></div>
      <div class="input-group mb-0">
        <span class="input-group-text"><i class="bi bi-key"></i></span>
        <input type="password" class="form-control" name="password" placeholder="Heslo"></div>
    </div>
    <div class="card-footer"><button type="submit" class="btn btn-primary w-100">Zkusit znovu</button></div>
  </form>
</div></div></div>
