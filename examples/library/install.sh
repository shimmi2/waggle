#!/bin/bash
# =====================================================================
#  install.sh — nasazení knihovny do tří oddělených domén
#
#  Tohle je ta vážná cesta. Proti webovému instaláku:
#
#    * rozkopíruje kód do TŘÍ docrootů, takže vrstvy jsou doopravdy
#      oddělené a backendové API není dosažitelné z internetu;
#    * vytvoří runtime účet databáze, který smí JEN čtení a zápis dat —
#      žádné DDL, protože aplikace za provozu nemá co měnit strukturu;
#    * zhasne příznak nebezpečné instalace;
#    * vypíše direktivy pro Apache i nginx s doplněnými cestami;
#    * z principu ho nelze spustit z hostingu, protože je to shell.
#
#  Přístupové údaje pro vytvoření schématu se NIKAM nezapisují. Do
#  konfigurace se dostane jen ten runtime účet.
#
#  Použití:
#    ./install.sh --conf=nasazeni.conf
#    ./install.sh --app-dir=… --bff-dir=… --api-dir=… \
#                 --app-url=… --bff-url=… --api-url=… \
#                 --db-name=… --prov-user=root [--prov-pass=…] \
#                 --admin-login=admin [--profil=mestska] [--force]
#
#  Soubor --conf je obyčejný shell: APP_DIR=..., BFF_URL=... a tak dál.
# =====================================================================

set -u
SRC="$(cd "$(dirname "$0")" && pwd)"

APP_DIR=; BFF_DIR=; API_DIR=
APP_URL=; BFF_URL=; API_URL=
DB_HOST=localhost; DB_NAME=knihovna; DB_RUNTIME_USER=knihovna; DB_RUNTIME_PASS=
PROV_USER=root; PROV_PASS=
ADMIN_LOGIN=admin; ADMIN_PASS=
REDIS_HOST=; CACHE_DIR=/dev/shm/knihovna-sessions
PROFIL=mestska; FORCE=0

for a in "$@"; do
  case "$a" in
    --conf=*)        . "${a#*=}" ;;
    --app-dir=*)     APP_DIR="${a#*=}" ;;
    --bff-dir=*)     BFF_DIR="${a#*=}" ;;
    --api-dir=*)     API_DIR="${a#*=}" ;;
    --app-url=*)     APP_URL="${a#*=}" ;;
    --bff-url=*)     BFF_URL="${a#*=}" ;;
    --api-url=*)     API_URL="${a#*=}" ;;
    --db-host=*)     DB_HOST="${a#*=}" ;;
    --db-name=*)     DB_NAME="${a#*=}" ;;
    --prov-user=*)   PROV_USER="${a#*=}" ;;
    --prov-pass=*)   PROV_PASS="${a#*=}" ;;
    --admin-login=*) ADMIN_LOGIN="${a#*=}" ;;
    --redis-host=*)  REDIS_HOST="${a#*=}" ;;
    --cache-dir=*)   CACHE_DIR="${a#*=}" ;;
    --profil=*)      PROFIL="${a#*=}" ;;
    --force)         FORCE=1 ;;
    -h|--help)       sed -n '2,30p' "$0"; exit 0 ;;
    *) echo "neznámý přepínač: $a" >&2; exit 2 ;;
  esac
done

zle() { echo "  !! $*" >&2; exit 1; }
rek() { echo "  $*"; }

# ---- kontroly ------------------------------------------------------
for v in APP_DIR BFF_DIR API_DIR APP_URL BFF_URL API_URL; do
  [ -n "${!v}" ] || zle "chybí --$(echo "$v" | tr 'A-Z_' 'a-z-')"
done
[[ "$DB_NAME" =~ ^[A-Za-z0-9_]+$ ]] || zle "jméno databáze smí být jen písmena, číslice a podtržítko"
command -v php   >/dev/null || zle "php v PATH není"
command -v mysql >/dev/null || zle "mysql v PATH není"
php -r 'exit(PHP_VERSION_ID >= 80100 ? 0 : 1);' || zle "je potřeba PHP 8.1 nebo novější"
php -m | grep -qx pdo_mysql || zle "chybí rozšíření pdo_mysql"
php -m | grep -qx curl      || zle "chybí rozšíření curl (BFF s ním volá API)"

# Tři různé cesty jsou celý smysl téhle varianty.
if [ "$APP_DIR" = "$BFF_DIR" ] || [ "$BFF_DIR" = "$API_DIR" ] || [ "$APP_DIR" = "$API_DIR" ]; then
  zle "dva cíle ukazují na tentýž adresář — pak nejsou vrstvy oddělené a na to je install.php"
fi
for d in "$APP_DIR" "$BFF_DIR" "$API_DIR"; do
  [ -d "$d" ] || zle "adresář neexistuje: $d"
  if [ -n "$(ls -A "$d" 2>/dev/null)" ] && [ "$FORCE" = 0 ]; then
    zle "adresář není prázdný: $d (přepiš přes --force, ale koukni, co v něm je)"
  fi
