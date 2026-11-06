#!/usr/bin/env bash
# /root/customize_airootfs.sh — NicoOS 27 Extreme
#
# Viene eseguito dentro il chroot di archiso, dopo che tutti i pacchetti di
# packages.x86_64 sono stati installati e dopo che l'overlay airootfs del
# profilo è stato copiato.
#
# È la "esecuzione dello script personalizzato archiso nel chroot" richiesta
# da README_SHELL.md: la shell NicoOS viene compilata/installata da
# nicoos27-shell, la sua integrazione Lua viene messa dove Hyprland la
# cercherà con `require("nicoos-shell")`, e la sessione live viene collegata
# (SDDM -> Hyprland -> shell NicoOS).
#
# Tutto ciò che viene fatto qui è anche ciò che riceve il sistema
# *installato*: Calamares scompatta questo stesso root file system
# (settings.conf, unpackfs).
set -euo pipefail

log() { printf '\n[nicoos] %s\n' "$*"; }

export LC_ALL=C

# ---------------------------------------------------------------------------
# 1. La shell NicoOS (nicoos27-shell)
# ---------------------------------------------------------------------------
# La shell viene distribuita come vero pacchetto pacman, compilato dai
# sorgenti nicoos27-shell non modificati con gli stessi comandi documentati in
# README_SHELL.md (Release, prefisso /usr, Ninja). Installarla come pacchetto
# significa che anche il sistema *installato* la ha nel proprio database
# pacman, e che `pacman -Q nicoos-shell` funziona dopo il riavvio dal disco.
log "installazione del pacchetto della shell NicoOS"
shopt -s nullglob
pkgs=(/root/nicoos-shell-*.pkg.tar.zst)
shopt -u nullglob

