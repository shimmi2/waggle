#!/usr/bin/php
<?php
/* async_sender.php — pošle dávku příkazů do prohlížeče mimo HTTP odpověď.
 *
 *  POZOR, ať to nepoužiješ na špatný problém. Apache drží streamovanou
 *  odpověď (mod_proxy_fcgi), ale push tou cestou vůbec nejde: fw_publish()
 *  je krátký POST na nchan na localhostu a s Apachem nemá nic společného.
 *  Uvnitř běžícího requestu tedy NENÍ potřeba spouštět nic externího —
 *  stačí zavolat fw_publish() rovnou:
 *
 *      fw_publish(STREAM_PUB_URL, $token, [fw_busy('main', 'Počítám…')]);
 *      $data = nejaky_dlouhy_dotaz();          // prohlížeč už kolečko točí
 *
 *  Tenhle skript je pro to, co PHP request udělat neumí:
 *
 *    - démon, cron nebo shellový skript, který chce něco napsat do
 *      otevřené stránky správce („zálohuji, 40 %")
 *    - práce, která má pokračovat potom, co odpověď skončila
 *    - jazyk, který není PHP (pošle JSON na stdin a je hotovo)
 *
 *  Použití:
 *
 *    php async_sender.php --token=TOKEN --pub=URL < davka.json
 *    echo '{"op":"notify","kind":"info","message":"Záloha hotova"}' \
 *      | php async_sender.php --token=TOKEN
 *    php async_sender.php --token=TOKEN --busy='Zálohuji…' --pct=40
 *    php async_sender.php --token=TOKEN --busy-done='Záloha hotova'
 *
 *  Na stdin se bere celá dávka {"v":1,"cmds":[…]}, holé pole příkazů
 *  i jediný příkaz. Token je ten, který dostal prohlížeč příkazem
 *  subscribe — je to klíč kanálu, ne přihlašovací údaj.
 *
 *  --pub lze vynechat, když je nastavená proměnná prostředí
 *  FW_STREAM_PUB_URL nebo když --config ukazuje na soubor s konstantou
 *  STREAM_PUB_URL.
 *
 *  Návratový kód: 0 odesláno, 1 chyba vstupu, 2 broker neodpověděl.
 *  Volající to má kontrolovat — tichý neúspěch je horší než hlasitý. */

if (PHP_SAPI !== 'cli') { http_response_code(403); exit('cli only'); }

$o = getopt('', ['token:', 'pub:', 'config:', 'sel:', 'busy:', 'pct:',
                 'busy-done:', 'busy-off', 'notify:', 'kind:', 'help']);

if (isset($o['help']) || !isset($o['token'])) {
    fwrite(STDERR, preg_replace('/^.*?\/\* |\*\/.*$/s', '', file_get_contents(__FILE__)) . "\n");
    exit(isset($o['help']) ? 0 : 1);
}
$token = (string)$o['token'];
$sel   = (string)($o['sel'] ?? 'main');

if (isset($o['config']) && is_file($o['config'])) require $o['config'];
require __DIR__ . '/../fw.inc';

$pub = (string)($o['pub'] ?? getenv('FW_STREAM_PUB_URL')
      ?: (defined('STREAM_PUB_URL') ? STREAM_PUB_URL : ''));
if ($pub === '') { fwrite(STDERR, "chybí --pub (ani FW_STREAM_PUB_URL, ani STREAM_PUB_URL)\n"); exit(1); }

/* Zkratky, ať se kvůli jednomu kolečku nemusí skládat JSON. */
$cmds = [];
if (isset($o['busy']))      $cmds[] = fw_busy($sel, (string)$o['busy'],
                                        isset($o['pct']) ? (int)$o['pct'] : null);
if (isset($o['busy-done'])) $cmds[] = fw_busy_done($sel, (string)$o['busy-done']);
if (isset($o['busy-off']))  $cmds[] = fw_busy_off($sel);
if (isset($o['notify']))    $cmds[] = ['op' => 'notify', 'kind' => (string)($o['kind'] ?? 'info'),
                                       'message' => (string)$o['notify']];

if (!$cmds) {
    $raw = stream_get_contents(STDIN);
    if (trim((string)$raw) === '') { fwrite(STDERR, "na stdin nic nepřišlo a nebyla zadána žádná zkratka\n"); exit(1); }
    $in = json_decode($raw, true);
    if (!is_array($in)) { fwrite(STDERR, "vstup není platný JSON\n"); exit(1); }
    if (isset($in['cmds']) && is_array($in['cmds'])) $cmds = $in['cmds'];   // celá dávka
    elseif (isset($in['op']))                        $cmds = [$in];         // jeden příkaz
    else                                             $cmds = $in;           // pole příkazů
}

foreach ($cmds as $c)
    if (!is_array($c) || !isset($c['op'])) { fwrite(STDERR, "příkaz bez 'op'\n"); exit(1); }

if (!fw_publish($pub, $token, $cmds)) {
    fwrite(STDERR, "broker neodpověděl: $pub\n");
    exit(2);
}
exit(0);
