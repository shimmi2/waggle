<?php
/* =====================================================================
 *  api_books.inc — knihy a žánry
 * ===================================================================== */

/* Kolik kusů je právě volných.
 *
 *  Nedrží se to ve sloupci schválně: dvě čísla o téže věci se při první
 *  chybě rozejdou a už se nikdy nesrovnají. Počítá se to z deníku —
 *  půjčení, na které neukazuje žádné vrácení.
 *
 *  Jako KORELOVANÝ podotaz nad už omezeným výsledkem, ne jako odvozená
 *  tabulka přes celý deník. Při LIMIT 100 je to sto vyhledání v indexu
 *  re_rent_id; odvozená tabulka by projela všech dvě stě osmdesát tisíc
 *  řádků i kvůli jedné stránce.
 */
const SQL_VOLNYCH = "(b.bo_count - (
        SELECT COUNT(*) FROM rentals r
        WHERE r.re_book = b.bo_id AND r.re_returned = 0 AND r.re_operation = 'rent'))";

function ep_books_list(): void {
    /* 1. vstupy */
    $hledat  = in_str('q', 64);
    $zanr    = in_int('genre');
    $jen_vol = in_str('available', 1) === '1';
    $limit   = max(1, min(500, in_int('limit') ?: 100));
    $offset  = max(0, in_int('offset'));
    $razeni  = in_str('order', 32);

    /* 2. sémantika — řazení se NEBERE z požadavku do SQL. Jen se
          přeloží ze seznamu, který známe; cokoli jiného je výchozí. */
    $razeni = [
        'name'   => 'b.bo_name',        'name_desc'   => 'b.bo_name DESC',
        'author' => 'b.bo_author',      'author_desc' => 'b.bo_author DESC',
        'year'   => 'b.bo_year DESC',   'price'       => 'b.bo_price DESC',
    ][$razeni] ?? 'b.bo_name';

    /* 3. sezení — knihy vidí každý přihlášený, i čtenář bez oprávnění */
    session_require();

    /* 4. práce */
    $w = []; $p = [];
    if ($hledat !== '') {
        $w[] = '(b.bo_name LIKE ? OR b.bo_author LIKE ?)';
        $p[] = '%' . $hledat . '%'; $p[] = '%' . $hledat . '%';
    }
    if ($zanr > 0) { $w[] = 'b.bo_genre = ?'; $p[] = $zanr; }
    $where = $w ? 'WHERE ' . implode(' AND ', $w) : '';
    $having = $jen_vol ? 'HAVING bo_available > 0' : '';

    $rows = q_all("SELECT b.bo_id, b.bo_name, b.bo_author, b.bo_year, b.bo_count,
                          b.bo_price, b.bo_genre, g.ge_name,
                          " . SQL_VOLNYCH . " AS bo_available
                   FROM books b LEFT JOIN genres g ON b.bo_genre = g.ge_id
                   $where $having
                   ORDER BY $razeni LIMIT $limit OFFSET $offset", $p);

    /* Celkový počet pro stránkování. Bez dostupnosti, aby to nebylo
       dvakrát tak drahé kvůli číslu pod tabulkou. */
    $celkem = (int)q_val("SELECT COUNT(*) FROM books b $where", $p);

    api_out(['books' => $rows, 'total' => $celkem, 'limit' => $limit, 'offset' => $offset]);
}

function ep_book_detail(): void {
    $id = in_int('bo_id');
    if ($id < 1) api_error(400, 'Chybí id knihy');
    session_require();

    $b = q_one("SELECT b.*, g.ge_name, " . SQL_VOLNYCH . " AS bo_available
                FROM books b LEFT JOIN genres g ON b.bo_genre = g.ge_id
                WHERE b.bo_id = ?", [$id]);
    if (!$b) api_error(404, 'Taková kniha u nás není');

    api_out(['book' => $b]);
}

/* ---------------------------------------------------------------------
 *  Uložení knihy — zakládá i mění. Jeden endpoint, protože validace je
 *  pro obě věci stejná a dvě kopie validace se vždycky rozejdou.
 *
 *  Záměr se ale NEODVOZUJE z toho, jestli přišlo id. Kdyby se to dělalo
 *  tak, znamenalo by ztracené bo_id „založ novou knihu" — server by
 *  vyrobil duplikát a odpověděl ok. Taková chyba se nehlásí a najde se
 *  za půl roku v katalogu.
 *
 *  Proto říká klient výslovně, co chce, a server zkontroluje, že si to
 *  neodporuje s daty. Že op posílá nedůvěryhodná strana, nevadí: op nic
 *  nepovoluje, jen se musí shodnout. Podvržené op skončí na 400.
 * ------------------------------------------------------------------- */
function ep_book_save(): void {
    /* 1. vstupy */
    $op    = in_str('op', 16);
    $id    = in_int('bo_id');
    $name  = in_str('bo_name', 255);
    $autor = in_str('bo_author', 128);
    $zanr  = in_int('bo_genre');
    $rok   = in_int('bo_year');
    $kusu  = in_int('bo_count');
    $cena  = in_float('bo_price');
    $popis = in_str('bo_description', 4000);

    /* 2. sémantika */
    if ($op !== 'insert' && $op !== 'update')
                                        api_error(400, 'Neznámá operace', "op='$op'");
    if ($op === 'update' && $id < 1)    api_error(400, 'Nevím, kterou knihu mám změnit');
    if ($op === 'insert' && $id !== 0)  api_error(400, 'Nová kniha nemůže mít id', "insert s bo_id=$id");
    if ($name === '')                   api_error(400, 'Název knihy je povinný');
    if ($kusu < 0 || $kusu > 9999)      api_error(400, 'Počet kusů musí být 0 až 9999');
    if ($cena < 0)                      api_error(400, 'Cena nemůže být záporná');
    if ($rok !== 0 && ($rok < 1 || $rok > (int)date('Y') + 1))
                                        api_error(400, 'Rok vydání je mimo rozsah');

    /* 3. sezení, 4. oprávnění */
    $me = session_require();
    acl_require($me, 'manage_books');

    if ($zanr > 0 && !q_val("SELECT ge_id FROM genres WHERE ge_id=?", [$zanr]))
        api_error(400, 'Takový žánr neexistuje');

    /* 5. práce */
    if ($op === 'update') {
        if (!q_val("SELECT bo_id FROM books WHERE bo_id=?", [$id])) api_error(404, 'Taková kniha u nás není');
        /* Počet kusů nesmí klesnout pod to, co je právě půjčené —
           jinak by se dostupnost stala zápornou a nikdo by nevěděl proč. */
        $venku = (int)q_val("SELECT COUNT(*) FROM rentals
                             WHERE re_book=? AND re_returned=0 AND re_operation='rent'", [$id]);
        if ($kusu < $venku)
            api_error(400, "Nelze snížit na $kusu kusů, právě je půjčeno $venku.");
        q("UPDATE books SET bo_name=?, bo_author=?, bo_genre=?, bo_year=?, bo_count=?,
                            bo_price=?, bo_description=? WHERE bo_id=? LIMIT 1",
          [$name, $autor, $zanr, $rok, $kusu, $cena, $popis, $id]);
    } else {
        q("INSERT INTO books (bo_name, bo_author, bo_genre, bo_year, bo_count, bo_price, bo_description)
           VALUES (?,?,?,?,?,?,?)", [$name, $autor, $zanr, $rok, $kusu, $cena, $popis]);
        $id = (int)db()->lastInsertId();
    }
    api_out(['ok' => true, 'bo_id' => $id]);
}

function ep_genres_list(): void {
    session_require();
    api_out(['genres' => q_all("SELECT g.ge_id, g.ge_name, g.ge_description,
                                       (SELECT COUNT(*) FROM books WHERE bo_genre=g.ge_id) AS ge_books
                                FROM genres g ORDER BY g.ge_name")]);
}

function ep_genre_save(): void {
    /* 1. vstupy */
    $op    = in_str('op', 16);
    $id    = in_int('ge_id');
    $name  = in_str('ge_name', 64);
    $popis = in_str('ge_description', 255);

    /* 2. sémantika — stejná úmluva jako u ep_book_save() */
    if ($op !== 'insert' && $op !== 'update')
                                       api_error(400, 'Neznámá operace', "op='$op'");
    if ($op === 'update' && $id < 1)   api_error(400, 'Nevím, který žánr mám změnit');
    if ($op === 'insert' && $id !== 0) api_error(400, 'Nový žánr nemůže mít id', "insert s ge_id=$id");
    if ($name === '') api_error(400, 'Název žánru je povinný');

    $me = session_require();
    acl_require($me, 'manage_books');

    $jiny = q_val("SELECT ge_id FROM genres WHERE ge_name=? AND ge_id<>?", [$name, $id]);
    if ($jiny) api_error(400, 'Žánr s tímhle názvem už existuje');

    if ($op === 'update') {
        if (!q_val("SELECT ge_id FROM genres WHERE ge_id=?", [$id])) api_error(404, 'Takový žánr tu není');
        q("UPDATE genres SET ge_name=?, ge_description=? WHERE ge_id=? LIMIT 1", [$name, $popis, $id]);
    } else {
        q("INSERT INTO genres (ge_name, ge_description) VALUES (?,?)", [$name, $popis]);
        $id = (int)db()->lastInsertId();
    }
    api_out(['ok' => true, 'ge_id' => $id]);
}
