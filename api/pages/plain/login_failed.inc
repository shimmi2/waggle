<div class="card">
    <h2>Přihlášení se nezdařilo</h2>
    <p class="err">Uživatel <b><?= esc($login) ?></b> — neplatné jméno nebo heslo.</p>
    <form data-fw action="#?function=do_login" method="post">
        <label for="u">Uživatel</label>
        <input type="text" id="u" name="user" value="<?= esc($login) ?>">
        <label for="p">Heslo</label>
        <input type="password" id="p" name="password">
        <button type="submit">Zkusit znovu</button>
    </form>
</div>
