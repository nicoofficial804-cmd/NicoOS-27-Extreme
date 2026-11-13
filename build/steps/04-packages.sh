#!/usr/bin/env bash
#
# build/steps/04-packages.sh — impacchetta calamares e nicoos-shell.
#
# Produce in .build/packages:
#   calamares-3.4.2-2-x86_64.pkg.tar.zst
#   nicoos-shell-1.0.0-1-x86_64.pkg.tar.zst
# e il repository locale [nicoos] (nicoos.db / nicoos.files) che
# archiso/pacman.conf.in usa tramite file://.
#
# I metadati di calamares vengono dal PKGBUILD AUR usato da 02; quelli di
# nicoos-shell dai sorgenti del repository: il PKGBUILD in
# nicoos27-shell/packaging/ non è interpretabile da bash (un apostrofo non
# sfuggito nella voce optdepends 'slurp: …dell'area…' chiude la stringa), e
# comunque build.sh impacchetta l'albero già installato, non un makepkg.
# I `depends` sono quindi quelli del pacchetto originale generato dalla build
# precedente (archiso/airootfs/root/nicoos-shell-1.0.0-1-x86_64.pkg.tar.zst),
# verificati contro packages.x86_64.
set -euo pipefail

# shellcheck source=../lib/common.sh
source "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../lib" && pwd)/common.sh"

STAGE_CAL=$B/stage/calamares
STAGE_SHELL=$B/stage/nicoos-shell
PKGDIR=$B/packages

[[ -x $STAGE_CAL/usr/bin/calamares ]] || die "Calamares non è in stage: esegui build/steps/02-calamares.sh"
[[ -x $STAGE_SHELL/usr/bin/nicoos-shell ]] || die "nicoos-shell non è in stage: esegui build/steps/03-shell.sh"

mkdir -p -- "$PKGDIR"

# ---------------------------------------------------------------------------
# 1. Metadati di Calamares (dal PKGBUILD AUR, tramite 02)
# ---------------------------------------------------------------------------
META=$B/src/calamares/.nicoos-meta
if [[ -f $META ]]; then
    # shellcheck disable=SC1090
    source "$META"
    cal_version=$NICOOS_CALAMARES_PKGVER
    cal_rel=$NICOOS_CALAMARES_PKGREL
    cal_desc=$NICOOS_CALAMARES_PKGDESC
    cal_url=$NICOOS_CALAMARES_URL
    cal_license=$NICOOS_CALAMARES_LICENSE
    cal_depends=("${NICOOS_CALAMARES_DEPENDS[@]}")
else
    warn "metadati AUR assenti ($META): uso i valori verificati del PKGBUILD calamares 3.4.2-2"
    cal_version=$NICOOS_CALAMARES_VERSION
    cal_rel=$NICOOS_CALAMARES_PKGREL
    cal_desc="Distribution-independent installer framework"
    cal_url="https://codeberg.org/Calamares/calamares"
    cal_license="GPL-3.0-or-later"
    cal_depends=(kcoreaddons kpmcore libpwquality qt6-declarative qt6-svg yaml-cpp)
fi

# ---------------------------------------------------------------------------
# 2. Metadati di nicoos-shell
# ---------------------------------------------------------------------------
shell_version=$(sed -n 's/^project(nicoos27-shell VERSION \([0-9.]*\).*/\1/p' \
    "$PROJ/nicoos27-shell/CMakeLists.txt")
[[ -n $shell_version ]] || shell_version=$NICOOS_SHELL_VERSION
[[ $shell_version == "$NICOOS_SHELL_VERSION" ]] || \
    warn "CMakeLists dichiara nicoos27-shell $shell_version (atteso $NICOOS_SHELL_VERSION)"

# depends del pacchetto nicoos-shell-1.0.0-1 originale (accesso diretto,
# opzionali: entrambi presenti in packages.x86_64).
shell_depends=(qt6-base qt6-declarative qt6-svg layer-shell-qt socat swaybg
    hyprlock grim slurp wl-clipboard cliphist libnotify playerctl
    networkmanager pipewire-pulse bluez)
shell_optdepends=(
    "brightnessctl: ripiego per la retroilluminazione"
    "power-profiles-daemon: profili energetici"
    "wf-recorder: registrazione dello schermo"
    "hyprsunset: luce notturna"
    "papirus-icon-theme: icone delle app"
)

# ---------------------------------------------------------------------------
# 3. Costruzione dei pacchetti
# ---------------------------------------------------------------------------
rm -f -- "$PKGDIR"/calamares-*.pkg.tar.zst \
         "$PKGDIR"/nicoos-shell-*.pkg.tar.zst \
         "$PKGDIR"/nicoos.db* "$PKGDIR"/nicoos.files*

log "creazione del pacchetto calamares $cal_version-$cal_rel"
cal_args=(--name calamares --version "$cal_version" --release "$cal_rel"
    --desc "$cal_desc" --url "$cal_url" --license "$cal_license" --arch x86_64
    --stage "$STAGE_CAL" --out "$PKGDIR")
