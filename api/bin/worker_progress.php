<?php
/* worker_progress.php — běží z CLI na pozadí, publikuje procenta do nchanu.
   Spouští ho stream_spawn(); jediný argument je cesta k souboru se vstupem,
   který si worker hned po přečtení smaže. */

if (PHP_SAPI !== 'cli') { http_response_code(403); exit; }

require __DIR__ . '/../config.inc';
require __DIR__ . '/../../fw.inc';
require __DIR__ . '/../inc/app.inc';
require __DIR__ . '/../inc/stream.inc';

$file  = $argv[1] ?? '';
$in    = is_file($file) ? json_decode((string)file_get_contents($file), true) : null;
@unlink($file);

$token = is_array($in) ? (string)($in['token'] ?? '') : '';
if ($token === '') die('token je null');

for ($i = 0; $i <= 100; $i++) {
    if (!fw_publish(STREAM_PUB_URL, $token, progress_cmds($i))) die('selhal publish');
    usleep(100000);
}
fw_publish(STREAM_PUB_URL, $token, progress_done('#run_push', 'pushem'));
