<?php
/* Horní rám: kdo je přihlášen a co mu běží. Překresluje se jen tehdy,
   když se některé z těch čísel změní — typicky po půjčení či vrácení.

   UNSAFE_DEMO je červený odznak, který nastavil instalák, když zjistil,
   že vrstvy nejsou oddělené. Nevypínej ho v konfiguraci — tím ten
   problém nezmizí, jen ta informace o něm. */
$m = me();
if ($m === null): ?>
  <div class="top">
    <span class="top-app">Knihovna</span>
    <span class="top-guest">nepřihlášen</span>
    <?php if (UNSAFE_DEMO): ?><span class="badge badge-danger">NEBEZPEČNÁ UKÁZKOVÁ INSTALACE</span><?php endif; ?>
  </div>
<?php return; endif;

/* Jedno volání, a jen kvůli souhrnu — proto limit 1. Data o výpůjčkách
   si bere obrazovka výpůjček sama. */
$mine = api_call('rentals_list', ['open' => '1', 'limit' => 1])['mine'] ?? [];
$open    = (int)($mine['open_count'] ?? 0);
$overdue = (int)($mine['overdue_count'] ?? 0);
$next    = $mine['next_due'] ?? null;
?>
<div class="top">
  <span class="top-app">Knihovna</span>
  <span class="top-user"><?= esc($m['us_name']) ?></span>
  <?php if ($open): ?>
    <span class="top-item">půjčeno: <b><?= $open ?></b></span>
    <?php if ($next): ?><span class="top-item">nejblíž vrátit: <b><?= den($next) ?></b></span><?php endif; ?>
    <?php if ($overdue): ?><?= badge(pocet($overdue,'po termínu','po termínu','po termínu'), 'danger') ?><?php endif; ?>
  <?php else: ?>
    <span class="top-item">nic nepůjčeno</span>
  <?php endif; ?>
  <?php if (UNSAFE_DEMO): ?><span class="badge badge-danger">NEBEZPEČNÁ UKÁZKOVÁ INSTALACE</span><?php endif; ?>
</div>
