#!/usr/bin/env bash
#
# build.sh — compila la ISO NicoOS 27 Extreme da questo repository.
#
#   ./build.sh              compila tutto ciò che manca, poi la ISO
#   ./build.sh --clean      ricompila da zero tutti i pacchetti e la ISO
#   ./build.sh --branding   rigenera solo gli asset grafici
#
# La compilazione è divisa in passi tutti riproducibili:
#   .build/steps/01-toolchain.sh   prepara il chroot di compilazione (una volta)
#   .build/steps/02-calamares.sh   compila Calamares 3.4.2 (AUR) nel chroot
#   .build/steps/03-shell.sh       compila nicoos27-shell (README_SHELL.md)
#   .build/steps/04-packages.sh    impacchetta entrambi nel repository [nicoos]
#   tools/make-branding.py         sfondo / logo / splash
#   mkarchiso                      assembla la ISO
#
# Il tutto gira senza privilegi: ogni passo che richiede root gira dentro
# `unshare --map-auto --map-root-user` con i wrapper pacstrap e arch-chroot
# compatibili con la sandbox in .build/bin. Nessun disco fisico viene mai
# toccato.
set -euo pipefail

PROJ=$(cd -- "$(dirname -- "$0")" && pwd)
B=$PROJ/.build
PROFILE=$PROJ/archiso
OUT=$PROJ/out

export PATH="$B/bin:$B/tools/usr/bin:$PATH"
export NICOOS_CHROOT_BINDS="$PROJ"
export LC_ALL=C

log()  { printf '\n\033[1;36m==>\033[0m \033[1m%s\033[0m\n' "$*"; }
fail() { printf '\033[1;31mERRORE:\033[0m %s\n' "$*" >&2; exit 1; }

CLEAN=0
BRANDING_ONLY=0
for arg in "$@"; do
    case "$arg" in
        --clean)     CLEAN=1 ;;
        --branding)  BRANDING_ONLY=1 ;;
        -h|--help)   sed -n '2,20p' "$0"; exit 0 ;;
        *)           fail "opzione sconosciuta: $arg" ;;
    esac
done

# ---------------------------------------------------------------------------
# 0. Identità visiva
# ---------------------------------------------------------------------------
log "generazione dell'identità visiva di NicoOS"
python3 "$PROJ/tools/make-branding.py"
if (( BRANDING_ONLY )); then
    exit 0
fi

# ---------------------------------------------------------------------------
# 1. Ambiente di compilazione e pacchetti
# ---------------------------------------------------------------------------
if (( CLEAN )); then
    log "pulizia degli artefatti di compilazione precedenti"
    rm -rf "$B/root" "$B/stage" "$B/packages" "$B/work" "$B/src/build-shell"
fi

[[ -x $B/root/usr/bin/pacman ]] || {
    log "preparazione del chroot di compilazione (toolchain)"
    "$B/steps/01-toolchain.sh"
}

[[ -x $B/stage/calamares/usr/bin/calamares ]] || {
    log "compilazione di Calamares 3.4.2 (AUR)"
    "$B/steps/02-calamares.sh"
}

# Lo stage della shell viene riusato tra una build e l'altra. Se un file dei
# sorgenti è più recente del binario già impacchettato va ricompilato: senza
# questo controllo le modifiche (per esempio a packaging/nicoos-shell.lua, che
# l'ISO installa in /usr/share/nicoos-shell) non arriverebbero mai.
shell_stage=$B/stage/nicoos-shell/usr/bin/nicoos-shell
if [[ ! -x $shell_stage ]] || \
   [[ -n $(find "$PROJ/nicoos27-shell" -newer "$shell_stage" -print -quit 2>/dev/null) ]]; then
    log "compilazione della shell NicoOS (nicoos27-shell)"
    "$B/steps/03-shell.sh"
fi

log "impacchettamento di calamares e nicoos-shell nel repository [nicoos]"
"$B/steps/04-packages.sh"

