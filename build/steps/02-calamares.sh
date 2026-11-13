#!/usr/bin/env bash
#
# build/steps/02-calamares.sh — compila Calamares 3.4.2 con il packaging AUR.
#
# Sorgente del packaging: snapshot AUR `calamares` (pkgver=3.4.2, pkgrel=2),
# lo stesso che build.sh/README indicano. Dal PKGBUILD si prendono versione,
# descrizione, licenza, dipendenze, URL del tarball upstream e sha256: niente
# valori inventati, e se l'AUR cambia versione il passo si ferma invece di
# produrre un pacchetto con metadati diversi da quelli attesi.
#
# Il build gira nel chroot di 01 (Qt6, KF6CoreAddons, KPMcore, yaml-cpp,
# libpwquality, ECM) e installa con DESTDIR in .build/stage/calamares, che è
# esattamente il percorso controllato da build.sh.
set -euo pipefail

# shellcheck source=../lib/common.sh
source "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../lib" && pwd)/common.sh"

SRC=$B/src/calamares
STAGE=$B/stage/calamares
META=$SRC/.nicoos-meta
AUR_SNAPSHOT=${NICOOS_AUR_SNAPSHOT:-https://aur.archlinux.org/cgit/aur.git/snapshot/calamares.tar.gz}

# La lista dei moduli esclusi è quella del PKGBUILD AUR (build(), variabile
# _skip_modules). Viene confrontata con il PKGBUILD scaricato più sotto:
# se l'AUR la cambia, un avviso lo dice invece di compilare qualcosa di
# diverso senza accorgersene.
SKIP_MODULES=(dracut dracutlukscfg dummycpp dummyprocess dummypython dummypythonqt \
    initramfs initramfscfg interactiveterminal packagechooser packagechooserq services-openrc)

if [[ -x $STAGE/usr/bin/calamares && -f $META ]] && (( ! ${NICOOS_FORCE:-0} )); then
    # shellcheck disable=SC1090
    source "$META"
    if [[ ${NICOOS_CALAMARES_PKGVER:-} == "$NICOOS_CALAMARES_VERSION" \
       && ${NICOOS_CALAMARES_PKGREL:-} == "$NICOOS_CALAMARES_PKGREL" ]]; then
        log "Calamares ${NICOOS_CALAMARES_PKGVER}-${NICOOS_CALAMARES_PKGREL} già in stage"
        exit 0
    fi
fi

[[ -x $B/root/usr/bin/cmake ]] || die "toolchain assente: esegui prima build/steps/01-toolchain.sh"

# ---------------------------------------------------------------------------
# 1. Packaging AUR
# ---------------------------------------------------------------------------
mkdir -p -- "$SRC" "$B/cache/src"
if [[ ! -f $SRC/PKGBUILD ]]; then
    tarball=$B/cache/src/calamares-aur.tar.gz
    if [[ ! -f $tarball ]]; then
        log "download del PKGBUILD AUR di calamares"
        curl -fL --retry 3 --connect-timeout 20 -o "$tarball.part" "$AUR_SNAPSHOT" \
            || die "impossibile scaricare $AUR_SNAPSHOT"
        mv -- "$tarball.part" "$tarball"
    fi
    bsdtar -xf "$tarball" -C "$SRC" --strip-components 1 \
        || die "snapshot AUR non estraibile"
fi
[[ -f $SRC/PKGBUILD ]] || die "PKGBUILD di calamares assente in $SRC"

# Si leggono solo le assegnazioni del PKGBUILD (le funzioni build/package non
# vengono chiamate): declare -p riproduce in modo sicuro versioni e array.
bash -c 'set -e; cd -- "$1"; source PKGBUILD;
         declare -p pkgver pkgrel pkgdesc url license arch depends makedepends source sha256sums' \
    _ "$SRC" >"$SRC/.metadata.bash" || die "PKGBUILD AUR non interpretabile"
# shellcheck disable=SC1091
source "$SRC/.metadata.bash"

if [[ $pkgver != "$NICOOS_CALAMARES_VERSION" || $pkgrel != "$NICOOS_CALAMARES_PKGREL" ]]; then
    die "il PKGBUILD AUR è calamares $pkgver-$pkgrel, atteso $NICOOS_CALAMARES_VERSION-$NICOOS_CALAMARES_PKGREL.
Aggiorna i sorgenti (README/build.sh) oppure fissa NICOOS_AUR_SNAPSHOT a uno snapshot compatibile."
fi
info "packaging AUR: calamares $pkgver-$pkgrel"

for mod in "${SKIP_MODULES[@]}"; do
    grep -qw -- "$mod" "$SRC/PKGBUILD" || \
        warn "il PKGBUILD AUR non elenca più il modulo '$mod' tra quelli esclusi"
done

# ---------------------------------------------------------------------------
# 2. Sorgente upstream (sha256 dal PKGBUILD)
# ---------------------------------------------------------------------------
srcdir=$SRC/src
rm -rf -- "$srcdir"
mkdir -p -- "$srcdir"

for i in "${!source[@]}"; do
    entry=${source[$i]}
    sum=${sha256sums[$i]:-SKIP}
    if [[ $entry == *::* ]]; then
        filename=${entry%%::*}
        url=${entry#*::}
    else
        url=$entry
        filename=${url##*/}
    fi
    if [[ ! $url =~ ^(https?|ftp):// ]]; then
        warn "sorgente non scaricabile ignorato: $entry"
        continue
    fi
    cache=$B/cache/src/$filename
    if [[ ! -f $cache ]]; then
        log "download di $filename"
        curl -fL --retry 3 --connect-timeout 20 -o "$cache.part" "$url" \
            || die "download fallito: $url"
        mv -- "$cache.part" "$cache"
    fi
    if [[ $sum != SKIP ]]; then
        printf '%s  %s\n' "$sum" "$cache" | sha256sum -c - >/dev/null \
            || die "sha256 non corrispondente per $filename (atteso $sum)"
        info "sha256 verificato: $filename"
    fi
    bsdtar -xf "$cache" -C "$srcdir" || die "estrazione fallita: $filename"
done

upstream=$srcdir/calamares-$pkgver
[[ -d $upstream ]] || die "sorgente Calamares non trovato in $upstream"

# ---------------------------------------------------------------------------
# 3. Compilazione nel chroot
# ---------------------------------------------------------------------------
log "compilazione di Calamares $pkgver nel chroot"
build_script=$(cat <<'CHROOT'
set -euo pipefail
srcdir=$1
stage=$2
jobs=$3
cd "$srcdir"
rm -rf build "$stage"
mkdir -p "$stage"
cmake -B build -S "$srcdir" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_INSTALL_LIBDIR=lib \
    -DWITH_QT6=ON \
    -DINSTALL_CONFIG=ON \
    -DSKIP_MODULES="dracut dracutlukscfg dummycpp dummyprocess dummypython dummypythonqt initramfs initramfscfg interactiveterminal packagechooser packagechooserq services-openrc" \
    -DBUILD_TESTING=OFF \
    -DKDE_INSTALL_BINDIR=/usr/bin \
    -DKDE_INSTALL_SBINDIR=/usr/sbin \
    -DKDE_INSTALL_LIBDIR=/usr/lib \
    -DKDE_INSTALL_LIBEXECDIR=/usr/libexec \
    -DKDE_INSTALL_INCLUDEDIR=/usr/include \
    -DKDE_INSTALL_LOCALSTATEDIR=/var \
    -DKDE_INSTALL_SHAREDSTATEDIR=/usr/share \
    -DKDE_INSTALL_DATAROOTDIR=/usr/share \
    -DKDE_INSTALL_DATADIR=/usr/share \
    -DKDE_INSTALL_LOCALEDIR=/usr/share/locale \
    -DKDE_INSTALL_MANDIR=/usr/share/man \
    -DKDE_INSTALL_INFODIR=/usr/share/info \
    -DKDE_INSTALL_SYSCONFDIR=/etc \
    -Wno-dev
cmake --build build --parallel "$jobs"
DESTDIR="$stage" cmake --install build
CHROOT
)
"$PROJ/build/bin/arch-chroot" "$B/root" /bin/bash -c "$build_script" nicoos-calamares \
    "$upstream" "$STAGE" "$(nproc)"

# ---------------------------------------------------------------------------
# 4. Verifiche
# ---------------------------------------------------------------------------
[[ -x $STAGE/usr/bin/calamares ]] || die "Calamares non ha prodotto $STAGE/usr/bin/calamares"
[[ -d $STAGE/usr/lib/calamares/modules ]] || die "moduli Calamares assenti da $STAGE/usr/lib/calamares/modules"

# In Calamares 3.4 ogni modulo è una directory con module.desc (moduli C++
# con libcalamares_*_module_*.so, moduli Python con main.py): il controllo
# giusto è sulla directory, non sul nome della libreria.
required_modules=(welcome locale keyboard partition users summary mount unpackfs
    machineid fstab initcpiocfg initcpio displaymanager services-systemd grubcfg
    bootloader packages shellprocess finished localecfg hwclock umount)
missing_modules=()
for mod in "${required_modules[@]}"; do
    [[ -f $STAGE/usr/lib/calamares/modules/$mod/module.desc ]] \
        || missing_modules+=("$mod")
done
(( ${#missing_modules[@]} == 0 )) || \
    die "moduli Calamares richiesti da settings.conf mancanti: ${missing_modules[*]}"

compgen -G "$STAGE/usr/lib/libcalamares.so*"   >/dev/null \
    || die "libcalamares non installata in $STAGE/usr/lib"
compgen -G "$STAGE/usr/lib/libcalamaresui.so*" >/dev/null \
    || die "libcalamaresui non installata in $STAGE/usr/lib"

# Le librerie di Calamares sono nello stage, non nel chroot: ldd deve
# cercarle esplicitamente lì, altrimenti le segnala come mancanti.
if ldd_out=$(chroot_run "$B/root" /usr/bin/env LD_LIBRARY_PATH="$STAGE/usr/lib" \
        /usr/bin/ldd "$STAGE/usr/bin/calamares" 2>&1); then
    if grep -q 'not found' <<<"$ldd_out"; then
        grep 'not found' <<<"$ldd_out" >&2
        die "Calamares ha librerie condivise non risolte nel chroot"
    fi
else
    die "ldd di Calamares è fallito nel chroot"
fi

{
    printf 'NICOOS_CALAMARES_PKGVER=%q\n' "$pkgver"
    printf 'NICOOS_CALAMARES_PKGREL=%q\n' "$pkgrel"
    printf 'NICOOS_CALAMARES_PKGDESC=%q\n' "$pkgdesc"
    printf 'NICOOS_CALAMARES_URL=%q\n' "$url"
    printf 'NICOOS_CALAMARES_LICENSE=%q\n' "${license[0]}"
    printf 'NICOOS_CALAMARES_ARCH=%q\n' "x86_64"
    printf 'NICOOS_CALAMARES_DEPENDS=(%s)\n' "$(printf '%q ' "${depends[@]}")"
} >"$META"

log "Calamares $pkgver-$pkgrel in stage"
printf '    binario : %s\n' "$STAGE/usr/bin/calamares"
printf '    moduli  : %s\n' "$(find "$STAGE/usr/lib/calamares/modules" -mindepth 1 -maxdepth 1 -type d | wc -l)"
printf '    stage   : %s\n' "$STAGE"
