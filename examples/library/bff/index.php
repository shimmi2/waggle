<?php
/* =====================================================================
 *  BFF knihovny — jediný vstupní bod
 *
 *  Tahle vrstva řídí aplikaci a mluví Waggle. NESAHÁ na databázi ani na
 *  Redis; všechno si bere z backendového API. Kdyby na data sahala,
 *  nebylo by možné dát mezi ni a data jiné oprávnění — a celé to
 *  rozdělení by bylo jen na ozdobu.
 *
 *  Jeden switch, jedna obrazovka na case. Bezpečnostní šablona platí
 *  i tady, byť je tenčí: skutečná oprávnění hlídá backend, tady se
 *  rozhoduje jen o tom, co se NAKRESLÍ.
 *
 *  Kreslení není autorizace. Kdyby si někdo podvrhl cokoli a uviděl
 *  položku menu, na kterou nemá právo, dostane z backendu 403.
 * ===================================================================== */

require __DIR__ . '/config.inc';          // musí být první
require __DIR__ . '/fw.inc';              // rozváží fwdeploy.sh, needituj tady
require __DIR__ . '/inc/api_client.inc';
require __DIR__ . '/inc/session_cache.inc';
require __DIR__ . '/inc/ui.inc';

fw_boot();

/* CORS zúžený na jedinou adresu.
 *
 *  fw_cors() ve frameworku odráží jakýkoli Origin. Díra to není — token
 *  sezení jde hlavičkou a leží v localStorage, takže cizí stránka se
 *  k němu nedostane a dostane jen přihlašovací obrazovku. Ale je to
 *  volnější, než je tu potřeba: frontend má jednu adresu a nikdo jiný
 *  tohle BFF z prohlížeče volat nemá.
 *
 *  header() se stejným jménem předchozí hodnotu přepíše, takže stačí
 *  zavolat ji po fw_boot().
 */
$origin = req_header('Origin');
if ($origin !== '') {
    if ($origin === rtrim(FRONTEND_URL, '/')) {
        header('Access-Control-Allow-Origin: ' . $origin);
    } else {
        /* Nic neposlat je správná odpověď: prohlížeč spojení zablokuje
           sám a my nemusíme řešit, jestli vrátit 403 nebo mlčet. */
        header_remove('Access-Control-Allow-Origin');
        header_remove('Access-Control-Allow-Credentials');
    }
}

$SESSION  = req_header('X-App-Session');
$function = req('function', 64);
if ($function === '') $function = 'index';
if (!is_word($function)) throw_http_error(400, 'Neplatný název obrazovky');

/* ---------------------------------------------------------------------
 *  Kanál pro push. Odvozuje se ze tokenu sezení a nikde se neukládá —
 *  dvě hodnoty s vlastní životností by se rozešly a projevilo by se to
 *  jako „push občas nechodí", což se ladí měsíc.
 *
 *  Ze sezení kanál spočítáš, z kanálu sezení ne. To je správně: id
 *  kanálu nemá nic prozrazovat.
 * ------------------------------------------------------------------- */
function kanal(): string {
    global $SESSION;
    if ($SESSION === '' || NCHAN_PUB === '') return '';
    return substr(hash('sha256', 'waggle-library|' . $SESSION), 0, 32);
}

/* Průběh dlouhé operace. Bez nchanu se nic nestane a to je v pořádku —
   kolečko ukáže prohlížeč sám přes data-busy, jen bez procent. */
function prubeh(string $text, ?int $pct = null): void {
    $k = kanal();
    if ($k === '') return;
    if (!fw_publish(NCHAN_PUB, $k, [fw_busy('main', $text, $pct)]))
        error_log('[library-bff] nchan neodpovídá na ' . NCHAN_PUB . ', průběh se posílat nebude');
}