# ---------------------------------------------------------------------------
# 2. Preparazione del profilo archiso
# ---------------------------------------------------------------------------
log "preparazione del profilo archiso"
sed "s|@NICOOS_REPO@|$B/packages|g" "$PROFILE/pacman.conf.in" > "$PROFILE/pacman.conf"

# Tutto ciò che sta in ./wallpapers viene copiato in /usr/share/nicoos/wallpapers
# nel profilo: è una delle cartelle cercate da WallpaperManager, quindi il
# selettore di sfondi della shell li vede sia nella ISO live sia nel sistema
# installato (Calamares scompatta lo stesso root file system).
log "impacchettamento degli sfondi di ./wallpapers"
WP_SRC=$PROJ/wallpapers
WP_DST=$PROFILE/airootfs/usr/share/nicoos/wallpapers
rm -rf "$WP_DST"
mkdir -p "$WP_DST"
shopt -s nullglob nocaseglob
wallpapers=("$WP_SRC"/*.png "$WP_SRC"/*.jpg "$WP_SRC"/*.webp "$WP_SRC"/*.bmp)
shopt -u nullglob nocaseglob
if (( ${#wallpapers[@]} )); then
    cp -f -- "${wallpapers[@]}" "$WP_DST/"
fi
log "${#wallpapers[@]} sfondi inclusi in /usr/share/nicoos/wallpapers"

# La shell viene installata nel chroot della ISO da customize_airootfs.sh a
# partire da un pacchetto: è questo che la rende un pacchetto tracciato sul
# sistema installato.
mkdir -p "$PROFILE/airootfs/root"
rm -f "$PROFILE"/airootfs/root/nicoos-shell-*.pkg.tar.zst
cp -f "$B"/packages/nicoos-shell-*.pkg.tar.zst "$PROFILE/airootfs/root/"

# ---------------------------------------------------------------------------
# 3. Compilazione della ISO
# ---------------------------------------------------------------------------
log "compilazione della ISO con mkarchiso"
mkdir -p "$OUT"

# mkarchiso chiama il suo output nicoos-<versione>-x86_64.iso; immagini
# vecchie di esecuzioni precedenti renderebbero ambigua la ricerca della "ISO
# più recente" qui sotto.
rm -f "$OUT"/*.iso "$OUT"/*.iso.sha256

# Un'esecuzione precedente dentro il namespace di compilazione lascia file di
# proprietà di root; un semplice rm non li può rimuovere, quindi riprova con la
# stessa mappatura degli uid.
if [ -e "$B/work" ]; then
    rm -rf "$B/work" 2>/dev/null || \
        unshare --map-auto --map-root-user rm -rf "$B/work"
fi

unshare --map-auto --map-root-user --mount --pid --fork --mount-proc \
    env PATH="$B/bin:$B/tools/usr/bin:$PATH" NICOOS_CHROOT_BINDS="$PROJ" \
    mkarchiso -v -w "$B/work" -o "$OUT" "$PROFILE"

# ---------------------------------------------------------------------------
# 4. Pubblica l'artefatto con il suo nome definitivo
# ---------------------------------------------------------------------------
# Scegli l'immagine appena scritta: la più recente per data di modifica, non
# per nome (una ISO vecchia rimasta non deve mai essere pubblicata per errore).
built=$(find "$OUT" -maxdepth 1 -name '*.iso' -printf '%T@ %p\n' \
    | sort -n | tail -n1 | cut -d' ' -f2-)
built=${built#./}
[[ -n $built ]] || fail "mkarchiso non ha prodotto alcuna ISO in $OUT"

final=$OUT/NicoOS-27-Extreme-x86_64.iso
if [[ $(basename "$built") != $(basename "$final") ]]; then
    mv -f "$built" "$final"
    [[ -f "$built.sha256" ]] && rm -f "$built.sha256"
fi
(cd "$OUT" && sha256sum "$(basename "$final")" > "$(basename "$final").sha256")

log "completato"
ls -lh "$final" "$final.sha256"
printf '\nISO:      %s\nSHA256:   %s\n' "$final" "$OUT/$(basename "$final").sha256"
