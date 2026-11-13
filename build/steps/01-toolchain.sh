#!/usr/bin/env bash
#
# build/steps/01-toolchain.sh — prepara la sandbox di compilazione.
#
# Crea una volta sola (e in modo idempotente):
#   .build/tools/   mkarchiso, prelevato dal pacchetto archiso e scompattato
#                  qui: l'host non viene modificato e build.sh lo trova in
#                  PATH tramite $B/tools/usr/bin;
#   .build/root/   chroot Arch con base-devel, CMake/Ninja, Qt6,
#                  LayerShellQt e tutte le dipendenze di Calamares 3.4.2;
#   .build/keys/   keyring pacman della sandbox (copia, mai l'host in scrittura);
#   .build/pacman-build.conf   configurazione pacman con mirror espliciti.
#
# Tutto passa dai wrapper di build/bin, quindi gira dentro
# `unshare --map-auto --map-root-user`. Nessuna partizione e nessun disco
# fisico vengono letti o scritti: si installano pacchetti in una directory.
set -euo pipefail

# shellcheck source=../lib/common.sh
source "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../lib" && pwd)/common.sh"

log "verifica dell'ambiente di compilazione"

[[ $(uname -m) == x86_64 ]] || die "la pipeline supporta solo x86_64 (trovato $(uname -m))"

if [[ -r /etc/os-release ]]; then
    # shellcheck disable=SC1091
    . /etc/os-release
    [[ ${ID:-} == arch ]] || \
        warn "host non Arch Linux (ID=${ID:-sconosciuto}): la pipeline è pensata per Arch"
fi

userns_ok || die "i namespace utente non privilegiati non sono disponibili.
Servono a girare come root solo dentro la sandbox. Abilitali con:
  sudo sysctl -w kernel.unprivileged_userns_clone=1   # se presente
Verifica: unshare --map-auto --map-root-user id"

require_host_tools pacman bsdtar zstd curl find sort du \
    mksquashfs xorriso mkfs.fat mmd mcopy

ensure_build_dirs

# ---------------------------------------------------------------------------
# 1. Configurazione pacman della sandbox
# ---------------------------------------------------------------------------
# Mirror espliciti, come archiso/pacman.conf.in: la build non dipende dalla
# mirrorlist della macchina che compila né la modifica.
write_pacman_conf() {
    local siglevel=$1
    cat >"$NICOOS_PACMAN_CONF" <<EOF
#
# pacman.conf della sandbox di compilazione NicoOS (generato da
# build/steps/01-toolchain.sh — non modificare a mano).
#
[options]
HoldPkg     = pacman glibc
Architecture = auto
ParallelDownloads = 10
SigLevel    = ${siglevel}
LocalFileSigLevel = Optional
#CheckSpace

[core]
Server = https://geo.mirror.pkgbuild.com/\$repo/os/\$arch
Server = https://mirror.rackspace.com/archlinux/\$repo/os/\$arch

[extra]
Server = https://geo.mirror.pkgbuild.com/\$repo/os/\$arch
Server = https://mirror.rackspace.com/archlinux/\$repo/os/\$arch
EOF
}