/* KOMPLETNÍ obrazovka — menu, horní rám i obsah. Používá se JEN při
   přihlášení, odhlášení a na úvodu, tedy když se mění stav aplikace.

   Menu se jinak nemění vůbec (jeho obsah plyne z oprávnění) a horní rám
   jen tehdy, když se změní číslo, které ukazuje. Posílat je u každé
   obrazovky by znamenalo překreslovat dvě třetiny stránky kvůli jedné
   tabulce — a v AdminLTE i znovu inicializovat šablonu. */
function obrazovka(string $telo, array $vars = []): array {
    return [
        ['op' => 'html', 'sel' => 'left_menu', 'content' => frag('menu')],
        ['op' => 'html', 'sel' => 'top_frame', 'content' => frag('top')],
        ['op' => 'html', 'sel' => 'main',      'content' => frag($telo, $vars)],
    ];
}

/* Prázdná kniha. Jedno místo, kde jsou výchozí hodnoty — jinak se
   rozejdou s tím, co čeká formulář. */
function prazdna_kniha(): array {
    return ['bo_id' => 0, 'bo_name' => '', 'bo_author' => '', 'bo_genre' => 0,
            'bo_year' => 0, 'bo_count' => 1, 'bo_price' => 0.0, 'bo_description' => ''];
}

/* Stav filtru katalogu, jak přišel s požadavkem. Putuje formulářem tam
   a zpět, aby se po uložení překreslil týž seznam. */
function filtr_katalogu(string $prefix = ''): array {
    return ['q'         => req($prefix . 'q', 64),
            'genre'     => req_int($prefix . 'genre'),
            'available' => req($prefix . 'available', 1),
            'order'     => req($prefix . 'order', 32),
            'offset'    => req_int($prefix . 'offset')];
}

/* BĚŽNÁ obrazovka — jen obsah. Tohle je ta, která se používá pořád. */
function jen_main(string $telo, array $vars = []): array {
    return [['op' => 'html', 'sel' => 'main', 'content' => frag($telo, $vars)]];
}

