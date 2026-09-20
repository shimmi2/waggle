<div class="row"><div class="col-lg-8">
<div class="card card-primary card-outline">
  <div class="card-header">
    <h3 class="card-title"><i class="bi bi-window-stack me-1"></i>Synchronizace mezi okny</h3>
    <div class="card-tools"><span id="sync_note" class="badge text-bg-secondary">zatím nic</span></div>
  </div>
  <div class="card-body">
    <div class="alert alert-info py-2 small">
      <i class="bi bi-arrow-left-right me-1"></i>
      Otevřete tuhle stránku <b>ve dvou záložkách</b> a hýbejte posuvníky.
      Server u toho vůbec není — dávky jdou přes <code>BroadcastChannel</code>,
      ale mají <b>stejný formát</b> jako odpověď API.
    </div>

    <?php foreach ([['s1','Jas','0','100','50'],['s2','Kontrast','0','100','30'],
                    ['s3','Hlasitost','0','11','7']] as [$id,$lbl,$min,$max,$val]): ?>
      <label class="form-label mb-1"><?= esc($lbl) ?>:
        <b id="<?= $id ?>_out"><?= esc($val) ?></b></label>
      <input type="range" class="form-range mb-3" id="<?= $id ?>" data-fw-sync
             min="<?= $min ?>" max="<?= $max ?>" value="<?= $val ?>">
    <?php endforeach; ?>

    <label class="form-label mb-1">Poznámka: <b id="t1_out"></b></label>
    <input type="text" class="form-control" id="t1" data-fw-sync placeholder="pište a dívejte se do druhé záložky">
  </div>
  <div class="card-footer small text-secondary">
    Jeden posun posuvníku = dávka tří příkazů:
    <code>value</code> na posuvník, <code>html</code> na popisek a projektová
    operace <code>sync_note</code>, která podle <code>ctx.origin</code> pozná,
    jestli změna přišla z tohohle okna, nebo z jiného.
  </div>
</div>
</div></div>
