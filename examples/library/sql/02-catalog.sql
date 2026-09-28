-- =====================================================================
--  Číselníky: oprávnění a žánry
--
--  Oprávnění jsou ŘÁDKY. Nová funkce v aplikaci = jeden INSERT, nikdy
--  ALTER TABLE. Číselník zároveň vykresluje zaškrtávátka ve správě
--  uživatelů a validuje, co se smí uložit do users.us_acl.
-- =====================================================================

SET NAMES utf8mb4;

INSERT INTO acls (ac_key, ac_name, ac_description, ac_sort) VALUES
 ('rent_books',      'Půjčovat a vracet',  'Smí zapsat výpůjčku i vrácení. Bez toho jsou tlačítka jen nakreslená pro čtení.', 10),
 ('see_all_rentals', 'Vidět cizí výpůjčky','Bez tohoto práva vidí uživatel jen svoje. Filtr na jeho id dělá backend, ne klient.', 20),
 ('see_statistics',  'Statistika',         'Přehledy nad celou knihovnou.', 30),
 ('manage_books',    'Správa knih',        'Zakládat, měnit a vyřazovat knihy a žánry.', 40),
 ('manage_users',    'Správa uživatelů',   'Zakládat konta a přidělovat oprávnění. Tohle je admin.', 50);

INSERT INTO genres (ge_id, ge_name, ge_description) VALUES
 (1, 'Klasická próza',   'Romány a novely, které přežily svou dobu.'),
 (2, 'Poezie',           'Verše, sbírky, básnické skladby.'),
 (3, 'Drama',            'Hry určené pro jeviště.'),
 (4, 'Dobrodružná',      'Cesty, výpravy a napětí.'),
 (5, 'Detektivka',       'Zločin a jeho rozplétání.'),
 (6, 'Fantastika',       'Vědecká fantastika a utopie.'),
 (7, 'Filosofie a esej', 'Myšlení o světě, ne vyprávění o něm.'),
 (8, 'Mýty a pohádky',   'Vyprávění starší než knihtisk.'),
 (9, 'Historie',         'Doba, která už byla.');
