<div class="card">
    <h2 class="ok">Uloženo</h2>
    <p>Server přijal od <b><?= esc($user['name']) ?></b> tyto hodnoty:</p>
    <table class="kv">
        <tr><th>Název</th><td><?= esc($nazev) ?></td></tr>
        <tr><th>Počet</th><td><?= (int)$pocet ?></td></tr>
        <tr><th>Poznámka</th><td><?= esc($poznamka) ?></td></tr>
    </table>
    <p style="margin-top:16px"><a href="#?function=form_test" data-fw>Zpět na formulář</a></p>
</div>
