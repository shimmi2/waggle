<?php /* Levé menu. Obsah plyne z oprávnění — proto se nemusí překreslovat
         při běžné práci, jen při přihlášení a odhlášení.

         Co se tu NEkreslí, to není zakázané. Zákaz je na backendu; tady
         se jen neukazuje, co by uživateli stejně nefungovalo. */
$m = me();
if ($m === null): ?>
  <nav class="menu">
    <a href="#?function=index" data-fw class="on">Přihlášení</a>
  </nav>
<?php return; endif; ?>
<nav class="menu">
  <a href="#?function=books"    data-fw data-busy="Hledám v katalogu…">Knihy</a>
  <a href="#?function=genres"   data-fw>Žánry</a>
  <a href="#?function=rentals"  data-fw data-busy="Načítám výpůjčky…">Výpůjčky</a>
  <?php if (may('see_statistics')): ?>
    <!-- Statistika je jediná obrazovka, kde se doopravdy čeká. Text
         překryvu ukáže prohlížeč sám; procenta do něj doplní server. -->
    <a href="#?function=statistics" data-fw data-busy="Strpení prosím, počítám statistiku…">Statistika</a>
  <?php endif; ?>
  <?php if (may('manage_users')): ?>
    <a href="#?function=users" data-fw data-busy="Načítám uživatele…">Uživatelé</a>
  <?php endif; ?>
  <a href="#?function=do_logout" data-fw class="odhlasit">Odhlásit se</a>
</nav>