for dep in "${cal_depends[@]}"; do
    cal_args+=(--depends "$dep")
done
cal_pkg=$("$PROJ/build/bin/mkpkg" "${cal_args[@]}" | tail -n1)

log "creazione del pacchetto nicoos-shell $shell_version-$NICOOS_SHELL_PKGREL"
shell_args=(--name nicoos-shell --version "$shell_version"
    --release "$NICOOS_SHELL_PKGREL" --arch x86_64
    --desc "NicoOS 27 Extreme desktop shell for Hyprland (panel, launcher, control center, notifications)"
    --url "https://nicoos.dev" --license MIT
    --stage "$STAGE_SHELL" --out "$PKGDIR")
for dep in "${shell_depends[@]}"; do
    shell_args+=(--depends "$dep")
done
for dep in "${shell_optdepends[@]}"; do
    shell_args+=(--optdepends "$dep")
done
shell_pkg=$("$PROJ/build/bin/mkpkg" "${shell_args[@]}" | tail -n1)

[[ -f $cal_pkg ]]   || die "pacchetto calamares non creato"
[[ -f $shell_pkg ]] || die "pacchetto nicoos-shell non creato"

# build.sh copia "$B/packages/nicoos-shell-*.pkg.tar.zst" nel root file system
# live: deve esserci esattamente un pacchetto della shell.
shopt -s nullglob
shell_pkgs=("$PKGDIR"/nicoos-shell-*.pkg.tar.zst)
shopt -u nullglob
(( ${#shell_pkgs[@]} == 1 )) || \
    die "atteso un solo pacchetto nicoos-shell in $PKGDIR, trovati ${#shell_pkgs[@]}"

# ---------------------------------------------------------------------------
# 4. Repository locale [nicoos]
# ---------------------------------------------------------------------------
log "aggiornamento del repository locale [nicoos]"
repo_db=$PKGDIR/nicoos.db.tar.gz
repo-add --quiet --remove "$repo_db" "$cal_pkg" "$shell_pkg" \
    || die "repo-add è fallito"
for f in nicoos.db nicoos.files nicoos.db.tar.gz nicoos.files.tar.gz; do
    [[ -e $PKGDIR/$f ]] || die "indice del repository mancante: $f"
done

# ---------------------------------------------------------------------------
# 5. Verifica: pacman deve leggere il repository e i metadati
# ---------------------------------------------------------------------------
check_root=$B/.repo-check
rm -rf -- "$check_root"
mkdir -p -- "$check_root/var/lib/pacman"
cat >"$check_root/pacman.conf" <<EOF
[options]
Architecture = x86_64
SigLevel = Optional TrustAll
[nicoos]
Server = file://$PKGDIR
EOF

rootless pacman --root "$check_root" --dbpath "$check_root/var/lib/pacman" \
    --config "$check_root/pacman.conf" --noconfirm -Sy >/dev/null \
    || die "pacman non riesce a sincronizzare il repository [nicoos]"

for spec in "calamares:$cal_version-$cal_rel" "nicoos-shell:$shell_version-$NICOOS_SHELL_PKGREL"; do
    name=${spec%%:*}
    want=${spec#*:}
    got=$(rootless pacman --root "$check_root" --dbpath "$check_root/var/lib/pacman" \
        --config "$check_root/pacman.conf" -Si "$name" 2>/dev/null \
        | sed -n 's/^Version[[:space:]]*:[[:space:]]*//p')
    [[ $got == "$want" ]] || die "nel repository [nicoos] $name ha versione '$got', attesa '$want'"
    info "repository: $name $got"
done

# Le dipendenze devono esistere nel root file system live, altrimenti pacman
# installerebbe un sistema con pacchetti irrisolti.
declare -A in_iso=()
while IFS= read -r line; do
    line=${line%%#*}
    line=${line//[[:space:]]/}
    [[ -n $line ]] && in_iso[$line]=1
done <"$PROFILE/packages.x86_64"

for dep in "${cal_depends[@]}" "${shell_depends[@]}"; do
    [[ -n ${in_iso[$dep]:-} ]] || \
        warn "dipendenza '$dep' non è elencata in archiso/packages.x86_64"
done

# Contenuto dei pacchetti: niente stage vuoti o file fuori posto.
bsdtar -tf "$cal_pkg" | grep -qx 'usr/bin/calamares' \
    || die "usr/bin/calamares non è dentro $cal_pkg"
bsdtar -tf "$shell_pkg" | grep -qx 'usr/bin/nicoos-shell' \
    || die "usr/bin/nicoos-shell non è dentro $shell_pkg"

log "repository [nicoos] pronto"
printf '    %s\n' "$(basename -- "$cal_pkg")"
printf '    %s\n' "$(basename -- "$shell_pkg")"
printf '    %s\n' "$PKGDIR"
