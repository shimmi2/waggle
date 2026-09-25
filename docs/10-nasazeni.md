# 10 — Nasazení a provoz

## Minimální požadavky

PHP 7.4+ (testováno na 7.4 i 8.4), jakýkoli webserver. Push navíc potřebuje
nginx s modulem nchan. Klient potřebuje `fetch`, `Promise` a `URLSearchParams`;
push navíc `EventSource`, synchronizace mezi okny `BroadcastChannel`.

## Rozdělení app / api

Aplikace zná jedinou adresu: `Fw.init({api: '…'})`. Nic víc. Proto může
`/app` a `/api` ležet kdekoli, i na jiných hostech.

Při rozdělení na hosty je potřeba CORS — `fw_boot()` odbaví preflight
a `fw_cors()` odráží `Origin`. Zvaž, jestli origin neomezit na seznam.

## Produkční checklist

| | |
|---|---|
| `FW_DEBUG = false` | v `api/config.inc`; jinak chybová hláška prozradí cesty |
| `AUTH_SECRET` | změnit a držet mimo webroot |
| `.inc` | ověřit `curl -w '%{http_code}'` na soubor s hesly — musí být **403** |
| `.htaccess` | ověřit, že `pages/`, `bin/`, `data/` vracejí 403; pozor na `AllowOverride None` |
| `.git` | nesmí být ve webrootu |
| session | demo má podepsaný token; reálná aplikace chce úložiště |
| tokeny kanálů | demo má JSON bez zamykání → Redis nebo databáze |
| `STREAM_PHP` | pin na konkrétní binárku |
| HTTPS | povinné, jinak nepůjde push |

## Apache

Funguje bez konfigurace. Jediné omezení se týká **streamovaných** odpovědí:

`mod_proxy_fcgi` drží výstup a uvolňuje ho po blocích, takže progress bar
přes `progress_run` poskakuje po ~4 s. PHP přitom posílá dávky přesně po
0,1 s — ověřeno na vestavěném serveru PHP.

Oprava vyžaduje zásah do konfigurace:

```apache
<Proxy "unix:/var/run/php/php7.4-fpm.sock|fcgi://framework-stream/">
    ProxySet flushpackets=on
</Proxy>
```
```apache
# .htaccess v adresáři, který streamuje
<FilesMatch "\.php$">
    SetHandler "proxy:unix:/var/run/php/php7.4-fpm.sock|fcgi://framework-stream"
</FilesMatch>
```

Pojmenovaný worker (`fcgi://framework-stream`) zajistí, že se `flushpackets`
dotkne jen tohohle adresáře a ne ostatních webů na stroji.

**Jednodušší cesta: nepoužívat stream a jít pushem.** Pak Apache v cestě
streamu vůbec není a problém zmizí.

### nginx jako webserver aplikace

Kdo místo Apache servíruje aplikaci nginxem, má stejný problém
s bufferem a řeší ho takhle:

```nginx
location ~ \.php$ {
    include snippets/fastcgi-php.conf;
    fastcgi_pass unix:/var/run/php/php8.4-fpm.sock;

    # jen pro streamované odpovědi; bez toho drží nginx výstup stejně
    # jako Apache a dávky dorazí naráz až na konci
    fastcgi_buffering off;
    gzip              off;   # gzip si vyrobí vlastní buffer
}
```

Direktivy pro zákaz `.inc` a neveřejných adresářů jsou v
[09 — Bezpečnost](09-bezpecnost.md); patří do každé instalace, ne jen
do té streamované.

I tady platí, že **pushem je to jednodušší** — pak se buffering řešit
nemusí vůbec.

## nginx + nchan

```
apt install libnginx-mod-nchan
cp nginx-nchan.conf.example /etc/nginx/sites-available/fw-nchan
ln -s ../sites-available/fw-nchan /etc/nginx/sites-enabled/
nginx -t && systemctl reload nginx
```

Dva server bloky, každý s jiným úkolem:

**Odběratelé — veřejně, port 8443, povinně TLS.** Aplikace běží na https,
takže `EventSource` na http by prohlížeč zablokoval jako mixed content.
Certifikát klidně ten samý, co má Apache.

