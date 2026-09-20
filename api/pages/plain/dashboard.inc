<div class="card">
    <h2>Přehled</h2>
    <p>Přihlášen jako <b><?= esc($user['name']) ?></b> (<code><?= esc($user['login']) ?></code>).</p>
    <table class="kv">
        <tr><th>Oprávnění</th><td><?= esc(implode(', ', $user['acl']) ?: '—') ?></td></tr>
        <tr><th>Seriál instalace</th><td><code><?= esc($serial) ?></code></td></tr>
        <tr><th>Čas serveru</th><td><?= esc(date('H:i:s')) ?></td></tr>
    </table>
</div>

<div class="card" id="ping_card" style="margin-top:16px">
    <h2>Jemné adresování</h2>
    <p>Server nepřekresluje celou kartu — sáhne jen na <code>#ping_out</code>,
       přidá třídu kartě a změní atribut <code>title</code> odkazu.</p>
    <p><a href="#?function=ping" data-fw id="ping_btn" title="zatím neproběhlo">Ping</a>
       &nbsp;<b id="ping_out">—</b></p>
</div>

<p><small>Zkuste tlačítko <b>Zpět</b> v prohlížeči — historie funguje.</small></p>