done

# ---- hesla ---------------------------------------------------------
if [ -z "$PROV_PASS" ]; then read -r -s -p "  heslo účtu $PROV_USER (pro vytvoření schématu): " PROV_PASS; echo; fi
if [ -z "$ADMIN_PASS" ]; then
  read -r -s -p "  heslo správce ${ADMIN_LOGIN} (min. 8 znaků): " ADMIN_PASS; echo
  [ "${#ADMIN_PASS}" -ge 8 ] || zle "heslo správce je krátké"
fi
# Runtime heslo si vyrobíme sami. Uživatel ho nikdy nemusí vidět ani znát.
[ -n "$DB_RUNTIME_PASS" ] || DB_RUNTIME_PASS="$(php -r 'echo bin2hex(random_bytes(18));')"

MY="mysql -h $DB_HOST -u $PROV_USER -p$PROV_PASS"
$MY -e "SELECT 1" >/dev/null 2>&1 || zle "k databázi se s těmi údaji nepřipojím"

echo
rek "=== 1/6 databáze ==="
$MY -e "CREATE DATABASE IF NOT EXISTS \`$DB_NAME\` CHARACTER SET utf8mb4" || zle "databázi nejde vytvořit"
for f in 01-schema 02-catalog 03-books 04-users; do
  $MY "$DB_NAME" < "$SRC/sql/$f.sql" || zle "$f.sql se nenačetl"
  rek "$f.sql"
done

