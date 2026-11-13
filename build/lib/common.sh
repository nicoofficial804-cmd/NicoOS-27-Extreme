#!/usr/bin/env bash
# shellcheck shell=bash
#
# build/lib/common.sh — percorsi, log e helper della sandbox di NicoOS.
#
# Ogni passo della pipeline (`01-toolchain.sh` … `04-packages.sh`) e ogni
# wrapper di `build/bin` importa questo file: definisce solo variabili e
# funzioni, non esegue nulla.
#
# Regole della sandbox (le stesse di build.sh):
#   * niente sudo e niente root sull'host;
#   * ogni operazione privilegiata gira in `unshare --map-auto --map-root-user`;
#   * si scrive solo dentro $PROJ (più le cache pacman in $B/cache);
#   * nessun comando tocca partizioni, dischi o periferiche fisiche.
#
# Variabili rispettate da build.sh e quindi da tutta la pipeline:
#   PROJ                  radice del repository
#   B                     $PROJ/.build
#   PROFILE               $PROJ/archiso
#   OUT                   $PROJ/out
#   PATH                  deve contenere build/bin e $B/tools/usr/bin
#   NICOOS_CHROOT_BINDS   percorsi da rendere visibili nel chroot

if [[ -n ${NICOOS_COMMON_SH:-} ]]; then
    return 0
fi
NICOOS_COMMON_SH=1

umask 0022

PROJ=${PROJ:-$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)}
B=${B:-$PROJ/.build}
PROFILE=${PROFILE:-$PROJ/archiso}
OUT=${OUT:-$PROJ/out}

NICOOS_CHROOT_BINDS=${NICOOS_CHROOT_BINDS:-$PROJ}
NICOOS_KEYRING=${NICOOS_KEYRING:-$B/keys/gnupg}
NICOOS_PACMAN_CACHE=${NICOOS_PACMAN_CACHE:-$B/cache/pacman}
NICOOS_PACMAN_CONF=${NICOOS_PACMAN_CONF:-$B/pacman-build.conf}

export PROJ B PROFILE OUT
export NICOOS_CHROOT_BINDS NICOOS_KEYRING NICOOS_PACMAN_CACHE NICOOS_PACMAN_CONF
export LC_ALL=C

# Lo stesso ordine di build.sh: i wrapper della sandbox vincono sui programmi
# di sistema, `.build/tools` fornisce mkarchiso senza installarlo sull'host.
export PATH="$PROJ/build/bin:$B/tools/usr/bin:$PATH"

# Versioni fissate dai sorgenti del repository:
#   AUR calamares          3.4.2-2  (nicoos27-shell/README + build.sh)
#   nicoos27-shell         1.0.0    (CMakeLists.txt project VERSION)
NICOOS_CALAMARES_VERSION=3.4.2
NICOOS_CALAMARES_PKGREL=2
NICOOS_SHELL_VERSION=1.0.0
NICOOS_SHELL_PKGREL=1

# ---------------------------------------------------------------------------
# Log
# ---------------------------------------------------------------------------
log()  { printf '\n\033[1;36m==>\033[0m \033[1m%s\033[0m\n' "$*"; }
info() { printf '    %s\n' "$*"; }
warn() { printf '\033[1;33mAVVISO:\033[0m %s\n' "$*" >&2; }
die()  { printf '\033[1;31mERRORE:\033[0m %s\n' "$*" >&2; exit 1; }

# ---------------------------------------------------------------------------
# Ambiente
# ---------------------------------------------------------------------------
have() { command -v "$1" >/dev/null 2>&1; }

# Namespace utente non privilegiato: è il modo in cui la pipeline ottiene i
# privilegi che servono a pacman e al chroot senza toccare l'host.
userns_ok() {
    unshare --map-auto --map-root-user true >/dev/null 2>&1
}

# Esegue un comando come root *dentro* il namespace utente.
rootless() {
    unshare --map-auto --map-root-user "$@"
}

# Come sopra, con un mount namespace privato: i mount fatti dal comando non
# sopravvivono alla sua uscita.
rootless_mount() {
    unshare --map-auto --map-root-user --mount --propagation private \
        --pid --fork --mount-proc "$@"
}

# Chroot della sandbox (wrapper build/bin/arch-chroot).
chroot_run() {
    "$B/bin/arch-chroot" "$@"
}

ensure_build_dirs() {
    mkdir -p -- "$B"/{tools,root,stage,packages,src,keys} \
               "$B"/cache/{pacman,src} \
               "$NICOOS_PACMAN_CACHE"
}

# Un pacchetto è installato in un root? Si guarda il database locale di
# pacman: nessun chroot, nessun privilegio.
pkg_in_root() {
    local root=$1 pkg=$2
    compgen -G "$root/var/lib/pacman/local/${pkg}-[0-9]*" >/dev/null
}

require_host_tools() {
    local missing=() t
    for t in "$@"; do
        have "$t" || missing+=("$t")
    done
    if (( ${#missing[@]} )); then
        die "strumenti mancanti sull'host: ${missing[*]}
Sono pacchetti dell'host (non vengono installati dalla pipeline):
  sudo pacman -S --needed base-devel arch-install-scripts libarchive zstd \\
       squashfs-tools libisoburn dosfstools mtools curl"
    fi
}

# Radice del progetto vista dal chroot: i passi costruiscono sempre percorsi
# assoluti dell'host che restano validi perché NICOOS_CHROOT_BINDS monta $PROJ
# allo stesso percorso dentro il chroot.
