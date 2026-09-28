<?php
/* app.inc — pomocné funkce PROJEKTU, ne frameworku.
   Framework záměrně neřeší šablony ani escapování do cílového kontextu. */

/* Escapování do HTML. Jediné správné místo je až tady, na výstupu. */
function esc(?string $s): string {
    return htmlspecialchars((string)$s, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8');
}

/* Vyrenderuje .inc soubor do řetězce. Proměnné dostane jako lokální. */
function render(string $file, array $vars = []): string {
    if (!is_file($file)) return '<p class="err">Chybí šablona ' . esc(basename($file)) . '</p>';
    extract($vars, EXTR_SKIP);
    ob_start();
    require $file;
    return (string)ob_get_clean();
}

/* Příkazy progress baru. Sdílí je streamovací endpoint i push worker,
   a jsou nezávislé na skinu — obě sady fragmentů používají stejná id. */

function progress_cmds(int $pct): array {
    return [
        ['op' => 'attr', 'sel' => '#pb', 'name' => 'style',         'value' => "width: $pct%"],
        ['op' => 'attr', 'sel' => '#pb', 'name' => 'aria-valuenow', 'value' => (string)$pct],
        ['op' => 'html', 'sel' => '#pb_pct', 'content' => $pct . ' %'],
    ];
}

function progress_done(string $btn, string $how): array {
    return [
        ['op' => 'class', 'sel' => '#pb', 'remove' => ['progress-bar-animated'], 'add' => ['bg-success']],
        ['op' => 'class', 'sel' => $btn, 'remove' => ['disabled']],
        ['op' => 'html',  'sel' => '#pb_note',
         'content' => 'Hotovo v ' . esc(date('H:i:s')) . ' — doručeno ' . esc($how)],
    ];
}
