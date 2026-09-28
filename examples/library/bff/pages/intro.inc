<div class="card">
  <h2>Vítejte v knihovně</h2>
  <p>Tohle je kickstart nad frameworkem Waggle. Tři vrstvy: tenhle frontend,
     BFF, které řídí aplikaci, a backendové API, které jediné vidí data.</p>
  <ul class="tiles">
    <li><a href="#?function=books"   data-fw data-busy="Hledám v katalogu…"><b>Knihy</b><span>katalog, filtr, dostupnost</span></a></li>
    <li><a href="#?function=rentals" data-fw data-busy="Načítám výpůjčky…"><b>Výpůjčky</b><span>co je venku a kdy se vrací</span></a></li>
    <?php if (may('see_statistics')): ?>
    <li><a href="#?function=statistics" data-fw data-busy="Strpení prosím, počítám statistiku…"><b>Statistika</b><span>pět přehledů, s ukazatelem postupu</span></a></li>
    <?php endif; ?>
  </ul>
  <?php if (!may('rent_books')): ?>
    <p class="hint">Jste přihlášen jako čtenář, takže vidíte jen svoje výpůjčky
       a nemůžete půjčovat ani vracet. Zkuste konto obsluhy a porovnejte —
       je to tatáž aplikace, liší se jen oprávnění.</p>
  <?php endif; ?>
</div>
