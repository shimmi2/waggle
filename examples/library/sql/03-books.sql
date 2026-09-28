-- =====================================================================
--  Knihy — ukázková data
--
--  VÝHRADNĚ volné dílo: autoři zemřelí před více než sedmdesáti lety.
--  Díky tomu se dá tenhle seznam ukazovat, forkovat i tisknout, aniž by
--  kdokoli řešil práva. Popisky jsou psané pro tenhle příklad, nejsou
--  to opsané anotace.
--
--  Když si tabulku nahradíš svou agendou, tohle je ten soubor, co se
--  smaže první.
-- =====================================================================

SET NAMES utf8mb4;

INSERT INTO books (bo_name, bo_author, bo_genre, bo_year, bo_count, bo_price, bo_description) VALUES
 ('Babička','Božena Němcová',1,1855,4,180.00,'Vzpomínka na venkovské dětství, ze které se stala národní čítanka.'),
 ('Povídky malostranské','Jan Neruda',1,1878,3,220.00,'Třináct obrázků ze staré Prahy, každý o jednom sousedovi.'),
 ('Kytice','Karel Jaromír Erben',2,1853,5,150.00,'Balady, ve kterých za každou vinou přijde odplata.'),
 ('Máj','Karel Hynek Mácha',2,1836,4,120.00,'Lyrickoepická skladba o vině, trestu a marnosti.'),
 ('Osudy dobrého vojáka Švejka','Jaroslav Hašek',1,1923,5,390.00,'Válka viděná zdola, kde je největší zbraní blbost na správném místě.'),
 ('R.U.R.','Karel Čapek',3,1920,3,160.00,'Hra, ve které se poprvé objevilo slovo robot.'),
 ('Válka s mloky','Karel Čapek',6,1936,4,290.00,'Satira o tom, jak se civilizace prodá za výnos.'),
 ('Krakatit','Karel Čapek',6,1924,2,260.00,'Muž, který vynalezl výbušninu a nemohl ji udržet.'),
 ('Bílá nemoc','Karel Čapek',3,1937,2,150.00,'Lékař s lékem a diktátor, který ho potřebuje.'),
 ('Temno','Alois Jirásek',9,1915,2,310.00,'Pobělohorská doba jako soumrak, ve kterém se dá i tak žít.'),
 ('Staré povesti české','Alois Jirásek',8,1894,3,240.00,'Praotec Čech, Bivoj, Libuše a další, kdo tu byli před námi.'),
 ('Labyrint sveta a raj srdce','Jan Amos Komenský',7,1631,2,280.00,'Poutník prochází světem a nachází v něm jen shon.'),
 ('Romance pro křídlovku','Josef Hora',2,1932,2,140.00,'Verše o lásce, smrti a jednom venkovském létě.'),
 ('Kytice z pohádek','Karel Jaromír Erben',8,1865,3,170.00,'Pohádky sebrané dřív, než je stihlo město zapomenout.'),
 ('Svatopluk Čech: Písně otroka','Svatopluk Čech',2,1894,2,130.00,'Sbírka, která si na konci století troufla mluvit o svobodě.'),
 ('Nikola Šuhaj loupežník','Ivan Olbracht',1,1933,3,250.00,'Balada z Podkarpatské Rusi o muži, kterého kulka nebrala.'),
 ('Rozmarné léto','Vladislav Vančura',1,1926,3,190.00,'Tři muži, jedno provazochodecké číslo a řeč, jakou nikdo nemluví.'),
 ('Krysař','Viktor Dyk',1,1915,2,150.00,'Kdo si zahrává s pomstou, odvede za sebou celé město.'),
 ('Zločin a trest','Fjodor Michajlovič Dostojevskij',1,1866,4,420.00,'Student zabije lichvářku a zbytek knihy s tím žije.'),
 ('Bratři Karamazovi','Fjodor Michajlovič Dostojevskij',1,1880,2,450.00,'Otcovražda jako záminka k otázce, jestli je něco svaté.'),
 ('Anna Kareninová','Lev Nikolajevič Tolstoj',1,1878,3,430.00,'Vášeň proti společnosti, která vždycky vyhraje.'),
 ('Vojna a mír','Lev Nikolajevič Tolstoj',9,1869,2,490.00,'Napoleonské války viděné přes pět rodin.'),
 ('Revizor','Nikolaj Vasiljevič Gogol',3,1836,2,140.00,'Do města přijede nikdo a všichni se před ním hrbí.'),
 ('Višňový sad','Anton Pavlovič Čechov',3,1904,2,160.00,'Statek se prodá a nikdo pro to nezvedne ruku.'),
 ('Dvacet tisíc mil pod morem','Jules Verne',4,1870,4,330.00,'Nautilus, kapitán Nemo a oceán jako celý svět.'),
 ('Cesta kolem sveta za osmdesát dní','Jules Verne',4,1873,4,300.00,'Sázka, jízdní řád a jeden velmi přesný gentleman.'),
 ('Tajemný ostrov','Jules Verne',4,1875,3,340.00,'Pět trosečníků, kteří si vyrobí civilizaci od kamene.'),
 ('Tři mušketýři','Alexandre Dumas',4,1844,4,360.00,'Přátelství, intriky a šerm, ve kterém jde vždycky o víc.'),
 ('Hrabe Monte Cristo','Alexandre Dumas',4,1845,2,470.00,'Nespravedlivě uvězněný muž se vrátí jako trpělivá pomsta.'),
 ('Oliver Twist','Charles Dickens',1,1838,3,320.00,'Sirotek v Londýně, který se odmítne stát tím, co se od něj čeká.'),
 ('Vánoční koleda','Charles Dickens',1,1843,5,160.00,'Tři duchové za jednu noc a lakomec, kterému to stačí.'),
 ('Pýcha a předsudek','Jane Austen',1,1813,4,290.00,'Dvě chyby v úsudku, které trvá celý román opravit.'),
 ('Jane Eyrová','Charlotte Brontëová',1,1847,3,330.00,'Guvernantka, která si nedá říct, a dům s tajemstvím.'),
 ('Na Větrné hurce','Emily Brontëová',1,1847,2,320.00,'Láska tak posedlá, že přežije oba, kdo ji měli.'),
 ('Dobrodružství Sherlocka Holmese','Arthur Conan Doyle',5,1892,5,280.00,'Dvanáct případů a metoda, kterou od té doby opisuje každý.'),
 ('Pes baskervillský','Arthur Conan Doyle',5,1902,4,260.00,'Rodinná kletba, rašeliniště a pes, který svítí.'),
 ('Vraždy v ulici Morgue','Edgar Allan Poe',5,1841,3,180.00,'První detektivka historie, a hned s uzamčeným pokojem.'),
 ('Havran a jiné básně','Edgar Allan Poe',2,1845,3,170.00,'Verše, které zní jako kroky za dveřmi.'),
 ('Obraz Doriana Graye','Oscar Wilde',1,1890,3,270.00,'Portrét stárne místo svého majitele, a účet přijde stejně.'),
 ('Jak je důležité míti Filipa','Oscar Wilde',3,1895,2,150.00,'Komedie, kde je jediným vážným problémem jméno.'),
 ('Hamlet','William Shakespeare',3,1603,4,200.00,'Princ, který ví, co má udělat, a celou hru to odkládá.'),
 ('Romeo a Julie','William Shakespeare',3,1597,4,190.00,'Dvě rodiny, jeden spěch a všechno špatně načasované.'),
 ('Odysseia','Homér',8,1,3,380.00,'Deset let cesty domů, když už válka dávno skončila.'),
 ('Ilias','Homér',8,1,2,380.00,'Devátý rok obléhání Tróje a jeden velmi rozzlobený hrdina.'),
 ('Don Quijote','Miguel de Cervantes',1,1605,2,460.00,'Kdo čte příliš mnoho romantiky, začne bojovat s větrnými mlýny.'),
 ('Bídníci','Victor Hugo',1,1862,2,480.00,'Ukradený chléb a devatenáct let, které za něj někdo platí.'),
 ('Madame Bovaryová','Gustave Flaubert',1,1856,2,310.00,'Život na venkově proti tomu, co slibovaly knihy.'),
 ('Bílá velryba','Herman Melville',4,1851,2,410.00,'Kapitán Achab a posedlost, která potopí celou posádku.'),
 ('Dobrodružství Toma Sawyera','Mark Twain',4,1876,4,250.00,'Kluk, plot na malování a pohřeb, na který přijde sám.'),
 ('Ostrov skladu','Robert Louis Stevenson',4,1883,4,270.00,'Mapa s křížkem a pirát, kterému se nedá věřit ani nohou.'),
 ('Stroj času','Herbert George Wells',6,1895,3,210.00,'Cesta do roku 802701, kde si lidstvo rozdělilo role.'),
 ('Válka svetu','Herbert George Wells',6,1898,3,240.00,'Marťané přistáli v Anglii a nikdo na to nebyl zvědavý.'),
 ('Proces','Franz Kafka',1,1925,3,280.00,'Josef K. je obžalován a nikdo mu neřekne z čeho.'),
 ('Přemena','Franz Kafka',1,1915,4,140.00,'Ráno se probudí jako hmyz a rodina to řeší jako nepříjemnost.'),
 ('Faust','Johann Wolfgang von Goethe',3,1808,2,350.00,'Sázka o duši, kterou uzavřel učenec ze samé nudy.'),
 ('Tak pravil Zarathustra','Friedrich Nietzsche',7,1883,2,300.00,'Filosofie psaná jako proroctví, se vším rizikem toho tvaru.'),
 ('Hovory k sobe','Marcus Aurelius',7,180,3,230.00,'Císařovy zápisky pro sebe, ne pro nás. Proto fungují.'),
 ('Ústava','Platón',7,1,2,340.00,'Dialog o spravedlnosti, ze kterého vyjde návrh státu.'),
 ('Pohádky','Hans Christian Andersen',8,1837,5,220.00,'Ošklivé kačátko, cínový vojáček a císař bez šatů.'),
 ('Pohádky bratří Grimmu','Jacob a Wilhelm Grimmovi',8,1812,5,240.00,'Verze, které jsou o hodně temnější než ty dnešní.');
