<div class="card">
    <h2>Vítejte</h2>
    <p>Nejste přihlášen. Zkuste <code>tomas</code> / <code>heslo</code>.</p>

    <form data-fw action="#?function=do_login" method="post">
        <label for="u">Uživatel</label>
        <input type="text" id="u" name="user" autocomplete="username" value="tomas">
        <label for="p">Heslo</label>
        <input type="password" id="p" name="password" autocomplete="current-password">
        <button type="submit">Přihlásit</button>
    </form>
</div>
