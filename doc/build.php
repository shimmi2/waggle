<?php
/* build.php — vygeneruje doc/index.html z docs/*.md a README.md.
   Spuštění:  php doc/build.php
   Podporuje podmnožinu Markdownu, kterou dokumentace skutečně používá:
   nadpisy, odstavce, seznamy, tabulky, bloky kódu, inline kód,
   tučné, kurzívu a odkazy. Žádná knihovna. */

if (PHP_SAPI !== 'cli') { http_response_code(404); exit; }

$root = dirname(__DIR__);
$files = glob("$root/docs/*.md");
sort($files);

function esc_h(string $s): string {
    return htmlspecialchars($s, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8');
}

/* inline: `kód`, **tučně**, *kurzíva*, [text](odkaz) */
function inline(string $t): string {
    /* Pozor: procházíme po bajtech, ale escapovat se musí až celý úsek.
       htmlspecialchars() nad jedním bajtem vícebajtového znaku ho
       s ENT_SUBSTITUTE nahradí za U+FFFD a rozsype diakritiku. */
    $out = ''; $plain = ''; $i = 0; $n = strlen($t);
    $flush = function () use (&$out, &$plain) { $out .= esc_h($plain); $plain = ''; };
    while ($i < $n) {
        if ($t[$i] === '`') {                          // kód má přednost
            $e = strpos($t, '`', $i + 1);
            if ($e !== false) {
                $flush();
                $out .= '<code>' . esc_h(substr($t, $i + 1, $e - $i - 1)) . '</code>';
                $i = $e + 1; continue;
            }
        }
        if (substr($t, $i, 2) === '**') {
            $e = strpos($t, '**', $i + 2);
            if ($e !== false) {
                $flush();
                $out .= '<strong>' . inline(substr($t, $i + 2, $e - $i - 2)) . '</strong>';
                $i = $e + 2; continue;
            }
        }
        if ($t[$i] === '*' && $i + 1 < $n && $t[$i + 1] !== ' ') {
            $e = strpos($t, '*', $i + 1);
            if ($e !== false) {
                $flush();
                $out .= '<em>' . inline(substr($t, $i + 1, $e - $i - 1)) . '</em>';
                $i = $e + 1; continue;
            }
        }
        if ($t[$i] === '[') {
            $c = strpos($t, ']', $i);
            if ($c !== false && ($t[$c + 1] ?? '') === '(') {
                $p = strpos($t, ')', $c);
                if ($p !== false) {
                    $label = substr($t, $i + 1, $c - $i - 1);
                    $href  = substr($t, $c + 2, $p - $c - 2);
                    /* odkaz na jinou kapitolu -> kotva na téže stránce */
                    if (preg_match('/^(\d\d)-[a-z-]+\.md$/', $href, $m)) $href = '#k' . $m[1];
                    $flush();
                    $out .= '<a href="' . esc_h($href) . '">' . inline($label) . '</a>';
                    $i = $p + 1; continue;
                }
            }
        }
        $plain .= $t[$i]; $i++;
    }
    $flush();
    return $out;
}

function table_row(string $line, string $cell): string {
    $cells = array_map('trim', explode('|', trim($line, "| \t")));
    $h = '<tr>';
    foreach ($cells as $c) $h .= "<$cell>" . inline($c) . "</$cell>";
    return $h . "</tr>\n";
}

function render_md(string $md, string $anchor): string {
    $lines = explode("\n", $md);
    $out = ''; $i = 0; $n = count($lines); $first_h1 = true;

    while ($i < $n) {
        $l = $lines[$i];

        if (substr($l, 0, 3) === '```') {              // blok kódu
            $i++; $buf = [];
            while ($i < $n && substr($lines[$i], 0, 3) !== '```') $buf[] = $lines[$i++];
            $i++;
            $out .= '<pre><code>' . esc_h(implode("\n", $buf)) . "</code></pre>\n";
            continue;
        }
        if (preg_match('/^(#{1,4})\s+(.*)$/', $l, $m)) {
            $lvl = strlen($m[1]); $txt = inline($m[2]);
            if ($lvl === 1 && $first_h1) {
                $first_h1 = false;
                $out .= "<h2 id=\"$anchor\">$txt</h2>\n";
            } else {
                $out .= '<h' . min($lvl + 1, 5) . ">$txt</h" . min($lvl + 1, 5) . ">\n";
            }
            $i++; continue;
        }
        if (strpos($l, '|') !== false && isset($lines[$i + 1])
            && preg_match('/^\s*\|?[\s:|-]+\|[\s:|-]*$/', $lines[$i + 1])) {
            $head = table_row($l, 'th'); $i += 2; $body = '';
            while ($i < $n && strpos($lines[$i], '|') !== false && trim($lines[$i]) !== '')
                $body .= table_row($lines[$i++], 'td');
            $out .= "<div class=\"tw\"><table><thead>$head</thead><tbody>$body</tbody></table></div>\n";
            continue;
        }
        if (preg_match('/^\s*[*-]\s+(.*)$/', $l)) {
            $out .= "<ul>\n";
            while ($i < $n && preg_match('/^\s*[*-]\s+(.*)$/', $lines[$i], $m)) {
                $item = $m[1]; $i++;
                while ($i < $n && preg_match('/^\s{2,}\S/', $lines[$i])
                       && !preg_match('/^\s*[*-]\s/', $lines[$i])) $item .= ' ' . trim($lines[$i++]);
                $out .= '  <li>' . inline($item) . "</li>\n";
            }
            $out .= "</ul>\n"; continue;
        }
        if (preg_match('/^\d+\.\s+(.*)$/', $l)) {
            $out .= "<ol>\n";
            while ($i < $n && preg_match('/^\d+\.\s+(.*)$/', $lines[$i], $m)) {
                $item = $m[1]; $i++;
                while ($i < $n && preg_match('/^\s{3,}\S/', $lines[$i])) $item .= ' ' . trim($lines[$i++]);
                $out .= '  <li>' . inline($item) . "</li>\n";
            }
            $out .= "</ol>\n"; continue;
        }
        if (substr(ltrim($l), 0, 2) === '> ') {
            $buf = [];
            while ($i < $n && substr(ltrim($lines[$i]), 0, 1) === '>')
                $buf[] = ltrim(ltrim($lines[$i++]), '> ');
            $out .= '<blockquote>' . inline(implode(' ', $buf)) . "</blockquote>\n";
            continue;
        }
        if (trim($l) === '') { $i++; continue; }

        $buf = [];                                     // odstavec
        while ($i < $n && trim($lines[$i]) !== ''
               && !preg_match('/^(#{1,4}\s|```|\s*[*-]\s|\d+\.\s|>\s)/', $lines[$i])
               && !(strpos($lines[$i], '|') !== false && isset($lines[$i + 1])
                    && preg_match('/^\s*\|?[\s:|-]+\|[\s:|-]*$/', $lines[$i + 1])))
            $buf[] = $lines[$i++];
        $out .= '<p>' . inline(implode(' ', $buf)) . "</p>\n";
    }
    return $out;
}

/* ---- sestavení stránky ---- */
$nav = ''; $body = '';
foreach ($files as $f) {
    $base = basename($f, '.md');
    if (!preg_match('/^(\d\d)-(.*)$/', $base, $m)) continue;
    $md = file_get_contents($f);
    preg_match('/^#\s+(.*)$/m', $md, $t);
    $title = trim(preg_replace('/^\d+\s*—\s*/u', '', $t[1] ?? $base));
    $nav  .= '    <li><a href="#k' . $m[1] . '"><span class="n">' . $m[1] . '</span> '
           . esc_h($title) . "</a></li>\n";
    $body .= '<section id="k' . $m[1] . "\">\n" . render_md($md, 'k' . $m[1]) . "</section>\n";
}

$tpl = file_get_contents(__DIR__ . '/template.html');
$html = str_replace(['{{NAV}}', '{{BODY}}', '{{DATE}}'],
                    [$nav, $body, date('j. n. Y')], $tpl);
file_put_contents(__DIR__ . '/index.html', $html);
printf("doc/index.html: %d kapitol, %d kB\n", count($files), round(strlen($html) / 1024));
