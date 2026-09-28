<?php
/* =====================================================================
 *  install.php — webový instalák, VÝHRADNĚ na vyzkoušení
 *
 *  Co dělá: zeptá se na přístup k databázi, vytvoří schéma, nahraje
 *  ukázková data, založí admina a napíše tři konfigurace na místo, kde
 *  příklad leží. Nic nikam nekopíruje.
 *
 *  Co NEdělá: neoddělí vrstvy na tři domény, neřeší SSE ani nchan,
 *  nevytvoří runtime účet databáze, nenastaví webserver. Na to je
 *  install.sh.
 *
 *  BEZPEČNOST, a čtěte to prosím celé:
 *
 *  Tenhle soubor umí založit databázi a admina. Dokud leží ve webu, je
 *  to vzdálené převzetí serveru pro kohokoli, kdo na něj trefí. Proto:
 *
 *    - odmítne běžet, když už config.inc existuje (instalace hotova),
 *    - odmítne běžet bez HTTPS, protože se do něj píše heslo k databázi,
 *    - po dokončení se sám smaže a ověří si to stažením přes HTTP.
 *
 *  Přístupové údaje pro vytvoření schématu se NIKAM nezapisují. Žijí jen
 *  v té jedné POST proměnné a do config.inc se nedostanou.
 * ===================================================================== */

declare(strict_types=1);
@ini_set('display_errors', '0');

const KROK_DIR  = __DIR__;
const CFG_API   = KROK_DIR . '/api/config.inc';
const CFG_BFF   = KROK_DIR . '/bff/config.inc';
const CFG_APP   = KROK_DIR . '/app/config.js';

/* Escapování z knihovny. Instalák měl vlastní, pojmenované jedním
   písmenem, a nemělo ENT_SUBSTITUTE — bez něj vrací htmlspecialchars()
   pro neplatné UTF-8 PRÁZDNÝ řetězec. Kdo by měl ve jménu databáze nebo
   v hesle špatný bajt, přišel by o celé políčko a nedozvěděl se proč.

   post() naopak zůstává vlastní, a schválně: čte JEN z $_POST, kdežto
   in_str() bere i z GET. Do instaláku se píše heslo k databázi a to
   nesmí projít URL, kde by skončilo v access logu i v historii
   prohlížeče. */
require __DIR__ . '/api/inc/io.inc';

function post(string $k, int $max = 512): string {
    $v = $_POST[$k] ?? ''; if (!is_string($v)) return '';
    return str_replace("\0", '', substr($v, 0, $max));
}

/* ---- odmítnutí ------------------------------------------------------ */
$blok = null;
if (is_file(CFG_API))
    $blok = 'Instalace už proběhla — <code>api/config.inc</code> existuje. '
          . 'Jestli chceš instalovat znovu, smaž ho ručně; tenhle instalák '
          . 'hotovou instalaci nepřepíše, aby ji nešlo přepsat zvenčí.';

$https = ($_SERVER['HTTPS'] ?? '') !== '' || ($_SERVER['HTTP_X_FORWARDED_PROTO'] ?? '') === 'https'
         || in_array($_SERVER['REMOTE_ADDR'] ?? '', ['127.0.0.1', '::1'], true);
if ($blok === null && !$https)
    $blok = 'Tohle spojení není zabezpečené. Do formuláře se píše heslo k databázi '
          . 'a po nezašifrovaném spojení ho přečte každý po cestě. '
          . 'Zprovozni HTTPS, nebo instaluj z localhostu.';

/* ---- prostředí ----------------------------------------------------- */
$kontroly = [
    ['PHP 8.1 nebo novější', PHP_VERSION_ID >= 80100, PHP_VERSION],
    ['rozšíření pdo_mysql',  extension_loaded('pdo_mysql'), 'pro databázi'],
    ['rozšíření curl',       extension_loaded('curl'), 'BFF s ním volá API'],
    ['rozšíření redis',      extension_loaded('redis'), 'volitelné, bez něj jde session do MySQL'],
    ['zápis do api/',        is_writable(KROK_DIR . '/api'), 'sem se píše config.inc'],
    ['zápis do bff/',        is_writable(KROK_DIR . '/bff'), 'sem taky'],
    ['zápis do app/',        is_writable(KROK_DIR . '/app'), 'sem config.js'],
    ['knihovna fw.inc',      is_file(KROK_DIR . '/bff/fw.inc'), 'rozváží fwdeploy.sh'],
    ['knihovna fw.js',       is_file(KROK_DIR . '/app/fw.js'), 'taky'],
];
$povinne_ok = true;
foreach ($kontroly as $k) if (!$k[1] && strpos($k[2], 'volitelné') === false) $povinne_ok = false;