# ---------------------------------------------------------------------------
# 2. Keyring della sandbox
# ---------------------------------------------------------------------------
prepare_keyring() {
    [[ -s $NICOOS_KEYRING/pubring.gpg ]] && return 0
    mkdir -p -- "$(dirname -- "$NICOOS_KEYRING")"
    rm -rf -- "$NICOOS_KEYRING"

    # 1. Solo la parte pubblica del keyring dell'host (pubring, trustdb,
    #    gpg.conf): sono leggibili anche senza root, a differenza di
    #    secring.gpg e private-keys-v1.d, che restano dove sono.
    if [[ -s /etc/pacman.d/gnupg/pubring.gpg ]]; then
        local f
        mkdir -p -- "$NICOOS_KEYRING"
        for f in pubring.gpg pubring.kbx trustdb.gpg gpg.conf; do
            [[ -f /etc/pacman.d/gnupg/$f ]] && \
                cp -f /etc/pacman.d/gnupg/$f "$NICOOS_KEYRING/" 2>/dev/null
        done
        if [[ -s $NICOOS_KEYRING/pubring.gpg ]]; then
            info "keyring pubblico copiato da /etc/pacman.d/gnupg"
            return 0
        fi
        rm -rf -- "$NICOOS_KEYRING"
    fi

    # 2. Keyring proprio generato nella sandbox (le chiavi archlinux arrivano
    #    da /usr/share/pacman/keyrings, nessuna rete). pacman-key vuole root:
    #    rootless lo esegue come root dentro il namespace utente.
    mkdir -p -- "$NICOOS_KEYRING"
    if have pacman-key \
        && rootless pacman-key --gpgdir "$NICOOS_KEYRING" --init >/dev/null 2>&1 \
        && rootless pacman-key --gpgdir "$NICOOS_KEYRING" --populate archlinux >/dev/null 2>&1; then
        info "keyring della sandbox generato con pacman-key"
        return 0
    fi
    rm -rf -- "$NICOOS_KEYRING"
    mkdir -p -- "$NICOOS_KEYRING"
    return 1
}

if prepare_keyring; then
    write_pacman_conf "Required DatabaseOptional"
else
    warn "impossibile preparare un keyring: i pacchetti saranno verificati solo via HTTPS"
    warn "configurazione pacman della sandbox con SigLevel = Optional TrustAll"
    rm -rf -- "$NICOOS_KEYRING"
    mkdir -p -- "$NICOOS_KEYRING"
    write_pacman_conf "Optional TrustAll"
fi

