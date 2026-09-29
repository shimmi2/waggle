<?php
/* =====================================================================
 *  Backend API knihovny — jediný vstupní bod
 *
 *  Vrací ČISTÝ JSON. O Waggle ani o HTML tady nikdo neví; ten protokol
 *  je věcí BFF. Díky tomu se dá tahle vrstva přepsat do jiného jazyka
 *  a napojit na ni cokoli — mobilní aplikaci, cizí systém, cron.
 *
 *  Jeden switch, jeden endpoint na case, tělo v modulu. Switch je jen
 *  rozcestník; bezpečnostní šablonu drží ta funkce v modulu, protože
 *  jedině ona ví, jaké oprávnění potřebuje.
 *
 *  SEZENÍ SI OVĚŘUJE KAŽDÝ ENDPOINT SÁM (kromě do_login). Je to ta
 *  jediná povolená výjimka z pořadí — kdyby se to dělalo jen tady
 *  nahoře, nešlo by mít veřejný endpoint bez přepisování rozcestníku,
 *  a hlavně by se ta kontrola časem stala neviditelnou.
 * ===================================================================== */

require __DIR__ . '/config.inc';        // musí být první
require __DIR__ . '/inc/boot.inc';
require __DIR__ . '/inc/api_session.inc';
require __DIR__ . '/inc/api_throttle.inc';
require __DIR__ . '/inc/api_users.inc';
require __DIR__ . '/inc/api_books.inc';
require __DIR__ . '/inc/api_rentals.inc';

/* CORS: BFF běží na jiném jménu, takže prohlížeč pošle preflight.
   Pouští se JEN adresa BFF z konfigurace, ne '*' — kdokoli jiný nemá
   co tohle API volat z prohlížeče. */
$origin = in_header('Origin');
if ($origin !== '' && $origin === rtrim(BFF_URL, '/')) {
    header('Access-Control-Allow-Origin: ' . $origin);
    header('Access-Control-Allow-Headers: X-App-Session, Content-Type');
    header('Vary: Origin');
}
if (($_SERVER['REQUEST_METHOD'] ?? '') === 'OPTIONS') { http_response_code(204); exit; }

/* Jméno endpointu. is_word() proto, že se z něj skládá jméno funkce —
   bez té kontroly by šlo zavolat cokoli, co je v paměti. */
$fn = in_str('fn', 64);
if (!is_word($fn)) api_error(400, 'Neplatné jméno operace');

switch ($fn) {

/* ---- veřejné ---------------------------------------------------- */
case 'ping':           api_out(['pong' => true, 'unsafe_demo' => (bool)UNSAFE_DEMO]);
case 'do_login':       ep_do_login();       break;

/* ---- sezení ----------------------------------------------------- */
case 'session_check':  ep_session_check();  break;
case 'do_logout':      ep_do_logout();      break;

/* ---- knihy a žánry ---------------------------------------------- */
case 'books_list':     ep_books_list();     break;
case 'book_detail':    ep_book_detail();    break;
case 'book_save':      ep_book_save();      break;
case 'genres_list':    ep_genres_list();    break;
case 'genre_save':     ep_genre_save();     break;

/* ---- výpůjčky --------------------------------------------------- */
case 'rentals_list':   ep_rentals_list();   break;
case 'rental_rent':    ep_rental_rent();    break;
case 'rental_return':  ep_rental_return();  break;
case 'statistics':     ep_statistics();     break;

/* ---- uživatelé -------------------------------------------------- */
case 'readers_list':   ep_readers_list();   break;
case 'users_list':     ep_users_list();     break;
case 'user_save':      ep_user_save();      break;

default:
    api_error(404, 'Taková operace neexistuje');
}

/* Endpoint, který nic neodpověděl, je chyba v kódu, ne prázdná
   odpověď. Bez tohohle by se to projevilo jako prázdné tělo se
   statusem 200 a hledalo by se to dlouho. */
api_error(500, 'Vnitřní chyba serveru', "endpoint $fn neodpověděl");
