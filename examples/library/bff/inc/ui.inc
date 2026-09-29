<?php
/* =====================================================================
 *  ui.inc — pomůcky pro skládání fragmentů
 *
 *  Tady se HTML vyrábí, takže tady se taky escapuje. Platí pravidlo
 *  z kapitoly 10: na vstupu validuj a typuj, na VÝSTUPU escapuj podle
 *  cíle. Do HTML nejde nic bez esc().
 * ===================================================================== */

/* esc() bydlí v io.inc — escapuje se v obou vrstvách stejně a dvě
   kopie téhož by se dřív nebo později rozešly. */

/* Fragment ze souboru. Stejná myšlenka jako render() ve frameworku:
   proměnné se předají polem, ne globálkami, aby bylo v šabloně vidět,
   s čím se pracuje. */
function frag(string $name, array $vars = []): string {
    $f = __DIR__ . '/../pages/' . $name . '.inc';
    if (!is_file($f)) return '<p class="err">Chybí fragment ' . esc($name) . '</p>';
    extract($vars, EXTR_SKIP);
    ob_start();
    require $f;
    return (string)ob_get_clean();
}

/* Peníze. Ne money_format() — ta z PHP 8 zmizela. */
function kc($v): string {
    return number_format((float)$v, 2, ',', ' ') . ' Kč';
}

/* Datum bez času, když ho nepotřebujeme. */
function den(?string $d): string {
    if (!$d || $d === '0000-00-00') return '—';
    $t = strtotime($d);
    return $t ? date('j. n. Y', $t) : esc($d);
}

/* Počet v češtině. Bez toho je všude „1 knih". */
function pocet(int $n, string $jedna, string $malo, string $mnoho): string {
    if ($n === 1) return "$n $jedna";
    if ($n >= 2 && $n <= 4) return "$n $malo";
    return "$n $mnoho";
}

/* Odznak. Stav se ukazuje odznakem, ne barvou celého řádku — obarvený
   řádek se v delší tabulce čte špatně a při dvou stavech vedle sebe
   nejde rozeznat, který je který. */
function badge(string $text, string $druh = 'neutral'): string {
    /* Sneat používá „label" varianty — tlumené pozadí, čitelný text.
       Mapa je tady, aby fragmenty psaly význam („danger"), ne třídu:
       až se šablona zase vymění, přepíše se jediné místo. */
    static $mapa = ['ok' => 'bg-label-success', 'warn' => 'bg-label-warning',
                    'danger' => 'bg-label-danger', 'neutral' => 'bg-label-secondary'];
    $tr = $mapa[$druh] ?? $mapa['neutral'];
    return '<span class="badge ' . $tr . '">' . esc($text) . '</span>';
}

/* Stránkování. Vrací prázdno, když je co zobrazit na jednu stránku. */
function pager(string $fn, int $total, int $limit, int $offset, array $params = []): string {
    if ($total <= $limit) return '';
    $stran = (int)ceil($total / $limit);
    $tato  = (int)floor($offset / $limit) + 1;
    $q = function (int $off) use ($fn, $params) {
        $p = $params + ['offset' => $off];
        return '#?function=' . rawurlencode($fn) . '&' . http_build_query($p);
    };
    $h = '<nav class="mt-3"><ul class="pagination pagination-sm mb-0 justify-content-center">';
    $h .= $tato > 1
        ? '<li class="page-item"><a class="page-link" href="' . esc($q($offset - $limit))
          . '" data-fw><i class="bx bx-chevron-left"></i></a></li>'
        : '<li class="page-item disabled"><span class="page-link"><i class="bx bx-chevron-left"></i></span></li>';
    $h .= '<li class="page-item active"><span class="page-link">' . $tato . ' / ' . $stran . '</span></li>';
    $h .= $tato < $stran
        ? '<li class="page-item"><a class="page-link" href="' . esc($q($offset + $limit))
          . '" data-fw><i class="bx bx-chevron-right"></i></a></li>'
        : '<li class="page-item disabled"><span class="page-link"><i class="bx bx-chevron-right"></i></span></li>';
    return $h . '</ul></nav>';
}
