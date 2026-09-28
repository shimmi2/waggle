<div class="card">
    <h2>Čas serveru</h2>
    <p>A tohle je <b>generovaná</b> stránka <code>page_time.inc</code> — stejná
       zkratka, ale soubor se includuje, takže může počítat.</p>
    <table class="kv">
        <tr><th>Datum a čas</th><td><?= esc(date('Y-m-d H:i:s')) ?></td></tr>
        <tr><th>PHP</th><td><?= esc(PHP_VERSION) ?></td></tr>
        <tr><th>Unix time</th><td><?= time() ?></td></tr>
    </table>
</div>
