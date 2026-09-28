<?php
/* =====================================================================
 *  BFF — konfigurace
 *
 *  Instalák z tohohle udělá config.inc.
 *
 *  VŠIMNI SI, CO TU NENÍ: přístup k databázi ani k Redisu. BFF na data
 *  nesahá, takže je nepotřebuje znát — a co nezná, to nemůže vyzradit.
 *  Kdyby to tu bylo „pro případ, že by se to hodilo", je celé rozdělení
 *  vrstev jen na ozdobu.
 * ===================================================================== */

/* ---- kam pro data --------------------------------------------------- */
define('API_URL', 'https://api.mojedomena.cz');

/* ---- kdo sem smí ----------------------------------------------------
   Adresa frontendu, kvůli CORS. Jedna konkrétní, nikdy '*'. */
define('FRONTEND_URL', 'https://www.mojedomena.cz');

/* ---- krátká paměť na to, kdo je přihlášen --------------------------
   BFF si na chvíli pamatuje id, jméno a oprávnění, aby se backendu
   neptal při každém kliknutí. Ušetří to jedno HTTP volání na klik.
   Cenou je, že po změně oprávnění může uživatel do minuty vidět
   položku menu, na kterou už nemá právo — klikne, dostane 403, a jak
   paměť vyprší, menu se spraví. DATA ohrožená nejsou nikdy, protože
   oprávnění si backend ověřuje sám.

   PATŘÍ MIMO DOCROOT. Leží v tom id, jméno a oprávnění přihlášených.
   Ramdisk je ideální — je to rychlé a po restartu stroje tam nemá co
   zůstat. */
define('BFF_CACHE_DIR', '/dev/shm/knihovna-sessions');
define('BFF_CACHE_TTL', 60);

/* ---- push (volitelné) ----------------------------------------------
   Prázdné = bez pushe, a aplikace to unese. Kolečko „pracuji" ukáže
   prohlížeč sám přes data-busy; jen u statistiky nebudou procenta.
   Když je nchan nastavený a neodpovídá, zapíše se to do logu.

   Vyžaduje nginx s modulem nchan — viz nginx-nchan.conf.example
   u frameworku. Na běžném hostingu to nebude, a to je v pořádku. */
define('NCHAN_PUB', '');       // 'http://127.0.0.1:8081/nchan/pub'
define('NCHAN_SUB', '');       // 'https://www.mojedomena.cz:8443/nchan/sub'

/* ---- příznak nebezpečné instalace ---------------------------------- */
define('UNSAFE_DEMO', true);

/* Na produkci false. */
define('FW_DEBUG', true);
