<div class="card">
  <h2>Synchronizace mezi okny <span id="sync_note" class="badge">zatím nic</span></h2>
  <p>Otevřete tuhle stránku <b>ve dvou záložkách</b> a hýbejte posuvníky.
     Server u toho vůbec není — dávky jdou přes <code>BroadcastChannel</code>,
     ale mají stejný formát jako odpověď API.</p>

  <?php foreach ([['s1','Jas',0,100,50],['s2','Kontrast',0,100,30],['s3','Hlasitost',0,11,7]] as [$id,$lbl,$min,$max,$val]): ?>
    <label for="<?= $id ?>"><?= esc($lbl) ?>: <b id="<?= $id ?>_out"><?= $val ?></b></label>
    <input type="range" id="<?= $id ?>" data-fw-sync min="<?= $min ?>" max="<?= $max ?>" value="<?= $val ?>">
  <?php endforeach; ?>

  <label for="t1">Poznámka: <b id="t1_out"></b></label>
  <input type="text" id="t1" data-fw-sync placeholder="pište a dívejte se do druhé záložky">
</div>