if (( ${#pkgs[@]} )); then
    # La compilazione avviene dentro un namespace di mount non privilegiato
    # la cui radice è una semplice directory, quindi il controllo dello spazio
    # libero di pacman non riesce a risolvere il mount point di "/" e annulla
    # la transazione. Esegui questa sola transazione con una configurazione
    # temporanea che ha CheckSpace disabilitato; l'immagine mantiene
    # /etc/pacman.conf intatto.
    sed 's/^CheckSpace/#CheckSpace/' /etc/pacman.conf >/tmp/nicoos-pacman.conf
    pacman --config /tmp/nicoos-pacman.conf -U --noconfirm --needed "${pkgs[@]}"
    rm -f /tmp/nicoos-pacman.conf
elif [[ -d /root/nicoos27-shell ]]; then
    # Fallback: compila direttamente dai sorgenti, esattamente come dice il
    # README.
    log "nessun pacchetto trovato, compilazione di nicoos27-shell dai sorgenti"
    cd /root/nicoos27-shell
    cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -GNinja
    cmake --build build
    cmake --install build
    cd /
    mkdir -p /usr/share/nicoos-shell
    install -m 0644 /root/nicoos27-shell/packaging/nicoos-shell.lua /usr/share/nicoos-shell/nicoos-shell.lua
else
    echo "ERRORE FATALE: nicoos-shell non è disponibile nel chroot di compilazione" >&2
    exit 1
fi

[[ -x /usr/bin/nicoos-shell ]] || { echo "ERRORE FATALE: /usr/bin/nicoos-shell mancante" >&2; exit 1; }

# Ogni libreria condivisa usata dalla shell deve essere presente in questo
# root file system: il sistema installato riceve proprio questi file.
if ldd /usr/bin/nicoos-shell | grep -q "not found"; then
    echo "ERRORE FATALE: nicoos-shell ha dipendenze da librerie condivise non soddisfatte:" >&2
    ldd /usr/bin/nicoos-shell | grep "not found" >&2
    exit 1
fi
log "nicoos-shell: $(LD_LIBRARY_PATH= ldd /usr/bin/nicoos-shell | wc -l) librerie condivise risolte"

# ---------------------------------------------------------------------------
# 2. Integrazione con Hyprland (README_SHELL.md)
# ---------------------------------------------------------------------------
# "cp /usr/share/nicoos-shell/nicoos-shell.lua ~/.config/hypr/nicoos-shell.lua"
# e `require("nicoos-shell")` in fondo a hyprland.lua: entrambi sono già in
# /etc/skel (hyprland.lua termina con il require), e il file di integrazione
# viene installato accanto qui, per l'utente live e per ogni utente creato in
# seguito (useradd di Calamares copia /etc/skel).
log "collegamento della shell alla configurazione di Hyprland"
install -Dm0644 /usr/share/nicoos-shell/nicoos-shell.lua \
    /etc/skel/.config/hypr/nicoos-shell.lua

if ! grep -q 'require("nicoos-shell")' /etc/skel/.config/hypr/hyprland.lua; then
    printf '\nrequire("nicoos-shell")\n' >> /etc/skel/.config/hypr/hyprland.lua
fi

# Lo shim require("hl"), accanto a hyprland.lua e a livello di sistema.
install -Dm0644 /usr/share/nicoos/hypr/hl.lua /etc/skel/.config/hypr/hl.lua

# ---------------------------------------------------------------------------
# 3. L'utente live
# ---------------------------------------------------------------------------
# Un account dedicato `nicoos` con zsh, esattamente come quello che Calamares
# crea sul sistema installato. La ISO live aggiunge sudo senza password
# (/etc/sudoers.d/10-nicoos-live) e l'autologin di SDDM
# (/etc/sddm.conf.d/20-nicoos-live-autologin.conf); l'installer rimuove
# entrambi.
log "creazione dell'utente live 'nicoos'"
if ! id nicoos >/dev/null 2>&1; then
    useradd -m -u 1000 \
        -G wheel,video,audio,input,network,storage,power,rfkill,optical,lp \
        -s /usr/bin/zsh \
        -c "NicoOS" \
        nicoos
fi
printf 'nicoos:nicoos\n' | chpasswd

install -d -m 0750 -o nicoos -g nicoos /home/nicoos
cp -a /etc/skel/. /home/nicoos/
chown -R nicoos:nicoos /home/nicoos
chmod 0750 /home/nicoos

# Il "sudo senza password" dell'utente live è in sudoers.d/10-nicoos-live.
chmod 0440 /etc/sudoers.d/10-nicoos-live
chmod 0440 /etc/sudoers
chmod 0600 /etc/shadow /etc/gshadow
chmod 0644 /etc/passwd /etc/group

# Gli script di sessione NicoOS devono essere eseguibili qualunque fosse la
# umask.
chmod 0755 /usr/local/bin/nicoos-session /usr/local/bin/nicoos-emergency-shell \
           /usr/local/bin/nicoos-installer /usr/local/bin/nicoos-ensure-shell \
           /usr/local/bin/nicoos-target-prepare /usr/local/bin/nicoos-target-finalize

# root è bloccato di proposito: vi si accede via sudo da `nicoos`.
passwd -l root >/dev/null

# ---------------------------------------------------------------------------
# 4. Servizi
# ---------------------------------------------------------------------------
log "abilitazione dei servizi (SDDM, NetworkManager, bluetooth, audio)"
systemctl enable sddm.service
systemctl enable NetworkManager.service
systemctl enable bluetooth.service
systemctl enable power-profiles-daemon.service
systemctl enable fstrim.timer
systemctl set-default graphical.target

# SDDM è il display manager di riferimento. graphical.target avvia il desktop
# tramite questo alias (Wants=display-manager.service), e sddm.service ha solo
# Alias=, quindi l'alias deve esistere anche sul sistema installato.
ln -sf /usr/lib/systemd/system/sddm.service /etc/systemd/system/display-manager.service

# Il greeter che SDDM riesce davvero a eseguire. SDDM preferisce il greeter
# indicato nei metadati del tema (QtVersion, predefinito 5 =>
# /usr/bin/sddm-greeter, la build Qt5, che non può partire su questo sistema
# solo-Qt6 e lascia la macchina su una TTY). Fai fallire la compilazione se
# quella combinazione dovesse tornare.
if [[ ! -x /usr/bin/sddm-greeter-qt6 ]]; then
    echo "ERRORE FATALE: /usr/bin/sddm-greeter-qt6 mancante - SDDM non ha un greeter Qt6" >&2
    exit 1
fi
if ! grep -qx 'QtVersion=6' /usr/share/sddm/themes/nicoos/metadata.desktop; then
    echo "ERRORE FATALE: il tema SDDM di NicoOS deve dichiarare QtVersion=6 (metadata.desktop)" >&2
    exit 1
fi
if [[ -x /usr/bin/sddm-greeter ]] && ldd /usr/bin/sddm-greeter 2>/dev/null | grep -q 'not found'; then
    log "nota: il greeter Qt5 inutilizzato /usr/bin/sddm-greeter ha librerie non soddisfatte (previsto; il tema richiede Qt6)"
fi

# Log globale della Shell di emergenza. Di proprietà di root di proposito: un
# utente normale non può accodarvi righe, quindi la shell ripiega su
# ~/.cache/nicoos/emergency.log invece di stampare un errore di permessi.
install -Dm0644 /dev/null /var/log/nicoos-emergency.log

# ---------------------------------------------------------------------------
# 5. Immagine coordinata
# ---------------------------------------------------------------------------
log "installazione dell'immagine coordinata NicoOS"
install -d /usr/share/icons/hicolor/256x256/apps
install -m 0644 /usr/share/nicoos/branding/nicoos-logo.png \
    /usr/share/icons/hicolor/256x256/apps/nicoos-logo.png
cp -f /usr/share/nicoos/branding/nicoos-logo.png \
    /usr/share/icons/hicolor/256x256/apps/nicoos-shell.png
gtk-update-icon-cache -q -t -f /usr/share/icons/hicolor 2>/dev/null || true

# ---------------------------------------------------------------------------
# 6. Un pacman funzionante sulla ISO e sul sistema installato
# ---------------------------------------------------------------------------
# mkarchiso costruisce la ISO con -G (il keyring dell'host non viene copiato)
# e pacstrap copia la mirrorlist della macchina che compila. Entrambe le cose
# lascerebbero un sistema incapace di installare qualsiasi cosa — e la Shell di
# emergenza dice esplicitamente all'utente di eseguire `sudo pacman -Sy
# hyprland`. Quindi: mirror veri, keyring vero.
log "configurazione di pacman (mirror + keyring)"
cat > /etc/pacman.d/mirrorlist <<'MIRRORS'
## NicoOS 27 Extreme - mirror di Arch Linux
Server = https://geo.mirror.pkgbuild.com/$repo/os/$arch
Server = https://mirror.rackspace.com/archlinux/$repo/os/$arch
Server = https://mirrors.kernel.org/archlinux/$repo/os/$arch
MIRRORS

if [[ ! -s /etc/pacman.d/gnupg/pubring.gpg ]]; then
    pacman-key --init
    pacman-key --populate archlinux
fi

# ---------------------------------------------------------------------------
# 7. Lingua: solo italiano
# ---------------------------------------------------------------------------
# /etc/locale.gen contiene unicamente it_IT.UTF-8 e /etc/locale.conf imposta
# LANG=it_IT.UTF-8: genera la locale, così l'immagine live e Calamares sono in
# italiano.
log "generazione della locale italiana (it_IT.UTF-8)"
locale-gen

# ---------------------------------------------------------------------------
# 8. Pulizia
# ---------------------------------------------------------------------------
# Gli input di compilazione non devono finire sulla ISO (e /root è comunque
# escluso dall'installazione, ma la ISO live deve restare ordinata e piccola).
rm -rf /root/nicoos27-shell /root/nicoos-shell-*.pkg.tar.zst \
       /root/.cache /root/.gnupg/S.* 2>/dev/null || true
rm -rf /var/cache/pacman/pkg/* 2>/dev/null || true
rm -f /etc/machine-id
: > /etc/machine-id

# Un /etc/machine-id vuoto fa scattare systemd-firstboot.service (descrizione
# "Initial Setup"): un wizard su console che chiede locale, keymap, timezone e
# password di root prima del login. Sulla live non ha senso — è tutto già
# configurato e l'utente deve solo usare il sistema — e ritarda l'avvio
# grafico, quindi la si maschera. Il machine-id viene comunque generato da
# systemd a ogni avvio, come per qualsiasi supporto live.
log "disattivazione di systemd-firstboot (wizard di primo avvio)"
systemctl mask systemd-firstboot.service >/dev/null 2>&1 \
    || ln -sf /dev/null /etc/systemd/system/systemd-firstboot.service

log "personalizzazione dell'airootfs di nicoos completata"
