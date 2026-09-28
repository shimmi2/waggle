-- =====================================================================
--  Knihovna — schéma
--
--  Konvence, které se vyplatí dodržet i v tabulkách, co si přidáš:
--
--    * Dvouznakový prefix podle tabulky (us_, bo_, re_). Díky tomu je
--      u každého JOINu na první pohled vidět, odkud sloupec je, a nikdy
--      nepotřebuješ alias jen kvůli kolizi jmen.
--    * Peníze jsou DECIMAL, nikdy FLOAT. Na FLOATu se sčítáním rozejdou
--      haléře a nikdo to pak nenajde.
--    * Oprávnění jsou ŘÁDKY v číselníku, ne ENUM ani SET. Přidání
--      oprávnění musí být INSERT, nikdy ALTER TABLE — jinak každá nová
--      funkce znamená migraci na tabulce uživatelů.
--    * utf8mb4, protože utf8 v MySQL není utf8 a neumí emoji.
-- =====================================================================

SET NAMES utf8mb4;

-- ---------------------------------------------------------------- lidé

CREATE TABLE users (
    us_id          INT AUTO_INCREMENT PRIMARY KEY,
    us_login       VARCHAR(64)  NOT NULL,
    us_password    VARCHAR(255) NOT NULL DEFAULT '',
    -- Typ hashe existuje JEN kvůli převodu starých systémů. Nové heslo
    -- je vždy 'bcrypt' přes password_hash(); 'plain' je tam proto, aby
    -- šlo naimportovat databázi z devadesátek a přehashovat při prvním
    -- úspěšném přihlášení.
    us_pwcrypttype ENUM('bcrypt','plain') NOT NULL DEFAULT 'bcrypt',
    us_name        VARCHAR(128) NOT NULL DEFAULT '',
    us_phone       VARCHAR(32)  NOT NULL DEFAULT '',
    us_email       VARCHAR(128) NOT NULL DEFAULT '',
    us_note        TEXT,
    -- Seznam klíčů z acls oddělený čárkami, např. 'rent_books,management'.
    -- Dotaz na držitele práva je FIND_IN_SET(); do desetitisíců uživatelů
    -- to stačí. Kdyby začalo vadit, přidá se spojovací tabulka AŽ POTOM
    -- a číselník už na místě je.
    us_acl         VARCHAR(255) NOT NULL DEFAULT '',
    us_created     DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY us_login (us_login),
    KEY us_name (us_name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Číselník oprávnění. Slouží ke dvěma věcem: vykreslí zaškrtávátka ve
-- správě uživatelů a validuje, co se smí uložit do users.us_acl.
CREATE TABLE acls (
    ac_key         VARCHAR(32)  NOT NULL PRIMARY KEY,
    ac_name        VARCHAR(64)  NOT NULL,
    ac_description VARCHAR(255) NOT NULL DEFAULT '',
    ac_sort        INT          NOT NULL DEFAULT 0
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ---------------------------------------------------------------- knihy

CREATE TABLE genres (
    ge_id          INT AUTO_INCREMENT PRIMARY KEY,
    ge_name        VARCHAR(64)  NOT NULL,
    ge_description VARCHAR(255) NOT NULL DEFAULT '',
    UNIQUE KEY ge_name (ge_name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE books (
    bo_id          INT AUTO_INCREMENT PRIMARY KEY,
    bo_name        VARCHAR(255)  NOT NULL,
    bo_author      VARCHAR(128)  NOT NULL DEFAULT '',
    bo_genre       INT           NOT NULL DEFAULT 0,
    bo_year        SMALLINT      NOT NULL DEFAULT 0,
    -- Počet kusů, které knihovna vlastní. Kolik je právě volných, se
    -- nedrží ve sloupci, ale počítá z rentals — jinak se ty dvě čísla
    -- při první chybě rozejdou a už se nikdy nesrovnají.
    bo_count       SMALLINT      NOT NULL DEFAULT 1,
    bo_price       DECIMAL(10,2) NOT NULL DEFAULT 0.00,
    bo_description TEXT,
    KEY bo_name (bo_name),
    KEY bo_author (bo_author),
    KEY bo_genre (bo_genre)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ------------------------------------------------------------ výpůjčky

-- Deník operací, ne stav. Jeden řádek = jedno půjčení nebo jedno vrácení.
-- Aktuální stav se z toho spočítá; opačně by to nešlo a historie by
-- chyběla přesně tam, kde ji reklamace potřebuje.
CREATE TABLE rentals (
    re_id                  INT AUTO_INCREMENT PRIMARY KEY,
    re_user                INT NOT NULL,
    re_book                INT NOT NULL,
    re_operation           ENUM('rent','return') NOT NULL,
    -- U vrácení ukazuje na to půjčení, které zavírá. U půjčení je NULL.
    --
    -- Bez tohohle sloupce se dá pár najít jen podle (kniha, uživatel)
    -- a pořadí dat — a to se nedá zaindexovat. Dotaz „jak dlouho byla
    -- kniha držená" pak nad čtvrt milionem řádků prohledává miliardu
    -- kombinací a neskončí. S tímhle je to spojení přes primární klíč.
    --
    -- Stejně tak „co je právě venku": půjčení, na které neukazuje žádné
    -- vrácení. LEFT JOIN přes index, ne korelovaný podotaz.
    re_rent_id             INT DEFAULT NULL,
    -- Je tahle výpůjčka už uzavřená? Platí jen na řádku 'rent'.
    --
    -- Ano, je to denormalizace, a ano, dostupnost ve sloupci jsem
    -- odmítl. Rozdíl je podstatný: dostupnost je POČET PŘES ŘÁDKY a při
    -- první chybě se rozejde, kdežto tohle je FAKT O TOMTÉŽ ŘÁDKU,
    -- zapsaný ve stejné transakci jako vrácení. Rozejít se nemá jak.
    --
    -- Proč: „co je právě venku" se bez něj musí ptát přes self join
    --   LEFT JOIN rentals z ON z.re_rent_id = r.re_id WHERE z.re_id IS NULL
    -- a to je v šesti místech aplikace. S příznakem je to jeden index
    -- (re_open) a plán type=ref, místo spojení tabulky se sebou.
    --
    -- Poctivě: na čtvrt milionu řádků je OBOJÍ stejně rychlé, 17 ms
    -- proti 21 ms. Příznak tu tedy není kvůli výkonu na téhle velikosti,
    -- ale kvůli jednoduchosti dotazů a proto, že cena self joinu roste
    -- s počtem vrácení, kdežto cena příznaku ne.
    re_returned            TINYINT NOT NULL DEFAULT 0,
    re_date                DATETIME NOT NULL,
    re_price               DECIMAL(10,2) NOT NULL DEFAULT 0.00,
    re_allowed_return_date DATE DEFAULT NULL,
    re_note                VARCHAR(255) NOT NULL DEFAULT '',
    -- Indexy jsou poskládané tak, aby otázky, které aplikace klade
    -- nejčastěji, šly z indexu bez dohledávání:
    --   „co má tenhle čtenář venku"      re_user
    --   „kolik kusů téhle knihy je venku" re_book
    --   „co je venku, od nejnovějšího"    re_open
    KEY re_user (re_user, re_returned, re_date),
    KEY re_book (re_book, re_returned, re_date),
    KEY re_open (re_operation, re_returned, re_date),
    KEY re_overdue (re_returned, re_allowed_return_date),
    KEY re_date (re_date),
    KEY re_rent_id (re_rent_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ------------------------------------------------- provozní tabulky

-- Session, když není Redis. Backend do ní sahá jediným modulem
-- (api_session_mysql.inc), takže se dá vyměnit za Redis beze změny
-- zbytku aplikace.
CREATE TABLE sessions (
    se_key     CHAR(64)  NOT NULL PRIMARY KEY,
    se_us_id   INT       NOT NULL,
    se_data    TEXT      NOT NULL,
    se_created DATETIME  NOT NULL DEFAULT CURRENT_TIMESTAMP,
    se_touched DATETIME  NOT NULL DEFAULT CURRENT_TIMESTAMP,
    KEY se_touched (se_touched)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Neúspěšné přihlášení. Dva klíče: 'ip:1.2.3.4' a 'user:jmeno'.
--
-- Podle IP samotné to nejde, protože za jednou adresou sedí celá firma
-- i celý mobilní operátor. Podle jména samotného je to horší než nic,
-- protože útočník pětkrát selže na 'admin' a zamkne ho právoplatnému
-- uživateli. Proto obojí zvlášť — a účet se NIKDY nezamyká natrvalo,
-- jen se odmítá v časovém okně.
CREATE TABLE login_fails (
    lf_id   INT AUTO_INCREMENT PRIMARY KEY,
    lf_key  VARCHAR(64) NOT NULL,
    lf_when DATETIME    NOT NULL DEFAULT CURRENT_TIMESTAMP,
    KEY lf_key (lf_key, lf_when)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
