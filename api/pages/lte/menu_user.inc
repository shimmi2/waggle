<li class="nav-header"><?= esc(mb_strtoupper($user['name'])) ?></li>
<li class="nav-item">
    <a href="#?function=index" data-fw class="nav-link">
        <i class="nav-icon bi bi-speedometer2"></i><p>Přehled</p></a></li>

<!-- Rozbalovací strom: po překreslení menu ho musí znovu oživit
     adminlte.initialize() z hooku afterReplace. Když se rozbaluje,
     lifecycle funguje. -->
<li class="nav-item">
    <a href="#" class="nav-link">
        <i class="nav-icon bi bi-folder2-open"></i>
        <p>Zkoušky <i class="nav-arrow bi bi-chevron-right"></i></p></a>
    <ul class="nav nav-treeview">
        <li class="nav-item">
            <a href="#?function=form_test" data-fw class="nav-link">
                <i class="nav-icon bi bi-pencil-square"></i><p>Formulář</p></a></li>
        <li class="nav-item">
            <a href="#?function=page_time" data-fw class="nav-link">
                <i class="nav-icon bi bi-clock"></i><p>Čas serveru</p></a></li>
        <li class="nav-item">
            <a href="#?function=progress" data-fw class="nav-link">
                <i class="nav-icon bi bi-hourglass-split"></i><p>Progress bar</p></a></li>
        <li class="nav-item">
            <a href="#?function=sync" data-fw class="nav-link">
                <i class="nav-icon bi bi-window-stack"></i><p>Sync mezi okny</p></a></li>
        <li class="nav-item">
            <a href="#?function=boom" data-fw class="nav-link">
                <i class="nav-icon bi bi-exclamation-triangle text-danger"></i><p>Chyba 500</p></a></li>
    </ul>
</li>

<li class="nav-item">
    <a href="#?function=page_about" data-fw class="nav-link">
        <i class="nav-icon bi bi-info-circle"></i><p>O aplikaci</p></a></li>
<li class="nav-item">
    <a href="#?function=do_logout" data-fw class="nav-link">
        <i class="nav-icon bi bi-box-arrow-right"></i><p>Odhlásit</p></a></li>