# ---------------------------------------------------------------------------
# 3. mkarchiso in .build/tools (nessuna installazione sull'host)
# ---------------------------------------------------------------------------
if [[ ! -x $B/tools/usr/bin/mkarchiso ]]; then
    log "prelievo di mkarchiso dal pacchetto archiso"
    mkdir -p -- "$B/cache/db"
    rootless pacman --config "$NICOOS_PACMAN_CONF" \
        --dbpath "$B/cache/db" \
        --cachedir "$NICOOS_PACMAN_CACHE" \
        --arch x86_64 --noconfirm -Sy
    # -Sw: solo download, -dd: niente risoluzione dipendenze. I programmi che
    # mkarchiso usa (mksquashfs, xorriso, mtools, …) sono già sull'host e
    # vengono verificati sopra: così in .build/tools non finisce una seconda
    # copia di libc/bash che coprirebbe quella di sistema in PATH.
    rootless pacman --config "$NICOOS_PACMAN_CONF" \
        --dbpath "$B/cache/db" \
        --cachedir "$NICOOS_PACMAN_CACHE" \
        --arch x86_64 --noconfirm -Swdd archiso

    shopt -s nullglob
    candidates=("$NICOOS_PACMAN_CACHE"/archiso-*.pkg.tar.zst)
    shopt -u nullglob
    (( ${#candidates[@]} )) || die "il pacchetto archiso non è stato scaricato in $NICOOS_PACMAN_CACHE"
    archiso_pkg=$(printf '%s\n' "${candidates[@]}" | sort -V | tail -n1)
    bsdtar -xf "$archiso_pkg" -C "$B/tools" || die "estrazione di $archiso_pkg fallita"
fi

[[ -x $B/tools/usr/bin/mkarchiso ]] || die "mkarchiso non disponibile in $B/tools/usr/bin"
# mkarchiso abilita extglob a runtime: il controllo sintattico deve fare lo
# stesso, altrimenti i suoi pattern !(*.cfg) sembrano errori di sintassi.
bash -O extglob -n "$B/tools/usr/bin/mkarchiso" \
    || die "mkarchiso estratto non è uno script valido"
info "mkarchiso: $("$B/tools/usr/bin/mkarchiso" -h 2>&1 | head -n1)"

# ---------------------------------------------------------------------------
# 4. Chroot di compilazione
# ---------------------------------------------------------------------------
# Gruppi e pacchetti verificati nei sorgenti:
#   * Calamares 3.4.2-2 (AUR): kcoreaddons, kpmcore, libpwquality,
#     qt6-declarative, qt6-svg, yaml-cpp + makedepends extra-cmake-modules,
#     libglvnd, ninja, qt6-tools, qt6-translations;
#   * nicoos27-shell (README.md): CMake, Ninja, pkgconf, Qt6 base/declarative/svg,
#     layer-shell-qt.
ROOT_GROUPS=(base base-devel)
ROOT_PKGS=(
    cmake ninja pkgconf gcc
    qt6-base qt6-declarative qt6-svg qt6-tools qt6-translations
    layer-shell-qt
    extra-cmake-modules libglvnd
    kcoreaddons kpmcore libpwquality yaml-cpp
    python
)

missing=()
for pkg in "${ROOT_PKGS[@]}"; do
    pkg_in_root "$B/root" "$pkg" || missing+=("$pkg")
done

if (( ${#missing[@]} )); then
    log "installazione della toolchain nel chroot $B/root"
    info "pacchetti mancanti: ${missing[*]}"
    "$PROJ/build/bin/pacstrap" -C "$NICOOS_PACMAN_CONF" "$B/root" \
        "${ROOT_GROUPS[@]}" "${ROOT_PKGS[@]}"
else
    info "toolchain già presente: nessuna installazione necessaria"
fi

# ---------------------------------------------------------------------------
# 5. Verifiche reali (non solo presenza di file)
# ---------------------------------------------------------------------------
[[ -x $B/root/usr/bin/pacman ]] || die "$B/root/usr/bin/pacman mancante: chroot non valido"
[[ -x $B/root/usr/bin/cmake ]]  || die "cmake mancante nel chroot"
[[ -x $B/root/usr/bin/ninja ]]  || die "ninja mancante nel chroot"
[[ -x $B/root/usr/bin/g++ ]]    || die "g++ mancante nel chroot"

for pkg in "${ROOT_PKGS[@]}"; do
    pkg_in_root "$B/root" "$pkg" || die "pacchetto assente dal chroot dopo l'installazione: $pkg"
done

# Prova di configurazione CMake: verifica che le dipendenze di build siano
# davvero trovabili (non basta che i pacchetti siano installati).
probe=$B/src/cmake-probe
mkdir -p -- "$probe"
cat >"$probe/CMakeLists.txt" <<'EOF'
cmake_minimum_required(VERSION 3.21)
project(nicoos-toolchain-probe LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Network DBus Svg)
find_package(LayerShellQt REQUIRED)
find_package(KF6CoreAddons REQUIRED)
find_package(KPMcore REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(PkgConfig REQUIRED)
EOF
chroot_run "$B/root" cmake -S "$probe" -B "$probe/build" -G Ninja -Wno-dev \
    >"$probe/configure.log" 2>&1 || {
    tail -n 40 "$probe/configure.log" >&2
    die "le dipendenze di compilazione non sono configurabili nel chroot (log: $probe/configure.log)"
}
info "probe CMake: Qt6, LayerShellQt, KF6CoreAddons, KPMcore, yaml-cpp trovati"

# Il root del chroot ha "/" in modalità 0555 (lo imposta il pacchetto
# filesystem): il marcatore va scritto accanto, non dentro.
{
    printf '# generato da 01-toolchain.sh — non modificare\n'
    printf 'date=%s\n' "$(date -Is)"
    printf 'groups=%s\n' "${ROOT_GROUPS[*]}"
    printf 'packages=%s\n' "${ROOT_PKGS[*]}"
} >"$B/toolchain.stamp"

log "toolchain pronta"
printf '    chroot : %s\n' "$B/root"
printf '    cmake  : %s\n' "$(chroot_run "$B/root" cmake --version | head -n1)"
printf '    g++    : %s\n' "$(chroot_run "$B/root" g++ --version | head -n1)"
printf '    pacman : %s\n' "$(chroot_run "$B/root" pacman --version | sed -n 's/.*Pacman v\([^ ]*\).*/\1/p' | head -n1)"
printf '    qt6    : %s\n' "$(chroot_run "$B/root" pkg-config --modversion Qt6Core)"
