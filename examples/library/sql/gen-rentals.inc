<?php
/* =====================================================================
 *  gen-rentals.inc — generátor ukázkových výpůjček
 *
 *  Je to FUNKCE, ne skript, a má to důvod. Webový instalák ji volá
 *  přímo se svým už otevřeným spojením. Původně si ji pouštěl přes
 *  exec(PHP_BINARY ...) a bylo to špatně dvakrát:
 *
 *    1. Pod PHP-FPM je PHP_BINARY /usr/sbin/php-fpm8.4, tedy správce
 *       procesů, ne interpret skriptů. Odpovědí byl výpis nápovědy
 *       php-fpm, což vypadá jako chyba generátoru, a není.
 *    2. exec() je na sdíleném hostingu běžně v disable_functions.
 *       Instalák, který ho potřebuje, tam prostě neproleze.
 *
 *  Data se nedají napsat do .sql souboru, protože musí respektovat
 *  skutečnost: knihovna nemůže mít půjčeno víc kusů, než kolik jich má,
 *  a čtenář nedrží nekonečně knih. Generátor proto drží stav a den po
 *  dni půjčuje a vrací.
 *
 *  Nepravidelnosti jsou schválně: někdo vrací pozdě, pár knih se nevrátí
 *  nikdy, v létě se půjčuje méně, populární žánry jdou na dračku. Bez
 *  toho je každý přehled vodorovná čára a statistika nemá co ukázat.
 * ===================================================================== */

/* Vážený náhodný výběr. Váhy nejsou v databázi — je to jen vlastnost
   generátoru, aby data nebyla plochá. */
function gen_vaz(array $pole) {
    $s = 0; foreach ($pole as $p) $s += $p['vaha'];
    $r = mt_rand(1, max(1, $s));
    foreach ($pole as $i => $p) { $r -= $p['vaha']; if ($r <= 0) return $i; }
    return array_key_last($pole);
}

