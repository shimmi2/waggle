<?php
/* Levé menu. Kreslí se JEN při přihlášení a odhlášení — jeho obsah plyne
   z oprávnění, a ta se uprostřed práce nemění.

   Kreslení není autorizace: kdo si položku podvrhne, dostane z datového
   API 403. Tady se rozhoduje jen o tom, co má smysl nabízet. */
$m = me();
$kde = in_str('function', 64) ?: 'index';
$polozka = function (string $fn, string $ikona, string $popis) use ($kde) {
    $akt = $kde === $fn || strpos($kde, $fn . '_') === 0;
    printf('<li class="menu-item%s"><a href="#?function=%s" data-fw class="menu-link">'
         . '<i class="menu-icon tf-icons bx %s"></i><div>%s</div></a></li>',
           $akt ? ' active' : '', esc($fn), esc($ikona), esc($popis));
};
?>
<ul class="menu-inner py-1">
<?php if ($m === null): ?>
  <?php $polozka('index', 'bx-log-in', 'Přihlášení'); ?>
<?php else: ?>
  <?php
    $polozka('index',   'bx-home-circle',     'Úvod');
    $polozka('books',   'bx-book',            'Katalog');
    $polozka('genres',  'bx-collection',      'Žánry');
    $polozka('rentals', 'bx-transfer',        'Výpůjčky');
    if (may('see_statistics')) $polozka('statistics', 'bx-bar-chart-alt-2', 'Statistika');
    if (may('manage_users'))   $polozka('users',      'bx-user',            'Uživatelé');
  ?>
  <li class="menu-header small text-uppercase"><span class="menu-header-text">Účet</span></li>
  <li class="menu-item">
    <a href="#?function=do_logout" data-fw class="menu-link">
      <i class="menu-icon tf-icons bx bx-log-out"></i><div>Odhlásit</div>
    </a>
  </li>
<?php endif; ?>
</ul>