rek "=== 2/6 runtime účet (bez DDL) ==="
$MY -e "
CREATE USER IF NOT EXISTS '$DB_RUNTIME_USER'@'localhost' IDENTIFIED BY '$DB_RUNTIME_PASS';
ALTER USER '$DB_RUNTIME_USER'@'localhost' IDENTIFIED BY '$DB_RUNTIME_PASS';
REVOKE ALL PRIVILEGES, GRANT OPTION FROM '$DB_RUNTIME_USER'@'localhost';
GRANT SELECT, INSERT, UPDATE, DELETE ON \`$DB_NAME\`.* TO '$DB_RUNTIME_USER'@'localhost';
FLUSH PRIVILEGES;" || zle "runtime účet nejde vytvořit"
rek "$DB_RUNTIME_USER@localhost smí jen SELECT/INSERT/UPDATE/DELETE na $DB_NAME"

rek "=== 3/6 správce a ukázková data ==="
php -r '
$db = new PDO("mysql:host=".$argv[1].";dbname=".$argv[2].";charset=utf8mb4", $argv[3], $argv[4],
              [PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION]);
$db->prepare("INSERT INTO users (us_login,us_password,us_name,us_acl) VALUES (?,?,?,?)")
   ->execute([$argv[5], password_hash($argv[6], PASSWORD_DEFAULT), "Správce",
              "rent_books,see_all_rentals,see_statistics,manage_books,manage_users"]);
' "$DB_HOST" "$DB_NAME" "$PROV_USER" "$PROV_PASS" "$ADMIN_LOGIN" "$ADMIN_PASS" || zle "správce se nezaložil"
rek "správce $ADMIN_LOGIN"

if [ "$PROFIL" = "mestska" ]; then DNY=3650; KUSU=6; CTEN=400; else DNY=730; KUSU=1; CTEN=40; fi
php "$SRC/sql/gen-rentals.php" --dsn="mysql:host=$DB_HOST;dbname=$DB_NAME" \
    --user="$PROV_USER" --pass="$PROV_PASS" --days=$DNY --copies=$KUSU --readers=$CTEN \
    || zle "ukázková data se nevygenerovala"

rek "=== 4/6 kód do tří adresářů ==="
# Vzorové konfigurace se do nasazení nekopírují. Tajné v nich nic není,
# ale je to zbytečná nápověda o struktuře a po instalaci k ničemu.
cp -r "$SRC/api/." "$API_DIR/" && rm -f "$API_DIR/config.inc" "$API_DIR/config.example.inc"
cp -r "$SRC/bff/." "$BFF_DIR/" && rm -f "$BFF_DIR/config.inc" "$BFF_DIR/config.example.inc"
cp -r "$SRC/app/." "$APP_DIR/" && rm -f "$APP_DIR/config.js" "$APP_DIR/config.example.js"
mkdir -p "$API_DIR/../cronjobs" 2>/dev/null
cp "$SRC/cronjobs/cleaner.php" "$API_DIR/../cronjobs/" 2>/dev/null \
  && rek "cleaner.php vedle API (mimo docroot)" \
  || cp "$SRC/cronjobs/cleaner.php" "$API_DIR/cleaner.php"
rek "api → $API_DIR"; rek "bff → $BFF_DIR"; rek "app → $APP_DIR"

rek "=== 5/6 konfigurace ==="
php -r '
[$_, $sab, $cil, $json] = $argv;
$t = file_get_contents($sab);
foreach (json_decode($json, true) as $k => $v)
    $t = preg_replace("/define\(\x27" . preg_quote($k, "/") . "\x27,\s*[^)]*\)/",
                      "define(\x27$k\x27, " . var_export($v, true) . ")", $t, 1);
file_put_contents($cil, $t);
' "$SRC/api/config.example.inc" "$API_DIR/config.inc" "$(php -r '
echo json_encode(["DB_DSN"=>"mysql:host=".$argv[1].";dbname=".$argv[2].";charset=utf8mb4",
 "DB_USER"=>$argv[3],"DB_PASS"=>$argv[4],"REDIS_HOST"=>$argv[5],"BFF_URL"=>rtrim($argv[6],"/"),
 "UNSAFE_DEMO"=>false,"FW_DEBUG"=>false]);' "$DB_HOST" "$DB_NAME" "$DB_RUNTIME_USER" "$DB_RUNTIME_PASS" "$REDIS_HOST" "$BFF_URL")"

php -r '
[$_, $sab, $cil, $json] = $argv;
$t = file_get_contents($sab);
foreach (json_decode($json, true) as $k => $v)
    $t = preg_replace("/define\(\x27" . preg_quote($k, "/") . "\x27,\s*[^)]*\)/",
                      "define(\x27$k\x27, " . var_export($v, true) . ")", $t, 1);
file_put_contents($cil, $t);
' "$SRC/bff/config.example.inc" "$BFF_DIR/config.inc" "$(php -r '
echo json_encode(["API_URL"=>rtrim($argv[1],"/"),"FRONTEND_URL"=>rtrim($argv[2],"/"),
 "BFF_CACHE_DIR"=>$argv[3],"UNSAFE_DEMO"=>false,"FW_DEBUG"=>false]);' "$API_URL" "$APP_URL" "$CACHE_DIR")"

printf "window.LIB_BFF_URL = '%s/';\n" "$(echo "$BFF_URL" | sed 's:/*$::')" > "$APP_DIR/config.js"
chmod 640 "$API_DIR/config.inc" "$BFF_DIR/config.inc"
mkdir -p "$CACHE_DIR" 2>/dev/null && chmod 700 "$CACHE_DIR"
rek "tři konfigurace, příznak nebezpečné instalace zhasnutý, FW_DEBUG vypnutý"

rek "=== 6/6 ověření ==="
for u in "$API_URL/?fn=ping" "$BFF_URL/"; do
  k=$(curl -s -o /dev/null -w '%{http_code}' -m 8 "$u" 2>/dev/null)
  [ "$k" = "200" ] && rek "$u → 200" || rek "!! $u → ${k:-nedostupné} (zkontroluj vhost)"
done
# Tohle je ta nejpravděpodobnější příčina, proč to někomu nepojede.
if ! php -r '
$c=curl_init(rtrim($argv[1],"/")."/?fn=ping");
curl_setopt_array($c,[CURLOPT_RETURNTRANSFER=>1,CURLOPT_TIMEOUT=>8]);
$r=curl_exec($c); exit($r !== false && str_contains((string)$r,"pong") ? 0 : 1);' "$API_URL"; then
  echo
  zle "BFF nedosáhne na API ($API_URL). Bez toho aplikace nepojede.
     Zkus: je to jméno z tohohle stroje dostupné? Nezakazuje hosting
     odchozí HTTP? Pomůže někdy API_URL na 127.0.0.1 s hlavičkou Host."
fi

cat <<KONEC

  Hotovo.

  Aplikace:  $APP_URL
  Správce:   $ADMIN_LOGIN

  ZBÝVÁ UDĚLAT DVĚ VĚCI RUČNĚ.

  1) Zakázat na webserveru to, co nemá být veřejné. Pro Apache:

     <FilesMatch "\.inc\$">
         Require all denied
     </FilesMatch>
     <FilesMatch "^\.">
         Require all denied
     </FilesMatch>

     Pro nginx do server bloku — POZOR na pořadí, tohle musí být PŘED
     blokem, který posílá .php na PHP-FPM, jinak se .inc chytne jako PHP:

     location ~ \.inc\$   { return 403; }
     location ~ /\.       { return 403; }
     location ~ \.php\$ {
         include snippets/fastcgi-php.conf;
         fastcgi_pass unix:/run/php/php8.4-fpm.sock;
     }

     Pak to OVĚŘ, nespoléhej na to:
     curl -o /dev/null -w '%{http_code}\n' $API_URL/config.inc
     Musí vrátit 403. Když vrátí 200, servíruješ heslo k databázi.

  2) Cron na úklid — potřeba JEN když nemáš Redis:

     */5 * * * * /usr/bin/php $API_DIR/../cronjobs/cleaner.php --quiet

  Push přes nchan tenhle instalák nenastavuje. Bez něj aplikace funguje,
  jen u statistiky nejsou procenta. Konfigurace je v
  nginx-nchan.conf.example u frameworku.

KONEC
