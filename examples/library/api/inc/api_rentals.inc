<?php
/* =====================================================================
 *  api_rentals.inc — výpůjčky a statistika
 * ===================================================================== */

/* ---------------------------------------------------------------------
 *  Seznam výpůjček
 *
 *  KLÍČOVÉ MÍSTO CELÉHO PŘÍKLADU: kdo nemá see_all_rentals, vidí jen
 *  svoje — a to omezení dělá SERVER, ne klient. Kdyby se filtr na
 *  vlastní id posílal z prohlížeče, stačí ho v prohlížeči změnit.
 *  Proto se tady požadovaný filtr přepíše, ne zkontroluje.
 * ------------------------------------------------------------------- */
function ep_rentals_list(): void {
    /* 1. vstupy */
    $user    = in_int('user');
    $kniha   = in_int('book');
    $jen_ven = in_str('open', 1) === '1';
    $po_term = in_str('overdue', 1) === '1';
    $limit   = max(1, min(500, in_int('limit') ?: 100));
    $offset  = max(0, in_int('offset'));

    /* 2. sémantika, 3. sezení */
    $me = session_require();

    /* 4. oprávnění — ne odmítnutím, ale zúžením. Čtenář smí seznam
          vidět, jen ne cizí; tak mu ho zúžíme na jeho vlastní. */
    if (!acl_has($me, 'see_all_rentals')) $user = (int)$me['us_id'];

    /* 5. práce */
    $w = ["r.re_operation = 'rent'"]; $p = [];
    if ($user  > 0) { $w[] = 'r.re_user = ?';  $p[] = $user; }
    if ($kniha > 0) { $w[] = 'r.re_book = ?';  $p[] = $kniha; }
    if ($jen_ven || $po_term) $w[] = 'r.re_returned = 0';
    if ($po_term)             $w[] = 'r.re_allowed_return_date < CURDATE()';
    $where = 'WHERE ' . implode(' AND ', $w);

    $rows = q_all("SELECT r.re_id, r.re_user, r.re_book, r.re_date, r.re_price,
                          r.re_allowed_return_date, r.re_note,
                          u.us_name, b.bo_name, b.bo_author,
                          z.re_date AS returned_at,
                          (r.re_returned = 0 AND r.re_allowed_return_date < CURDATE()) AS is_overdue
                   FROM rentals r
                   LEFT JOIN rentals z ON z.re_rent_id = r.re_id AND r.re_returned = 1
                   LEFT JOIN users u   ON u.us_id = r.re_user
                   LEFT JOIN books b   ON b.bo_id = r.re_book
                   $where
                   ORDER BY r.re_date DESC LIMIT $limit OFFSET $offset", $p);

    $celkem = (int)q_val("SELECT COUNT(*) FROM rentals r $where", $p);

    /* Do horního rámu: co mi běží a kdy mám nejblíž vracet. Je to
       o uživateli, který se ptá, ne o filtru. */
    /* next_due je nejbližší BUDOUCÍ termín. Kdyby to bylo minimum přes
       všechno, ukáže se datum dávno po termínu — a „nejblíž vrátit:
       19. 2. 2017" je pro uživatele nesmysl. Prošlé se hlásí zvlášť
       jako overdue_count. */
    $moje = q_one("SELECT COUNT(*) AS open_count,
                          MIN(CASE WHEN r.re_allowed_return_date >= CURDATE()
                                   THEN r.re_allowed_return_date END) AS next_due,
                          SUM(r.re_allowed_return_date < CURDATE()) AS overdue_count
                   FROM rentals r
                   WHERE r.re_operation='rent' AND r.re_returned = 0 AND r.re_user = ?",
                  [(int)$me['us_id']]);

    api_out(['rentals' => $rows, 'total' => $celkem, 'mine' => $moje,
             'limit' => $limit, 'offset' => $offset,
             'scope' => acl_has($me, 'see_all_rentals') ? 'all' : 'own']);
}

/* ---------------------------------------------------------------------
 *  Půjčení
 * ------------------------------------------------------------------- */
function ep_rental_rent(): void {
    /* 1. vstupy */
    $kniha = in_int('book');
    $user  = in_int('user');
    $dnu   = in_int('days') ?: 30;
    $cena  = in_float('price');
    $note  = in_str('note', 255);

    /* 2. sémantika */
    if ($kniha < 1)              api_error(400, 'Chybí kniha');
    if ($user  < 1)              api_error(400, 'Chybí čtenář');
    if ($dnu < 1 || $dnu > 365)  api_error(400, 'Výpůjční doba musí být 1 až 365 dní');
    if ($cena < 0)               api_error(400, 'Cena nemůže být záporná');

    /* 3. sezení, 4. oprávnění */
    $me = session_require();
    acl_require($me, 'rent_books');

    /* 5. práce */
    if (!q_val("SELECT us_id FROM users WHERE us_id=?", [$user]))   api_error(404, 'Takový čtenář neexistuje');
    $b = q_one("SELECT bo_id, bo_count, bo_name FROM books WHERE bo_id=?", [$kniha]);
    if (!$b) api_error(404, 'Taková kniha u nás není');

    /* Dostupnost se kontroluje v transakci a řádek knihy se zamkne.
       Bez toho by dva zaměstnanci u dvou pultů půjčili poslední kus
       současně — a ten závod se v testech nikdy neprojeví. */
    db()->beginTransaction();
    try {
        q("SELECT bo_id FROM books WHERE bo_id=? FOR UPDATE", [$kniha]);
        $venku = (int)q_val("SELECT COUNT(*) FROM rentals
                             WHERE re_book=? AND re_returned=0 AND re_operation='rent'", [$kniha]);
        if ($venku >= (int)$b['bo_count']) {
            db()->rollBack();
            api_error(400, 'Všechny kusy téhle knihy jsou půjčené');
        }
        q("INSERT INTO rentals (re_user, re_book, re_operation, re_date, re_price, re_allowed_return_date, re_note)
           VALUES (?,?,'rent',NOW(),?,DATE_ADD(CURDATE(), INTERVAL ? DAY),?)",
          [$user, $kniha, $cena, $dnu, $note]);
        $id = (int)db()->lastInsertId();
        db()->commit();
    } catch (Throwable $e) {
        if (db()->inTransaction()) db()->rollBack();
        api_error(500, 'Výpůjčku se nepodařilo zapsat', $e->getMessage());
    }

    api_out(['ok' => true, 're_id' => $id]);
}

/* ---------------------------------------------------------------------
 *  Vrácení
 * ------------------------------------------------------------------- */
function ep_rental_return(): void {
    $rent = in_int('rental');
    $note = in_str('note', 255);
    if ($rent < 1) api_error(400, 'Chybí výpůjčka');

    $me = session_require();
    acl_require($me, 'rent_books');

    $r = q_one("SELECT re_id, re_user, re_book FROM rentals WHERE re_id=? AND re_operation='rent'", [$rent]);
    if (!$r) api_error(404, 'Taková výpůjčka neexistuje');

    /* Zápis vrácení a nastavení příznaku patří do JEDNÉ transakce. Kdyby
       se to rozdělilo, může mezi tím spadnout PHP a kniha zůstane navěky
       „půjčená", i když řádek o vrácení existuje. Příznak se navíc
       nastavuje podmíněně (re_returned=0), takže dvojí vrácení
       neprojde ani při závodu dvou pultů. */
    db()->beginTransaction();
    try {
        $n = q("UPDATE rentals SET re_returned=1
                WHERE re_id=? AND re_operation='rent' AND re_returned=0", [$rent])->rowCount();
        if ($n !== 1) { db()->rollBack(); api_error(400, 'Tahle výpůjčka už je vrácená'); }

        q("INSERT INTO rentals (re_user, re_book, re_operation, re_date, re_price, re_note, re_rent_id)
           VALUES (?,?,'return',NOW(),0,?,?)",
          [(int)$r['re_user'], (int)$r['re_book'], $note, $rent]);
        $id = (int)db()->lastInsertId();
        db()->commit();
    } catch (Throwable $e) {
        if (db()->inTransaction()) db()->rollBack();
        api_error(500, 'Vrácení se nepodařilo zapsat', $e->getMessage());
    }

    api_out(['ok' => true, 're_id' => $id]);
}

/* ---------------------------------------------------------------------
 *  Statistika — PO ČÁSTECH
 *
 *  Každá část je vlastní volání. Vypadá to jako práce navíc, ale je to
 *  ten důvod, proč jde ukázat postup: procenta hlásí BFF mezi voláními,
 *  takže backend nemusí vědět nic o prohlížeči ani o push kanálu.
 *  Kdyby ta smyčka byla tady, musel by backend publikovat do nchanu —
 *  a tím by se datová vrstva provázala s klientem.
 *
 *  Části jsou navíc samostatně cachovatelné a při rozšiřování se přidá
 *  jen další case.
 * ------------------------------------------------------------------- */
function ep_statistics(): void {
    $part = in_str('part', 32);

    $me = session_require();
    acl_require($me, 'see_statistics');

    switch ($part) {

    case 'monthly':
        api_out(['part' => $part, 'rows' => q_all(
            "SELECT YEAR(re_date) y, MONTH(re_date) m, COUNT(*) n, SUM(re_price) v
             FROM rentals WHERE re_operation='rent'
             GROUP BY y, m ORDER BY y DESC, m DESC LIMIT 60")]);

    case 'genres':
        api_out(['part' => $part, 'rows' => q_all(
            "SELECT g.ge_id, g.ge_name, COUNT(*) n, SUM(r.re_price) v
             FROM rentals r JOIN books b ON r.re_book=b.bo_id
                            JOIN genres g ON b.bo_genre=g.ge_id
             WHERE r.re_operation='rent' GROUP BY g.ge_id ORDER BY n DESC")]);

    case 'top_books':
        api_out(['part' => $part, 'rows' => q_all(
            "SELECT b.bo_id, b.bo_name, b.bo_author, COUNT(*) n
             FROM rentals r JOIN books b ON r.re_book=b.bo_id
             WHERE r.re_operation='rent' GROUP BY b.bo_id ORDER BY n DESC LIMIT 20")]);

    case 'top_readers':
        api_out(['part' => $part, 'rows' => q_all(
            "SELECT u.us_id, u.us_name, COUNT(*) n, SUM(r.re_price) v
             FROM rentals r JOIN users u ON r.re_user=u.us_id
             WHERE r.re_operation='rent' GROUP BY u.us_id ORDER BY n DESC LIMIT 20")]);

    case 'duration':
        /* Doba držení. Páruje se přes re_rent_id, tedy spojením přes
           primární klíč. Bez toho sloupce by se to dalo dohledat jen
           podle (kniha, uživatel) a pořadí dat — a takový dotaz nad
           čtvrt milionem řádků prohledává miliardu kombinací
           a nedoběhne. */
        api_out(['part' => $part, 'rows' => q_all(
            "SELECT YEAR(r.re_date) y, COUNT(*) n,
                    ROUND(AVG(DATEDIFF(z.re_date, r.re_date)),1) days_avg,
                    MAX(DATEDIFF(z.re_date, r.re_date)) days_max
             FROM rentals z JOIN rentals r ON r.re_id = z.re_rent_id
             WHERE z.re_operation='return' GROUP BY y ORDER BY y DESC LIMIT 20")]);

    default:
        api_error(400, 'Neznámá část statistiky');
    }
}
