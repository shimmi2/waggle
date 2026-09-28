<div class="card">
    <h2>Testovací formulář</h2>
    <?php if ($user === null): ?>
        <p class="err">Nejste přihlášen — odeslání vrátí HTTP&nbsp;401.</p>
    <?php endif; ?>
    <form data-fw action="#?function=save_form_test" method="post">
        <label for="n">Název <small>(povinný, jinak 400)</small></label>
        <input type="text" id="n" name="nazev" value="Pokusný záznam">
        <label for="c">Počet <small>(1–99, jinak 400)</small></label>
        <input type="number" id="c" name="pocet" value="3">
        <label for="p">Poznámka</label>
        <input type="text" id="p" name="poznamka" value="zkuste sem dát &lt;script&gt;">
        <button type="submit">Uložit</button>
    </form>
</div>
