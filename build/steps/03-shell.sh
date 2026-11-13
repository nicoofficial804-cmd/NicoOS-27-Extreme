#!/usr/bin/env bash
#
# build/steps/03-shell.sh — compila nicoos27-shell e la mette in stage.
#
# Segue nicoos27-shell/README.md alla lettera:
#   cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -GNinja
#   cmake --build build
#   DESTDIR=… cmake --install build        (prefisso /usr)
#
# La compilazione avviene nel chroot della toolchain (03 usa Qt6 e
# layer-shell-qt del chroot, non quelli dell'host). Lo stage finisce in
# .build/stage/nicoos-shell, il percorso che build.sh controlla prima di
# decidere se ricompilare.
#
# La shell deve compilare CON LayerShellQt: senza, il pannello diventa una
# normale finestra (fallback documentato). Qui la configurazione senza
# LayerShellQt è trattata come errore, perché il pannello ancorato è parte del
# prodotto e una build silenziosamente degradata non deve passare.
set -euo pipefail

# shellcheck source=../lib/common.sh
source "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../lib" && pwd)/common.sh"

SRC=$PROJ/nicoos27-shell
BUILD=$B/src/build-shell
STAGE=$B/stage/nicoos-shell

[[ -f $SRC/CMakeLists.txt ]] || die "sorgenti della shell assenti: $SRC/CMakeLists.txt"
[[ -x $B/root/usr/bin/cmake ]] || die "toolchain assente: esegui prima build/steps/01-toolchain.sh"

for pkg in qt6-base qt6-declarative qt6-svg layer-shell-qt cmake ninja gcc; do
    pkg_in_root "$B/root" "$pkg" || die "dipendenza della shell assente dal chroot: $pkg"
done

# ---------------------------------------------------------------------------
# 1. Configurazione e compilazione nel chroot
# ---------------------------------------------------------------------------
log "compilazione di nicoos27-shell (Release, Ninja, prefisso /usr)"
build_script=$(cat <<'CHROOT'
set -euo pipefail
src=$1
build=$2
stage=$3
jobs=$4
mkdir -p "$build"
cmake -S "$src" -B "$build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr
cmake --build "$build" --parallel "$jobs"
rm -rf "$stage"
DESTDIR="$stage" cmake --install "$build"
CHROOT
)
"$PROJ/build/bin/arch-chroot" "$B/root" /bin/bash -c "$build_script" nicoos-shell \
    "$SRC" "$BUILD" "$STAGE" "$(nproc)"

# ---------------------------------------------------------------------------
# 2. Verifiche
# ---------------------------------------------------------------------------
[[ -x $STAGE/usr/bin/nicoos-shell ]] || die "nicoos-shell non installato in $STAGE/usr/bin"

# NICOOS_LAYER_SHELL_AVAILABLE è una variabile CMake normale (non CACHE):
# la prova definitiva è il collegamento con libLayerShellQtInterface, che
# viene verificato qui sotto con ldd.
[[ -f $BUILD/CMakeCache.txt ]] || die "CMakeCache.txt non prodotto in $BUILD"
[[ -f $BUILD/build.ninja ]]      || die "build.ninja non prodotto in $BUILD"

if ldd_out=$(chroot_run "$B/root" /usr/bin/ldd "$STAGE/usr/bin/nicoos-shell" 2>&1); then
    if grep -q 'not found' <<<"$ldd_out"; then
        grep 'not found' <<<"$ldd_out" >&2
        die "nicoos-shell ha librerie condivise non risolte nel chroot"
    fi
    grep -q 'libLayerShellQtInterface' <<<"$ldd_out" || \
        die "nicoos-shell non è collegata a libLayerShellQtInterface: layer-shell non attivo"
else
    die "ldd di nicoos-shell è fallito nel chroot"
fi

# Ciò che customize_airootfs.sh e il desktop si aspettano dal pacchetto.
for f in usr/share/applications/nicoos-shell.desktop \
         usr/share/nicoos-shell/nicoos-shell.lua \
         usr/share/nicoos-shell/config/nicoos.conf.example; do
    [[ -f $STAGE/$f ]] || die "file atteso assente dallo stage: $f"
done

log "nicoos27-shell in stage"
printf '    binario : %s\n' "$STAGE/usr/bin/nicoos-shell"
printf '    layer   : layer-shell-qt attivo\n'
printf '    stage   : %s\n' "$STAGE"
