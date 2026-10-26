<?php
/* =====================================================================
 *  Backend API — konfigurace
 *
 *  Instalák z tohohle udělá config.inc.
 *
 *  Tahle vrstva je JEDINÁ, která zná databázi a Redis. BFF ani frontend
 *  o nich nevědí — a proto se k nim nedostanou ani omylem. Není to
 *  formalita: kdyby heslo k databázi leželo i v konfiguraci BFF,
 *  prolomený BFF ho vydá, přesto že ho nikdy nepoužil.
 * ===================================================================== */

/* ---- databáze -------------------------------------------------------
   Runtime účet má mít JEN SELECT, INSERT, UPDATE, DELETE na tohle jedno
   schéma. Žádné DDL — aplikace za provozu nemá co měnit strukturu.
   Shellový instalák takový účet vytvoří sám. */
define('DB_DSN',  'mysql:host=localhost;dbname=knihovna;charset=utf8mb4');
define('DB_USER', 'knihovna');
define('DB_PASS', '');

/* ---- Redis (volitelné) ---------------------------------------------
   Prázdný host = sezení a počítadla přihlášení jdou do MySQL. Funguje
   to stejně, jen se o úklid musí postarat cronjobs/cleaner.php.
   Redis si klíče maže sám podle TTL. Nastavený, ale nedostupný Redis
   znamená 503 — API na MySQL za běhu nepřepíná, rozdělilo by sezení
   do dvou úložišť (viz docs/10, Session). */
define('REDIS_HOST',   '');          // '127.0.0.1'
define('REDIS_PORT',   6379);
define('REDIS_PREFIX', 'lib.');

/* ---- sezení ---------------------------------------------------------
   Životnost se posouvá při každém použití, měří se tedy od poslední
   činnosti, ne od přihlášení. */
define('SESSION_TTL', 3600);

/* ---- ochrana přihlašování ------------------------------------------
   Hlavní důvod není uhádnuté heslo, ale cena ověření: bcrypt trvá kolem
   150 ms, přičtení v Redisu 0,07 ms. Bez škrtiče stačí šest útočníků
   k vytížení jádra. Dva klíče: adresa celkem (PER_IP) a adresa na jedno
   jméno (PER_USER). Podle jména samotného ne — šlo by zamknout účet
   právoplatnému uživateli z jakékoli adresy. */
define('LOGIN_FAIL_WINDOW',   900);
define('LOGIN_FAIL_PER_IP',    20);
define('LOGIN_FAIL_PER_USER',   5);

/* ---- adresa BFF -----------------------------------------------------
   Jen kvůli CORS: prohlížeč volá tohle API z jiného jména, takže pošle
   preflight. Pouští se sem VÝHRADNĚ tahle jedna adresa, nikdy '*' —
   z cizí stránky nemá tohle API co volat. */
define('BFF_URL', 'https://backend.mojedomena.cz');

/* ---- odkud smí přijít IP klienta ---------------------------------
   BFF posílá při přihlášení IP klienta (client_ip), aby škrtič počítal
   pokusy podle klienta, ne podle BFF. Věří se jí jen od BFF. Běží-li BFF
   na stejném stroji, pozná se sám (loopback, vlastní adresa serveru)
   a tohle zůstane zakomentované. Jinak sem patří adresa, ze které BFF
   volá — ne jeho jméno, ale IP, kterou tu uvidíš v REMOTE_ADDR. */
// define('BFF_IPS', '10.0.0.5');

/* ---- příznak nebezpečné instalace ---------------------------------
   Nastaví instalák, když zjistí, že vrstvy nejsou oddělené. Aplikace
   pak svítí červeným odznakem. Nevypínej to ručně — tím ten problém
   nezmizí, jen ta informace o něm. */
define('UNSAFE_DEMO', true);

/* Na produkci false. S true prozradí chybová hláška cesty na disk. */
define('FW_DEBUG', true);