switch ($function) {

/* ---- přihlášení ------------------------------------------------- */
case 'index':
    send_answer(me() === null ? obrazovka('login') : obrazovka('intro'));
    send_answer(['op' => 'history', 'url' => '#?function=index']);
    break;

case 'do_login':
    $login = req('login', 64);
    $pass  = req('password', 256);
    [$code, $data] = api_raw('do_login', ['login' => $login, 'password' => $pass]);
    if ($code !== 200) {
        /* Hláška z backendu se předá tak, jak je — ten ví, jestli je to
           špatné heslo (401) nebo příliš mnoho pokusů (429). */
        send_answer(['op' => 'html', 'sel' => 'main',
                     'content' => frag('login', ['err' => $data['error'] ?? 'Přihlášení selhalo',
                                                 'login' => $login])]);
        break;
    }
    $SESSION = (string)$data['session'];
    cache_put($SESSION, $data['me']);
    send_answer([['op' => 'session', 'value' => $SESSION]]);   // musí být PRVNÍ
    if (NCHAN_SUB !== '' && NCHAN_PUB !== '')
        send_answer([['op' => 'subscribe', 'name' => 'main',
                      'url' => NCHAN_SUB, 'token' => kanal()]]);
    send_answer(obrazovka('intro'));
    break;

case 'do_logout':
    if ($SESSION !== '') { api_raw('do_logout', [], $SESSION); cache_forget($SESSION); }
    $SESSION = '';
    send_answer([['op' => 'session', 'value' => null], ['op' => 'unsubscribe']]);
    send_answer(obrazovka('login'));
    break;

/* ---- knihy ------------------------------------------------------ */
case 'books':
    /* main se dělí na filtr a výsledky. Filtr pak zůstane stát i s
       fokusem a překresluje se jen tabulka pod ním. */
    $g = api_call('genres_list');
    send_answer(jen_main('books', ['genres' => $g['genres']]));
    send_answer(['op' => 'history', 'url' => '#?function=books']);
    /* fallthrough do výsledků */
case 'books_results':
    $args = ['q' => req('q', 64), 'genre' => req_int('genre'),
             'available' => req('available', 1), 'order' => req('order', 32),
             'limit' => 50, 'offset' => req_int('offset')];
    $r = api_call('books_list', $args);
    send_answer(['op' => 'html', 'sel' => '#books_results',
                 'content' => frag('books_results', $r + ['args' => $args])]);
    break;

case 'book_detail':
    $r = api_call('book_detail', ['bo_id' => req_int('bo_id')]);
    send_answer(['op' => 'html', 'sel' => 'overlay',
                 'content' => frag('book_detail', $r + ['filtr' => filtr_katalogu()])]);
    break;

/* ---------------------------------------------------------------------
 *  Zápis knihy — editace i přidání. Tohle je vzor pro každou další
 *  zápisovou obrazovku v téhle aplikaci, takže stojí za popis.
 *
 *  1. Jeden fragment, dva vstupní stavy. book_form s bo_id načte hodnoty
 *     z backendu, bez bo_id vezme prázdné výchozí. Dva fragmenty by se
 *     rozešly.
 *
 *  2. Záměr se posílá výslovně jako op. Kdyby ho server odvozoval z
 *     přítomnosti bo_id, ztracené id by tiše založilo duplikát.
 *
 *  3. Chyba se kreslí ZPÁTKY do formuláře, ne jako notifikace. Proto se
 *     ukládá přes api_raw() a ne přes api_call() — ten by na první 400
 *     poslal error a skončil, takže by uživatel přišel o všechno, co
 *     napsal. Stejně se chová case do_login; není to výjimka, je to
 *     pravidlo pro formuláře.
 *
 *  4. 403 se do formuláře nekreslí. To není chyba vyplnění, to je
 *     „na tohle nemáš právo" a překreslený formulář by lhal, že to jde.
 * ------------------------------------------------------------------- */

case 'book_form':
    /* 1. vstupy */
    $id = req_int('bo_id');

    /* 2. sémantika — nic k ověřování, id je buď kladné (editace), nebo
       nula (nová kniha), a obojí je platné. */

    /* 3. sezení. OSVĚŽENÉ, ne z cache: kdyby se otevřel dialog nad
       sezením, které už hodinu neplatí, člověk by vyplnil celou knihu a
       vyhodilo by ho to teprve při ukládání. Jedno volání navíc je proti
       ztracené práci levné — a jen tady, čtecí obrazovky cache věří. */
    if (me(true) === null) {
        cache_forget($SESSION);
        send_answer([['op' => 'session', 'value' => null],
                     ['op' => 'error', 'code' => 401,
                      'message' => 'Sezení vypršelo, přihlaste se prosím znovu']]);
        break;
    }

    /* 4. oprávnění. Kreslení není autorizace — tohle rozhoduje jen o
       tom, jestli formulář nabídnout. Skutečné „smíš" padne u book_save
       a padne i tehdy, když tuhle podmínku někdo obejde. */
    if (!may('manage_books')) {
        send_answer(['op' => 'error', 'code' => 403, 'message' => 'Katalog spravuje jen knihovník']);
        break;
    }

    /* 5. práce, 6. odpověď */
    $g = api_call('genres_list');
    /* Prázdný dialog není zvláštní případ, je to jen jiná náplň. */
    $b  = $id > 0 ? api_call('book_detail', ['bo_id' => $id])['book'] : prazdna_kniha();
    send_answer(['op' => 'html', 'sel' => 'overlay',
                 'content' => frag('book_form', [
                     'book'   => $b,
                     'genres' => $g['genres'],
                     'op'     => $id > 0 ? 'update' : 'insert',
                     'err'    => null,
                     'filtr'  => filtr_katalogu(),
                 ])]);
    break;

case 'do_book_save':
    /* 1. vstupy
       Hodnoty se z požadavku vytáhnou JEDNOU a používají se pro volání
       i pro případné překreslení formuláře. Dvě čtení stejného pole se
       rozejdou přesně ve chvíli, kdy jedno z nich někdo upraví. */
    $op = req('op', 16);
    $b  = ['bo_id'          => req_int('bo_id'),
           'bo_name'        => req('bo_name', 255),
           'bo_author'      => req('bo_author', 128),
           'bo_genre'       => req_int('bo_genre'),
           'bo_year'        => req_int('bo_year'),
           'bo_count'       => req_int('bo_count'),
           'bo_price'       => req_float('bo_price'),
           'bo_description' => req('bo_description', 4000)];
    $filtr = filtr_katalogu('f_');

    /* 2. sémantika
       Nesrovnalost mezi op a bo_id není chyba vyplnění, ale rozbitý
       formulář — vlastní chyba téhle vrstvy. Kreslit ji do dialogu by
       byla past: uživatel by ji neopravil, protože špatné pole ani
       nevidí, a zůstal by ve formuláři, který se nikdy neuloží.
       Proto error op a ne překreslení.

       Backend má tutéž kontrolu. Není to zdvojení: tady si BFF hlídá
       smlouvu se svým vlastním formulářem, tam si backend hlídá smlouvu
       s kterýmkoli klientem. Ta druhá musí platit i pro klienta, který
       tenhle soubor nikdy neviděl. */
    if (($op !== 'insert' && $op !== 'update')
        || ($op === 'update' && $b['bo_id'] < 1)
        || ($op === 'insert' && $b['bo_id'] !== 0)) {
        send_answer(['op' => 'error', 'code' => 400,
                     'message' => 'Formulář poslal nesmyslný požadavek. Zkus dialog otevřít znovu.']);
        break;
    }

    /* 3. sezení. Tady STAČÍ cache: formulář se otevřel nad osvěženým
       blokem a od té doby uplynula chvilka, co člověk klikal na Uložit.
       Kdyby sezení mezitím padlo, api_raw niž vrátí 401 a ten se řeší
       tam. Tahle podmínka je proto jen zkratka, aby se do backendu
       nechodilo s tokenem, o kterém už víme, že je mrtvý. */
    if (me() === null) {
        cache_forget($SESSION);
        send_answer([['op' => 'session', 'value' => null],
                     ['op' => 'error', 'code' => 401,
                      'message' => 'Sezení vypršelo, přihlaste se prosím znovu']]);
        break;
    }

    /* 4. oprávnění. Zkratka, ne autorizace: ušetří kolečko do backendu a
       drží tenhle case čitelný ve stejném pořadí jako book_form. O tom,
       co se smí, rozhoduje acl_require() v api_books.inc, a rozhodne to
       i pro klienta, který tenhle soubor nikdy neviděl. */
    if (!may('manage_books')) {
        send_answer(['op' => 'error', 'code' => 403, 'message' => 'Katalog spravuje jen knihovník']);
        break;
    }

    /* 5. práce */
    [$code, $data] = api_raw('book_save', $b + ['op' => $op], $SESSION);

    if ($code !== 200) {
        if ($code === 401) {
            /* Vypršelé sezení. Stejně jako api_call: zahodit a poslat na
               přihlášení. Kdyby šel jen error, klient by si dál nosil
               mrtvý token a každé další kliknutí by skončilo takhle. */
            cache_forget($SESSION);
            send_answer([['op' => 'session', 'value' => null],
                         ['op' => 'error', 'code' => 401,
                          'message' => $data['error'] ?? 'Byl jste odhlášen']]);
            break;
        }
        if ($code === 403) {
            /* Sem formulář nepatří. Kreslit ho znovu by tvrdilo, že to
               po opravě půjde — nepůjde, chybí právo. */
            send_answer(['op' => 'error', 'code' => 403,
                         'message' => $data['error'] ?? 'Na tohle nemáš právo']);
            break;
        }
        /* Vyplnění je špatně. Formulář se překreslí s hláškou a se vším,
           co uživatel napsal. */
        $g = api_call('genres_list');
        send_answer(['op' => 'html', 'sel' => 'overlay',
                     'content' => frag('book_form', [
                         'book'   => $b,
                         'genres' => $g['genres'],
                         'op'     => $op,
                         'err'    => $data['error'] ?? 'Uložení se nepovedlo',
                         'filtr'  => $filtr,
                     ])]);
        break;
    }

    /* 6. odpověď
       Hotovo: zavřít dialog, říct to a překreslit týž seznam. Prázdný
       obsah v overlay je zavření — zavírá se stejnou cestou jako
       data-fw-clear na klientovi. */
    send_answer([
        ['op' => 'html',   'sel' => 'overlay', 'content' => ''],
        ['op' => 'notify', 'kind' => 'success',
         'message' => $op === 'insert' ? 'Kniha byla založena.' : 'Změny byly uloženy.'],
    ]);
    $r = api_call('books_list', $filtr + ['limit' => 50]);
    send_answer(['op' => 'html', 'sel' => '#books_results',
                 'content' => frag('books_results', $r + ['args' => $filtr])]);
    break;

/* ---- žánry ------------------------------------------------------ */
case 'genres':
    $r = api_call('genres_list');
    send_answer(jen_main('genres', $r));
    send_answer(['op' => 'history', 'url' => '#?function=genres']);
    break;

/* ---- výpůjčky --------------------------------------------------- */
case 'rentals':
    send_answer(jen_main('rentals'));
    send_answer(['op' => 'history', 'url' => '#?function=rentals']);
    /* fallthrough */
case 'rentals_results':
    $args = ['user' => req_int('user'), 'book' => req_int('book'),
             'open' => req('open', 1), 'overdue' => req('overdue', 1),
             'limit' => 50, 'offset' => req_int('offset')];
    $r = api_call('rentals_list', $args);
    send_answer(['op' => 'html', 'sel' => '#rentals_results',
                 'content' => frag('rentals_results', $r + ['args' => $args])]);
    break;

case 'do_return':
    api_call('rental_return', ['rental' => req_int('rental')]);
    send_answer(['op' => 'notify', 'kind' => 'success', 'message' => 'Kniha vrácena.']);
    $r = api_call('rentals_list', ['open' => '1', 'limit' => 50]);
    send_answer([['op' => 'html', 'sel' => '#rentals_results',
                  'content' => frag('rentals_results', $r + ['args' => ['open' => '1']])],
                 ['op' => 'html', 'sel' => 'top_frame', 'content' => frag('top')]]);
    break;

/* ---- statistika ------------------------------------------------- */
case 'statistics':
    /* Pět částí, pět volání, a mezi nimi se hlásí, kde to je. Procenta
       jsou skutečná: pátá část je hotová, když je hotová. */
    $casti = ['monthly' => 'Počítám výpůjčky po měsících…',
              'genres' => 'Sčítám podle žánrů…',
              'top_books' => 'Hledám nejpůjčovanější…',
              'top_readers' => 'Hledám nejaktivnější čtenáře…',
              'duration' => 'Počítám doby držení…'];
    $data = []; $i = 0; $n = count($casti);
    foreach ($casti as $part => $hlaska) {
        prubeh($hlaska, (int)round($i / $n * 100));
        $data[$part] = api_call('statistics', ['part' => $part])['rows'];
        $i++;
    }
    send_answer(jen_main('statistics', $data));
    send_answer(['op' => 'history', 'url' => '#?function=statistics']);
    break;

/* ---- uživatelé -------------------------------------------------- */
case 'users':
    $r = api_call('users_list', ['q' => req('q', 64), 'limit' => 200]);
    send_answer(jen_main('users', $r));
    send_answer(['op' => 'history', 'url' => '#?function=users']);
    break;

default:
    send_answer(['op' => 'error', 'code' => 404,
                 'message' => 'Taková obrazovka v téhle aplikaci není']);
}

finish_answer();
