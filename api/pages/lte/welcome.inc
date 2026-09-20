<div class="row justify-content-center">
  <div class="col-md-5">
    <div class="card card-primary card-outline">
      <div class="card-header"><h3 class="card-title">Přihlášení</h3></div>
      <form data-fw action="#?function=do_login" method="post">
        <div class="card-body">
          <p class="text-secondary">Zkuste <code>tomas</code> / <code>heslo</code>.</p>
          <div class="input-group mb-3">
            <span class="input-group-text"><i class="bi bi-person"></i></span>
            <input type="text" class="form-control" name="user" value="tomas" placeholder="Uživatel">
          </div>
          <div class="input-group mb-3">
            <span class="input-group-text"><i class="bi bi-key"></i></span>
            <input type="password" class="form-control" name="password" placeholder="Heslo">
          </div>
        </div>
        <div class="card-footer">
          <button type="submit" class="btn btn-primary w-100">
            <i class="bi bi-box-arrow-in-right me-1"></i>Přihlásit</button>
        </div>
      </form>
    </div>
  </div>
</div>