```nginx
location /nchan/sub {
    nchan_subscriber                eventsource;
    nchan_channel_id                $arg_token;
    nchan_subscriber_first_message  newest;
    # CORS řeší nchan sám — NEPŘIDÁVAT add_header Access-Control-*
}
```

**Publikace — jen localhost, port 8081, bez TLS.**

```nginx
location /nchan/pub {
    allow 127.0.0.1;
    deny  all;
    nchan_publisher             http;
    nchan_channel_id            $arg_token;
    nchan_message_buffer_length 32;
    nchan_message_timeout       5m;
}
```

Buffer zprávy znamená, že klient, který se na chvíli odpojí, si po návratu
vyzvedne, co mu uteklo — `EventSource` pošle `Last-Event-ID` sám.

Bez TLS schválně: spojení nikdy neopustí stroj a PHP nemusí ověřovat
certifikát.

### Frontend na víc doménách

URL odběru se odvozuje z hostu, na kterém aplikace běží. Jede-li frontend
na dvou doménách, musí je nchan umět obě — jinak prohlížeč spojení odmítne
na certifikátu a v konzoli není nic než neúspěšný `EventSource`.

**Víc certifikátů v jednom server bloku nejde.** Několik direktiv
`ssl_certificate` vedle sebe je jen pro několik typů klíče (RSA + ECDSA)
k témuž jménu. Na různé domény je od toho SNI, a to v nginxu znamená
**samostatný server blok** se svým `server_name` a svým certifikátem.
Společný `location /nchan/sub` patří do snippetu, který oba bloky
includují — viz `nginx-nchan.conf.example`.

Jeden wildcard certifikát často pokryje víc projektů naráz, takže bloků
nemusí být tolik, kolik je domén. Ověř si, co v něm doopravdy je:

```bash
openssl x509 -in fullchain.pem -noout -ext subjectAltName
```

## Posílání do prohlížeče mimo odpověď

Apache drží streamovanou odpověď, ale **pushe se to netýká**.
`fw_publish()` je krátký POST na nchan na localhostu — Apache v té cestě
vůbec není. Uvnitř běžícího requestu tedy stačí:

```php
fw_publish(STREAM_PUB_URL, $tok, [fw_busy('main', 'Kompletuji data…')]);
$data = dlouhy_dotaz();          // prohlížeč už kolečko točí
```

Žádný stream, žádný worker, žádná změna konfigurace Apache.

Pro to, co PHP request udělat neumí, je `tools/async_sender.php`:

```bash
php tools/async_sender.php --token=TOKEN --busy='Zálohuji…' --pct=40
echo '{"op":"notify","kind":"success","message":"Hotovo"}' \
  | php tools/async_sender.php --token=TOKEN
```

Hodí se pro démona, cron nebo shellový skript, který chce něco napsat
do otevřené stránky správce, pro práci pokračující po konci odpovědi,
a pro jazyk, který není PHP — ten pošle JSON na stdin a je hotovo.
Token je klíč kanálu z příkazu `subscribe`, ne přihlašovací údaj.
Návratový kód: 0 odesláno, 1 chyba vstupu, 2 broker neodpověděl.
**Kontroluj ho** — tichý neúspěch je horší než hlasitý.

## Výkon

Naměřeno na demu, 101 kroků po 0,1 s:

| | POST drží | doručení |
|---|---|---|
| `progress_run` (stream) | **10,1 s** | Apache slepuje po ~4 s |
| `progress_push` (nchan) | **0,04 s** | přesně po 0,1 s |

U `mpm_prefork` drží streamovaný požadavek celý Apache proces. Push drží
jen socket v nginxu, což je řádově levnější. **Cokoli delšího než pár
sekund patří na push.**

## Ověření funkčnosti

```bash
# API
curl -s -H "X-App-Serial: t" -d "function=index" https://host/framework/api/

# push kanál — POZOR na hlavičku Accept, bez ní nchan vrátí 403
curl -N -H "Accept: text/event-stream" \
     "https://host:8443/nchan/sub?token=…"

# publikace
curl -X POST -d '{"v":1,"cmds":[]}' "http://127.0.0.1:8081/nchan/pub?token=…"

# neveřejné adresáře musí vracet 403
curl -o /dev/null -w '%{http_code}\n' https://host/framework/api/pages/lte/sync.inc
```