/* Vrací pole se souhrnem, nebo vyhodí RuntimeException. */
function gen_rentals(PDO $db, int $dny = 730, int $nasobic = 1, int $ctenaru = 0): array {
    $dny     = max(30, $dny);
    $nasobic = max(1, $nasobic);
    $ctenaru = max(0, $ctenaru);

    if ($nasobic > 1) $db->exec("UPDATE books SET bo_count = bo_count * $nasobic");

    $knihy = [];
    foreach ($db->query("SELECT bo_id, bo_count, bo_genre FROM books") as $b) {
        $kusu = (int)$b['bo_count'];      /* už po vynásobení v databázi */
        $knihy[] = ['id' => (int)$b['bo_id'], 'kusu' => $kusu,
                    'volnych' => $kusu,
                    /* Klasika a dobrodružná jdou nejvíc, filosofie leží. */
                    'vaha' => [1=>9, 2=>3, 3=>4, 4=>10, 5=>8, 6=>6, 7=>2, 8=>7, 9=>3][(int)$b['bo_genre']] ?? 5];
    }
    /* Dogenerování čtenářů. Konta jsou nepřihlašitelná (prázdné heslo) —
       jsou to jen jména ve výpůjčkách, ne cesta dovnitř. */
    if ($ctenaru > 0) {
        $jmena = ['Jan','Petr','Josef','Martin','Tomáš','Jaroslav','Miroslav','Zdeněk','Pavel','Václav',
                  'Jiří','Michal','František','Milan','David','Lukáš','Ondřej','Marek','Filip','Karel',
                  'Marie','Jana','Eva','Hana','Anna','Lenka','Kateřina','Věra','Alena','Petra',
                  'Lucie','Jaroslava','Veronika','Tereza','Michaela','Martina','Zdeňka','Ivana','Helena','Klára'];
        $prijmeni = ['Novák','Svoboda','Novotný','Dvořák','Černý','Procházka','Kučera','Veselý','Horák','Němec',
                     'Marek','Pospíšil','Pokorný','Hájek','Jelínek','Král','Růžička','Beneš','Fiala','Sedláček',
                     'Doležal','Zeman','Kolář','Navrátil','Čermák','Vaněk','Urban','Blažek','Křížek','Šťastný'];
        $q = $db->prepare("INSERT INTO users (us_login, us_password, us_name, us_email, us_acl)
                           VALUES (?, '', ?, ?, '')");
        $db->beginTransaction();
        for ($i = 1; $i <= $ctenaru; $i++) {
            $j = $jmena[($i * 7) % count($jmena)];
            $p = $prijmeni[($i * 11) % count($prijmeni)];
            $zensky = str_contains('Marie Jana Eva Hana Anna Lenka Kateřina Věra Alena Petra Lucie Jaroslava Veronika Tereza Michaela Martina Zdeňka Ivana Helena Klára', $j);
            $pn = $zensky ? (str_ends_with($p, 'ý') ? substr($p, 0, -1) . 'á' : $p . 'ová') : $p;
            $q->execute(['ctenar' . $i, "$pn $j", 'ctenar' . $i . '@example.com']);
        }
        $db->commit();
    }

    /* Čtenáři. Obsluha si taky půjčuje, ale málo. */
    $ctenari = [];
    foreach ($db->query("SELECT us_id, us_acl FROM users") as $u)
        $ctenari[] = ['id' => (int)$u['us_id'],
                      'vaha' => $u['us_acl'] === '' ? 10 : 2,
                      /* Strop na počet současně držených knih. Knihovny ho
                         mají a bez něj data nesmyslně narostou: za deset let
                         drží jeden čtenář sedmdesát svazků a horní rám hlásí
                         „půjčeno: 69", což nikdo nikdy neviděl. */
                      'strop' => 8, 'drzi' => 0];
    if (!$knihy || !$ctenari)
        throw new RuntimeException('V databázi nejsou knihy nebo uživatelé — nejdřív se musí naimportovat 03-books.sql a 04-users.sql.');

    /* Filip Marek (a kdokoli 12.) půjčuje po pěti — ať je v datech někdo,
       kdo se vymyká, protože na takových se ladí filtry. */
    if (isset($ctenari[11])) { $ctenari[11]['vaha'] = 40; $ctenari[11]['strop'] = 20; }


    $vypujcene = [];     // otevřené výpůjčky
    $radky     = [];
    /* Id si generátor přiděluje sám, protože řádek vrácení musí znát id toho
       půjčení, které zavírá — a auto_increment se dozvíme až po vložení.
       Tabulka je při generování prázdná, takže je to bezpečné. */
    $seq = 0;
    $dnes      = new DateTimeImmutable('today');

    for ($d = $dny; $d >= 0; $d--) {
        $den = $dnes->sub(new DateInterval("P{$d}D"));
        $vikend = (int)$den->format('N') >= 6;

        /* 1) vracení — většina v termínu, část dřív, část pozdě */
        foreach ($vypujcene as $k => $v) {
            /* Bez tohohle by v datech nikdy nebyla výpůjčka po termínu,
               protože opozdilci se do dneška stejně všichni vrátí — a přesně
               tyhle řádky potřebuje horní rám a upomínací agenda.
               Rozděleno na dvě skupiny schválně: „hodně pozdě" se nakonec
               vrátí a v datech se nehromadí, „nikdy" ano. Kdyby se
               nehromadila jen jedna z nich, měla by desetiletá historie
               tři tisíce trvale ztracených knih, což žádná knihovna nemá. */
            if ($v['nikdy']) continue;
            /* Doba držení je podíl výpůjční doby, ne pevný počet dní. S pevným
               jitterem se čtrnáctidenní výpůjčka vracela po dvou dnech, obrat
               byl třikrát rychlejší než skutečný a z desetileté historie
               vypadl milion operací místo tří set tisíc. */
            if ($den < $v['vrati_v']) continue;
            if ($v['pozde'] && $den < $v['do_kdy']->add(new DateInterval('P' . $v['pozdeni'] . 'D'))) continue;
            $radky[] = [++$seq, $v['us_id'], $knihy[$v['bo']]['id'], 'return',
                        $den->format('Y-m-d') . ' ' . sprintf('%02d:%02d:00', mt_rand(9, 18), mt_rand(0, 59)),
                        0, null, '', $v['re_id'], 0];
            $knihy[$v['bo']]['volnych']++;
            $ctenari[$v['ui']]['drzi']--;
            unset($vypujcene[$k]);
        }

        /* 2) půjčování — v týdnu víc než o víkendu, v létě méně */
        $mesic = (int)$den->format('n');
        $zaklad = ($vikend ? 3 : 9) * $nasobic;
        if ($mesic === 7 || $mesic === 8) $zaklad = (int)round($zaklad * 0.6);
        $pocet = max(0, $zaklad + mt_rand(-2 * $nasobic, 3 * $nasobic));

        for ($i = 0; $i < $pocet; $i++) {
            $pokusy = 0;
            do { $bi = gen_vaz($knihy); } while ($knihy[$bi]['volnych'] < 1 && ++$pokusy < 20);
            if ($knihy[$bi]['volnych'] < 1) continue;          // vše rozpůjčeno, dnes nic

            /* Komu už jde strop, tomu se nepůjčuje. Pár pokusů a jdeme dál —
               dnes se prostě půjčí méně, což je taky realita. */
            $pok = 0;
            do { $ui = gen_vaz($ctenari); } while ($ctenari[$ui]['drzi'] >= $ctenari[$ui]['strop'] && ++$pok < 15);
            if ($ctenari[$ui]['drzi'] >= $ctenari[$ui]['strop']) continue;
            $den_h = $den->format('Y-m-d') . ' ' . sprintf('%02d:%02d:00', mt_rand(9, 18), mt_rand(0, 59));
            $doba  = [14, 21, 30, 30, 30, 60][mt_rand(0, 5)];
            $dokdy = $den->add(new DateInterval("P{$doba}D"));

            $re_id = ++$seq;
            $radky[] = [$re_id, $ctenari[$ui]['id'], $knihy[$bi]['id'], 'rent', $den_h,
                        mt_rand(0, 3) ? 20.00 : 0.00,          // někdy zdarma
                        $dokdy->format('Y-m-d'), '', null, 0];
            $knihy[$bi]['volnych']--;
            $ctenari[$ui]['drzi']++;
            $vypujcene[] = ['re_id' => $re_id, 'bo' => $bi, 'ui' => $ui, 'us_id' => $ctenari[$ui]['id'],
                            'datum' => $den_h, 'do_kdy' => $dokdy,
                            /* vrátí mezi 55 a 100 % výpůjční doby */
                            'vrati_v' => $den->add(new DateInterval('P' . (int)round($doba * mt_rand(55, 100) / 100) . 'D')),
                            'pozdeni' => mt_rand(180, 540),
                            'pozde' => mt_rand(1, 1000) <= 6,     /* 0,6 % se vrátí za půl roku až rok a půl */
                            'nikdy' => mt_rand(1, 2000) <= 1];    /* 0,05 % se nevrátí nikdy */
        }
    }

    /* Zápis po dávkách — jeden INSERT na tisíc řádků. */
    $db->beginTransaction();
    $q = $db->prepare("INSERT INTO rentals
          (re_id, re_user, re_book, re_operation, re_date, re_price, re_allowed_return_date,
           re_note, re_rent_id, re_returned)
          VALUES (?,?,?,?,?,?,?,?,?,?)");
    foreach ($radky as $r) $q->execute($r);
    /* Příznak se dopočítá jedním dotazem, ne v cyklu: půjčení je uzavřené
       tehdy, když na něj ukazuje nějaké vrácení. Tímhle se zároveň ověří,
       že párování přes re_rent_id drží. */
    $db->exec("UPDATE rentals r SET r.re_returned = 1
               WHERE r.re_operation = 'rent'
                 AND EXISTS (SELECT 1 FROM (SELECT re_rent_id FROM rentals WHERE re_operation='return') z
                             WHERE z.re_rent_id = r.re_id)");
    $db->commit();

    $po_terminu = 0; $t = $dnes->format('Y-m-d');
    foreach ($vypujcene as $v) if ($v['do_kdy']->format('Y-m-d') < $t) $po_terminu++;

    return ['operaci' => count($radky), 'otevrenych' => count($vypujcene),
            'po_terminu' => $po_terminu, 'ctenaru' => count($ctenari),
            'dny' => $dny, 'nasobic' => $nasobic];
}
