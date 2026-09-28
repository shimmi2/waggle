-- =====================================================================
--  Uživatelé — ukázková data
--
--  POZOR: všechna konta tu vznikají s PRÁZDNÝM heslem, tedy
--  nepřihlašitelná. password_verify() proti prázdnému hashi vždycky
--  selže, takže z tohohle souboru sám o sobě nevznikne žádná cesta
--  dovnitř.
--
--  Admina s heslem zakládá instalák. Demo hesla ukázkovým kontům
--  nastavuje taky instalák, a jen když si to vyloženě vyžádáš —
--  ve webové (nebezpečné) variantě je to předvolené, v shellové ne.
--
--  Obsluha i čtenáři jsou TITÍŽ uživatelé. Liší se výhradně oprávněním,
--  a to je celý ten vtip: žádné dvě přihlašovací obrazovky, žádné dvě
--  aplikace, jen jiný obsah menu a jiné filtrování dat.
-- =====================================================================

SET NAMES utf8mb4;

INSERT INTO users (us_login, us_password, us_name, us_phone, us_email, us_acl, us_note) VALUES
 -- obsluha
 -- Jediná z obsluhy, kdo smí do katalogu. Rozdíl proti Dvořákovi je
 -- jedno slovo v us_acl a nic jiného — to je na tom to podstatné.
 ('hlavata','', 'Jana Hlavatá',   '605 112 233', 'hlavata@knihovna.example',
  'rent_books,see_all_rentals,see_statistics,manage_books', 'Vedoucí půjčovny.'),
 ('dvorak','',  'Petr Dvořák',    '605 445 566', 'dvorak@knihovna.example',
  'rent_books,see_all_rentals,see_statistics', 'Odpolední směna.'),
 -- čtenáři, bez jediného oprávnění
 ('novakova','','Eva Nováková',   '606 201 301', 'eva.novakova@example.com',  '', ''),
 ('svoboda','', 'Tomáš Svoboda',  '606 202 302', 'tomas.svoboda@example.com', '', ''),
 ('cerna','',   'Lucie Černá',    '606 203 303', 'lucie.cerna@example.com',   '', ''),
 ('prochazka','','Jan Procházka', '606 204 304', 'jan.prochazka@example.com', '', 'Chodí vracet na poslední chvíli.'),
 ('kucerova','','Marie Kučerová', '606 205 305', 'marie.kucerova@example.com','', ''),
 ('vesely','',  'Martin Veselý',  '606 206 306', 'martin.vesely@example.com', '', ''),
 ('horakova','','Anna Horáková',  '606 207 307', 'anna.horakova@example.com', '', ''),
 ('nemec','',   'Josef Němec',    '606 208 308', 'josef.nemec@example.com',   '', ''),
 ('pokorna','', 'Tereza Pokorná', '606 209 309', 'tereza.pokorna@example.com','', ''),
 ('marek','',   'Filip Marek',    '606 210 310', 'filip.marek@example.com',   '', 'Studuje, půjčuje po pěti.'),
 ('bartosova','','Klára Bartošová','606 211 311','klara.bartosova@example.com','', ''),
 ('urban','',   'David Urban',    '606 212 312', 'david.urban@example.com',   '', '');
