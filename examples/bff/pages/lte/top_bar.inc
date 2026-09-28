<?php if ($user === null): ?>
  <li class="nav-item"><span class="nav-link text-secondary">
    <i class="bi bi-person-slash me-1"></i>Nepřihlášen</span></li>
<?php else: ?>
  <li class="nav-item"><span class="nav-link">
    <i class="bi bi-person-circle me-1"></i><?= esc($user['name']) ?></span></li>
  <li class="nav-item"><a class="nav-link" href="#?function=do_logout" data-fw
     data-bs-toggle="tooltip" title="Odhlásit se"><i class="bi bi-box-arrow-right"></i></a></li>
<?php endif; ?>