/* Je něco, co nemá být veřejné, opravdu veřejné? Neptáme se serveru na
   konfiguraci — zkusíme to stáhnout, protože jen to je důkaz. */
function zkus_stahnout(string $rel): ?int {
    $base = (($_SERVER['HTTPS'] ?? '') !== '' ? 'https' : 'http') . '://'
          . ($_SERVER['HTTP_HOST'] ?? 'localhost')
          . rtrim(dirname($_SERVER['SCRIPT_NAME'] ?? '/'), '/') . '/';
    $c = curl_init($base . $rel);
    curl_setopt_array($c, [CURLOPT_RETURNTRANSFER => true, CURLOPT_TIMEOUT => 4,
                           CURLOPT_NOBODY => false, CURLOPT_SSL_VERIFYPEER => false]);
    curl_exec($c);
    $kod = (int)curl_getinfo($c, CURLINFO_HTTP_CODE);
    curl_close($c);
    return $kod ?: null;
}

$hotovo = false; $chyba = null; $smazan = null; $vystaveno = [];

/* ---- instalace ----------------------------------------------------- */
if ($blok === null && ($_SERVER['REQUEST_METHOD'] ?? '') === 'POST' && $povinne_ok) {
    $host  = post('db_host', 128) ?: 'localhost';
    $dbnam = post('db_name', 64);
    $duser = post('prov_user', 64);
    $dpass = post('prov_pass', 256);
    $admin = post('admin_login', 64) ?: 'admin';
    $apass = post('admin_pass', 256);
    $profil= post('profil', 16) === 'mestska' ? 'mestska' : 'mala';
    $demo  = isset($_POST['demo_hesla']);
    $rhost = post('redis_host', 128);

    try {
        if ($dbnam === '' || !preg_match('/^[A-Za-z0-9_]+$/', $dbnam))
            throw new RuntimeException('Jméno databáze smí být jen písmena, číslice a podtržítko.');
        if (strlen($apass) < 8)
            throw new RuntimeException('Heslo správce musí mít aspoň osm znaků.');

        $dsn = "mysql:host=$host;charset=utf8mb4";
        $db = new PDO($dsn, $duser, $dpass, [PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION]);
        $db->exec("CREATE DATABASE IF NOT EXISTS `$dbnam` CHARACTER SET utf8mb4");
        $db->exec("USE `$dbnam`");

        /* Celý soubor jedním exec(). PDO s mysqlnd zvládne víc příkazů
           v jednom volání a chybu OHLÁSÍ i v tom pozdějším — ověřeno.

           Rozsekávat si to sám jsem zkoušel a byla to chyba: naivní
           dělení podle „;\n" s přeskočením bloků začínajících na „--"
           zahodí každý příkaz, před kterým je komentář. Ze sedmi tabulek
           vznikla jedna a instalace to ohlásila až o tři soubory dál
           jako „Table acls doesn't exist". Kdo tohle potřebuje dělit,
           musí napsat parser, který rozumí řetězcům — a na to není
           důvod, když to databáze umí sama. */
        foreach (['01-schema', '02-catalog', '03-books', '04-users'] as $f)
            $db->exec((string)file_get_contents(KROK_DIR . "/sql/$f.sql"));

        /* A hned si to ověřit. Polovičatě naimportovaná databáze, o které
           instalák tvrdí, že je hotová, je to nejhorší, co může vzniknout:
           aplikace se pak rozbíjí až u třetí obrazovky a nikdo netuší, že
           chyba je v instalaci. */
        $ceka = ['acls' => 5, 'genres' => 9, 'books' => 60, 'users' => 14,
                 'rentals' => 0, 'sessions' => 0, 'login_fails' => 0];
        $chybi = [];
        foreach ($ceka as $tab => $min) {
            try { $n = (int)$db->query("SELECT COUNT(*) FROM `$tab`")->fetchColumn(); }
            catch (Throwable $e) { $chybi[] = "$tab (tabulka chybí)"; continue; }
            if ($n < $min) $chybi[] = "$tab ($n řádků, čekal jsem aspoň $min)";
        }
        if ($chybi)
            throw new RuntimeException('Databáze se nenaimportovala celá: '
                . implode(', ', $chybi) . '. Smaž databázi a zkus to znovu.');

        $q = $db->prepare("INSERT INTO users (us_login, us_password, us_name, us_acl)
                           VALUES (?,?,?,?)");
        $q->execute([$admin, password_hash($apass, PASSWORD_DEFAULT), 'Správce',
                     'rent_books,see_all_rentals,see_statistics,manage_books,manage_users']);

        if ($demo) {
            $h = password_hash('demo', PASSWORD_DEFAULT);
            $db->prepare("UPDATE users SET us_password=? WHERE us_login IN ('hlavata','novakova')")
               ->execute([$h]);
        }

        /* Ukázková historie. Generuje se, ne stahuje — proto je balíček
           malý bez ohledu na profil.

           Volá se PŘÍMO jako funkce, se spojením, které tu už máme.
           Původně to bylo exec(PHP_BINARY . ' gen-rentals.php') a bylo to
           špatně ze dvou důvodů: pod PHP-FPM je PHP_BINARY
           /usr/sbin/php-fpm8.4, tedy správce procesů a ne interpret
           skriptů (odpovědí je výpis nápovědy php-fpm), a exec() je na
           sdíleném hostingu běžně v disable_functions. Instalák, který
           potřebuje shell, na hostingu neproleze — a to je zrovna to
           prostředí, pro které je určený. */
        require KROK_DIR . '/sql/gen-rentals.inc';
        [$dny, $kusu, $ctenaru] = $profil === 'mestska' ? [3650, 6, 400] : [730, 1, 40];
        $souhrn = gen_rentals($db, $dny, $kusu, $ctenaru);

        /* ---- konfigurace ------------------------------------------- */
        $base = (($_SERVER['HTTPS'] ?? '') !== '' ? 'https' : 'http') . '://'
              . ($_SERVER['HTTP_HOST'] ?? 'localhost')
              . rtrim(dirname($_SERVER['SCRIPT_NAME'] ?? '/'), '/');
        $sab = fn(string $f) => (string)file_get_contents($f);
        $nahrad = function (string $t, array $m): string {
            foreach ($m as $k => $v)
                $t = preg_replace("/define\('" . preg_quote($k, '/') . "',\s*[^)]*\)/",
                                  "define('$k', " . $v . ")", $t, 1);
            return $t;
        };
        $s = fn(string $v) => "'" . str_replace("'", "\\'", $v) . "'";

        file_put_contents(CFG_API, $nahrad($sab(KROK_DIR . '/api/config.example.inc'), [
            'DB_DSN'  => $s("mysql:host=$host;dbname=$dbnam;charset=utf8mb4"),
            'DB_USER' => $s($duser),
            'DB_PASS' => $s($dpass),
            'REDIS_HOST' => $s($rhost),
            'BFF_URL' => $s("$base/bff"),
            'UNSAFE_DEMO' => 'true',
        ]));
        file_put_contents(CFG_BFF, $nahrad($sab(KROK_DIR . '/bff/config.example.inc'), [
            'API_URL'      => $s("$base/api"),
            'FRONTEND_URL' => $s("$base/app"),
            'BFF_CACHE_DIR'=> $s(sys_get_temp_dir() . '/knihovna-sessions'),
            'UNSAFE_DEMO'  => 'true',
        ]));
        file_put_contents(CFG_APP, "window.LIB_BFF_URL = '" . $base . "/bff/';\n");
        @chmod(CFG_API, 0640); @chmod(CFG_BFF, 0640);

        /* ---- dosáhne BFF na API? ----------------------------------
           Tohle je NEJPRAVDĚPODOBNĚJŠÍ příčina, proč aplikace po
           instalaci nepojede, a projeví se jako „Služba není dostupná"
           na přihlašovací obrazovce.

           Detektor je elegantní sám sebou: instalák běží jako požadavek
           na tomtéž serveru jako BFF. Když se on odsud na API nedovolá,
           nedovolá se ani BFF — protože server obsluhuje jeden požadavek
           po druhém a ten druhý nemá kdo vzít.

           Přesně to dělá `php -S` bez PHP_CLI_SERVER_WORKERS. */
        $c = curl_init("$base/api/?fn=ping");
        curl_setopt_array($c, [CURLOPT_RETURNTRANSFER => true, CURLOPT_TIMEOUT => 6,
                               CURLOPT_SSL_VERIFYPEER => false]);
        $odpoved = curl_exec($c);
        $chyba_curl = curl_error($c);
        curl_close($c);
        $api_dosazitelne = $odpoved !== false && str_contains((string)$odpoved, 'pong');

        /* ---- co je vidět z internetu ------------------------------- */
        foreach (['api/config.inc' => 'konfigurace s heslem k databázi',
                  'api/inc/boot.inc' => 'kód backendu',
                  'sql/01-schema.sql' => 'schéma databáze',
                  'cronjobs/cleaner.php' => 'skript pro cron'] as $rel => $co) {
            $kod = zkus_stahnout($rel);
            if ($kod !== null && $kod === 200) $vystaveno[] = [$rel, $co];
        }

        /* ---- a teď sám sebe --------------------------------------- */
        @unlink(__FILE__);
        $kod = zkus_stahnout(basename(__FILE__));
        $smazan = ($kod !== null && $kod >= 400) || !is_file(__FILE__);

        $hotovo = true;
    } catch (Throwable $e) {
        $chyba = $e->getMessage();
    }
}
?><!DOCTYPE html>
<html lang="cs"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Knihovna — instalace</title>
<style>
body{font:15px/1.6 system-ui,sans-serif;max-width:720px;margin:2rem auto;padding:0 1rem;color:#1f2328}
h1{font-size:1.5rem}h2{font-size:1.1rem;margin-top:1.8rem}
.box{border:1px solid #dfe3e8;border-radius:8px;padding:14px 16px;margin:14px 0}
.bad{border-color:#b3261e;background:#fdf2f1}.warn{border-color:#9a6200;background:#fdf8ef}
.good{border-color:#1a7f4b;background:#f1f9f4}
label{display:block;margin:10px 0}input,select{width:100%;padding:7px;border:1px solid #ccd2d8;border-radius:5px;font:inherit}
input[type=checkbox]{width:auto}button{padding:9px 18px;border:0;border-radius:6px;background:#0e6a72;color:#fff;font:inherit;font-weight:600;cursor:pointer}
table{width:100%;border-collapse:collapse}td,th{text-align:left;padding:4px 8px;border-bottom:1px solid #eee}
code{background:#eef1f3;padding:.1em .35em;border-radius:3px}
.ok::before{content:"✓ ";color:#1a7f4b}.no::before{content:"✗ ";color:#b3261e}
</style></head><body>
<h1>Knihovna — instalace</h1>

<?php if ($blok !== null): ?>
  <div class="box bad"><b>Nejde pokračovat.</b><br><?= $blok ?></div>

<?php elseif ($hotovo): ?>
  <div class="box good"><b>Hotovo.</b> Aplikace je na
     <a href="app/">app/</a>. Přihlaš se jako <code><?= esc($admin) ?></code>.<br>
     <small>Ukázková data: <?= (int)($souhrn['operaci'] ?? 0) ?> operací,
     <?= (int)($souhrn['otevrenych'] ?? 0) ?> otevřených výpůjček
     (<?= (int)($souhrn['po_terminu'] ?? 0) ?> po termínu),
     <?= (int)($souhrn['ctenaru'] ?? 0) ?> čtenářů,
     <?= (int)($souhrn['dny'] ?? 0) ?> dní historie.</small></div>

  <div class="box <?= $smazan ? 'good' : 'bad' ?>">
    <?= $smazan ? '<b>Instalák se smazal.</b> Ověřeno stažením přes HTTP.'
                : '<b>Instalák se NEPODAŘILO smazat!</b> Smaž <code>install.php</code> ručně, '
                  . 'hned. Dokud tam leží, může ho spustit kdokoli.' ?>
  </div>

  <?php if (!$api_dosazitelne): ?>
    <div class="box bad">
      <b>BFF se nedovolá na backendové API — aplikace takhle nepojede.</b><br>
      Zkusil jsem <code><?= esc("$base/api/?fn=ping") ?></code> a nedostal odpověď
      <?= $chyba_curl !== '' ? '(<code>' . esc($chyba_curl) . '</code>)' : '' ?>.
      <br><br>
      <b>Nejčastější příčina:</b> webserver obsluhuje jeden požadavek po druhém.
      Vývojový server <code>php -S</code> to dělá — a protože tu všechny tři
      vrstvy leží pod jednou adresou, BFF se dovolává serveru, který je
      zaneprázdněný jím samým. Spusť ho s víc workery:
      <br><code>PHP_CLI_SERVER_WORKERS=4 php -S localhost:8000</code>
      <br><br>
      Na Apache ani nginxu s PHP-FPM tenhle problém není. Další možnost je,
      že hosting zakazuje odchozí HTTP nebo nepustí spojení na vlastní jméno;
      pak zkus do <code>bff/config.inc</code> dát <code>API_URL</code>
      na <code>127.0.0.1</code>.
    </div>
  <?php endif; ?>

  <?php if ($vystaveno): ?>
    <div class="box bad"><b>Tohle je vidět z internetu:</b>
      <ul><?php foreach ($vystaveno as [$rel, $co]): ?>
        <li><code><?= esc($rel) ?></code> — <?= esc($co) ?> (vrátilo <b>200</b>)</li>
      <?php endforeach; ?></ul>
      Zakaž to na webserveru — direktivy jsou v kapitole 09 dokumentace
      frameworku, pro Apache i nginx. Zvlášť ta konfigurace s heslem.
    </div>
  <?php endif; ?>

  <div class="box warn">
    <b>Tahle instalace není na provoz.</b> Všechny tři vrstvy leží pod jednou
    doménou, takže backendové API je dosažitelné z internetu a oddělení vrstev
    nechrání nic. Aplikace to hlásí červeným odznakem v horním rámu.
    Na produkci použij <code>install.sh</code>, který vrstvy rozkopíruje
    do vlastních domén.<br><br>
    Push přes SSE a nchan tahle instalace neřeší vůbec — kolečko „pracuji"
    funguje, procenta u statistiky ne.
  </div>

<?php else: ?>
  <div class="box warn">
    <b>Tohle je instalace na vyzkoušení, ne na provoz.</b><br>
    Zprovozní příklad tam, kde leží: tři vrstvy pod jednou doménou.
    Backendové API tím zůstane dosažitelné z internetu. Neřeší SSE ani
    nchan. Pro cokoli skutečného je tu <code>install.sh</code>.
  </div>

  <h2>Prostředí</h2>
  <table><?php foreach ($kontroly as [$co, $ok, $pozn]): ?>
    <tr><td class="<?= $ok ? 'ok' : 'no' ?>"><?= esc($co) ?></td><td><small><?= esc($pozn) ?></small></td></tr>
  <?php endforeach; ?></table>

  <?php if (!$povinne_ok): ?>
    <div class="box bad">Něco povinného chybí — doplň to a obnov stránku.</div>
  <?php else: ?>
    <?php if ($chyba !== null): ?><div class="box bad"><b>Nepovedlo se:</b> <?= esc($chyba) ?></div><?php endif; ?>
    <form method="post">
      <h2>Databáze</h2>
      <p><small>Tyhle údaje potřebují právo zakládat databázi a tabulky.
         <b>Nikam se nezapíšou</b> — do konfigurace se nedostanou.</small></p>
      <label>Server <input name="db_host" value="localhost"></label>
      <label>Jméno databáze <input name="db_name" value="knihovna" pattern="[A-Za-z0-9_]+"></label>
      <label>Uživatel <input name="prov_user" value="root"></label>
      <label>Heslo <input type="password" name="prov_pass"></label>
      <p><small>Pozn.: webový instalák zapíše do konfigurace <b>tyhle</b> údaje,
         protože na hostingu jiný účet vytvořit neumí. <code>install.sh</code>
         vyrobí runtime účet, který smí jen čtení a zápis dat.</small></p>

      <h2>Redis</h2>
      <label>Host <input name="redis_host" placeholder="prázdné = sezení do MySQL"
        <?= extension_loaded('redis') ? '' : 'disabled placeholder="rozšíření redis tu není"' ?>></label>

      <h2>Správce</h2>
      <label>Přihlašovací jméno <input name="admin_login" value="admin"></label>
      <label>Heslo <input type="password" name="admin_pass" minlength="8"></label>

      <h2>Ukázková data</h2>
      <label>Profil <select name="profil">
        <option value="mala">malá knihovna — 2 roky, ~9 tisíc operací, ~10 MB</option>
        <option value="mestska" selected>městská — 10 let, ~280 tisíc operací, ~50 MB</option>
      </select></label>
      <label><input type="checkbox" name="demo_hesla" checked> nastavit heslo
        <code>demo</code> kontům <code>hlavata</code> (obsluha) a
        <code>novakova</code> (čtenář), ať je vidět rozdíl v oprávněních</label>

      <p><button type="submit">Instalovat</button></p>
    </form>
  <?php endif; ?>
<?php endif; ?>
</body></html>
