<div class="row"><div class="col-md-7">
<div class="card card-primary card-outline">
  <div class="card-header"><h3 class="card-title">Testovací formulář</h3></div>
  <form data-fw action="#?function=save_form_test" method="post">
    <div class="card-body">
      <?php if ($user === null): ?>
        <div class="alert alert-warning py-2">Nejste přihlášen — odeslání vrátí HTTP&nbsp;401.</div>
      <?php endif; ?>
      <div class="mb-3">
        <label class="form-label">Název <small class="text-secondary">(povinný, jinak 400)</small></label>
        <input type="text" class="form-control" name="nazev" value="Pokusný záznam"></div>
      <div class="mb-3">
        <label class="form-label">Počet <small class="text-secondary">(1–99, jinak 400)</small></label>
        <input type="number" class="form-control" name="pocet" value="3"></div>
      <div class="mb-0">
        <label class="form-label">Poznámka</label>
        <input type="text" class="form-control" name="poznamka" value="&lt;script&gt;alert(1)&lt;/script&gt;"></div>
    </div>
    <div class="card-footer">
      <button type="submit" class="btn btn-primary"><i class="bi bi-save me-1"></i>Uložit</button>
    </div>
  </form>
</div></div></div>
