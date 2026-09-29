/* Konfigurace frontendu. Instalák z tohohle udělá config.js.
 *
 * Frontend je statický, takže nemá PHP konfiguraci — a je to tak dobře:
 * nevidí databázi ani tajné klíče, protože o nich nemá jak vědět. Jediné,
 * co potřebuje, je adresa BFF.
 *
 * Na produkci tři různá jména. Podadresáře jedné domény jsou jen na
 * vyzkoušení; backendové API je pak dosažitelné z internetu. */
window.LIB_BFF_URL = 'https://backend.mojedomena.cz/';

/* ---------------------------------------------------------------------
 *  Kdo smí poslat který příkaz
 *
 *  Dávka dorazí čtyřmi cestami a každá je jinak důvěryhodná:
 *
 *    response   odpověď na náš požadavek   — jen náš server
 *    local      naše vlastní Fw.broadcast  — jen naše stránka
 *    push       nchan                      — každý, kdo zná id kanálu
 *    broadcast  BroadcastChannel           — KAŽDÁ stránka na originu
 *
 *  Výchozí sada je ve frameworku a je bezpečná: pushem projde jen
 *  kreslení, které neumí spustit kód, broadcastem skoro nic. Tady se dá
 *  rozšířit — utažení je rozhodnutí toho, kdo nasazuje, ne knihovny.
 *
 *  Piš to jako ROZDÍL proti výchozímu, ne jako celý seznam: kdybys
 *  vyjmenoval všechny příkazy, nové vydání s novým příkazem by ho mělo
 *  tiše mimo seznam.
 *
 *  Pozor na html: innerHTML sice nespustí <script>, ale
 *  <img src=x onerror=…> ano. html přes push nebo broadcast je tedy
 *  spuštění kódu, ne kreslení. Kdo ho pushem potřebuje, ať dávku
 *  PODEPÍŠE — fw_publish($url, $token, $cmds, 2.0, $klic) — a nechá to
 *  na accessSigned.
 *
 *  Zakomentovaný příklad: povolit mezi okny i html.
 * ------------------------------------------------------------------- */
// window.LIB_FW_ACCESS = { broadcast: ['value', 'text', 'class', 'notify', 'html'] };
