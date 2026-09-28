/* Konfigurace frontendu. Instalák z tohohle udělá config.js.
 *
 * Frontend je statický, takže nemá PHP konfiguraci — a je to tak dobře:
 * nevidí databázi ani tajné klíče, protože o nich nemá jak vědět. Jediné,
 * co potřebuje, je adresa BFF.
 *
 * Na produkci tři různá jména. Podadresáře jedné domény jsou jen na
 * vyzkoušení; backendové API je pak dosažitelné z internetu. */
window.LIB_BFF_URL = 'https://backend.mojedomena.cz/';
