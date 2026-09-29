<?php
/* Horní rám: kdo je přihlášen a co mu běží. Překresluje se jen tehdy,
   když se některé z těch čísel změní — typicky po půjčení nebo vrácení.

   UNSAFE_DEMO je červený odznak, který nastavil instalák, když zjistil,
   že vrstvy nejsou oddělené. Nevypínej ho v konfiguraci — tím ten
   problém nezmizí, jen ta informace o něm. */
$m = me();
if ($m === null): ?>
  <ul class="navbar-nav flex-row align-items-center ms-auto">
    <li class="nav-item text-muted small me-3">nepřihlášen</li>
    <?php if (UNSAFE_DEMO): ?>
      <li class="nav-item"><span class="badge bg-label-danger">NEBEZPEČNÁ UKÁZKOVÁ INSTALACE</span></li>
    <?php endif; ?>
  </ul>
<?php return; endif;

/* Jedno volání, a jen kvůli souhrnu — proto limit 1. Data o výpůjčkách
   si bere obrazovka výpůjček sama. */
$mine = api_call('rentals_list', ['open' => '1', 'limit' => 1])['mine'] ?? [];
$open    = (int)($mine['open_count'] ?? 0);
$overdue = (int)($mine['overdue_count'] ?? 0);
$next    = $mine['next_due'] ?? null;
?>
<ul class="navbar-nav flex-row align-items-center ms-auto gap-3">
  <?php if (UNSAFE_DEMO): ?>
    <li class="nav-item d-none d-md-block">
      <span class="badge bg-label-danger">NEBEZPEČNÁ UKÁZKOVÁ INSTALACE</span></li>
  <?php endif; ?>
  <?php if ($open): ?>
    <li class="nav-item small text-muted">
      půjčeno <span class="fw-semibold text-body"><?= $open ?></span></li>
    <?php if ($next): ?>
      <li class="nav-item small text-muted d-none d-sm-block">
        nejblíž vrátit <span class="fw-semibold text-body"><?= den($next) ?></span></li>
    <?php endif; ?>
    <?php if ($overdue): ?>
      <li class="nav-item"><?= badge(pocet($overdue, 'po termínu', 'po termínu', 'po termínu'), 'danger') ?></li>
    <?php endif; ?>
  <?php else: ?>
    <li class="nav-item small text-muted">nic nepůjčeno</li>
  <?php endif; ?>
  <li class="nav-item d-flex align-items-center">
    <i class="bx bx-user-circle fs-4 me-1 text-muted"></i>
    <span class="fw-semibold"><?= esc($m['us_name']) ?></span>
  </li>
</ul>
