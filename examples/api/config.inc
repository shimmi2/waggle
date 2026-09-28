<?php
/* =====================================================================
 *  config.inc — veškeré nastavení projektu na jednom místě.
 *  Načítá se jako PRVNÍ, ještě před fw.inc, aby mohl přebít jeho výchozí
 *  hodnoty (fw.inc používá defined() || define()).
 *
 *  NA PRODUKCI: FW_DEBUG na false a tajné klíče mimo webroot.
 * ===================================================================== */

/* ---- framework ---------------------------------------------------- */
define('FW_DEBUG', true);          // chybová hláška prozradí cesty na disku

/* ---- přihlášení --------------------------------------------------- */
define('AUTH_SECRET', 'demo-secret-zmen-me-v-produkci');
define('AUTH_TTL',    3600);

/* ---- push kanály -------------------------------------------------- */
define('STREAM_SUB_PORT', 8443);                              // veřejně, TLS
define('STREAM_PUB_URL',  'http://127.0.0.1:8081/nchan/pub'); // jen localhost
define('STREAM_TTL',      3600);
define('STREAM_PHP',      '/usr/bin/php8.4');                 // web jede na 7.4, worker ne
define('STREAM_STORE',    __DIR__ . '/data/tokens.json');
