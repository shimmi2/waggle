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
 *  BEZ SHEBANGU schválně. PHP ho odstraňuje jen v CLI; pod webserverem
 *  ho pošle jako text, tedy bajt před hlavičkami. Ochranou je kontrola
 *  PHP_SAPI níž, ne shebang. Volej s interpretem.
 *
 *  Použití:
 *
 *    php async_sender.php --token-file=/run/tok --pub=URL < davka.json
 *    FW_CHANNEL_TOKEN=… php async_sender.php --pub=URL < davka.json
 *    php async_sender.php --token=TOKEN --pub=URL < davka.json   (vidět v ps!)
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
                 'busy-done:', 'busy-off', 'notify:', 'kind:', 'help', 'fw:',
                 'token-file:']);

/* Token se bere ze tří míst, v tomhle pořadí. --token je nejpohodlnější
   a nejhorší: ARGUMENTY PROCESU VIDÍ V `ps` KAŽDÝ UŽIVATEL STROJE. Je to
   tentýž důvod, kvůli kterému workery dostávají parametry dočasným
   souborem s právy 0600. Na vlastním stroji to obvykle nevadí, na
   sdíleném ano — a tam použij --token-file nebo proměnnou prostředí. */
$token = '';
if (isset($o['token-file']) && is_file($o['token-file'])) {
    $token = trim((string)file_get_contents($o['token-file']));
} elseif (getenv('FW_CHANNEL_TOKEN') !== false) {
    $token = trim((string)getenv('FW_CHANNEL_TOKEN'));
} elseif (isset($o['token'])) {
    $token = (string)$o['token'];
}

if (isset($o['help']) || $token === '') {
    fwrite(STDERR, preg_replace('/^.*?\/\* |\*\/.*$/s', '', file_get_contents(__FILE__)) . "\n");
    exit(isset($o['help']) ? 0 : 1);
}
$sel   = (string)($o['sel'] ?? 'main');

if (isset($o['config']) && is_file($o['config'])) require $o['config'];
/* fw.inc se HLEDÁ, ne napevno: tenhle soubor se má zkopírovat mimo
   docroot, takže relativní cesta by po přesunu přestala platit. Pořadí:
   --fw, proměnná prostředí, pak pár pater nahoru. */
$fw = (string)($o['fw'] ?? getenv('FW_INC') ?: '');
if ($fw === '') {
    for ($d = __DIR__, $i = 0; $i < 6; $i++, $d = dirname($d))
        if (is_file("$d/fw.inc")) { $fw = "$d/fw.inc"; break; }
}
if ($fw === '' || !is_file($fw)) {
    fwrite(STDERR, "nenašel jsem fw.inc — zadej --fw=/cesta/fw.inc nebo FW_INC\n"); exit(1);
}
require $fw;

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
