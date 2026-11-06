# NicoOS 27 Extreme

Una distribuzione Linux **basata su Arch Linux** con desktop **Hyprland su
Wayland** e una shell proprietaria scritta in C++20/Qt6. La ISO è un ambiente
live completo e avviabile che contiene già l'intero sistema: l'installer non
scarica pacchetti, scompatta lo stesso root file system sul disco.

NicoOS è distribuita **solo in italiano**: la locale generata è unicamente
`it_IT.UTF-8`, l'installer e i menu sono in italiano e la documentazione è in
italiano. La tastiera predefinita è `it`.

---

## Cosa c'è dentro

| Componente | Descrizione |
|---|---|
| **Hyprland** | Compositore Wayland con configurazione Lua pronta (`~/.config/hypr/hyprland.lua`): animazioni, blur, arrotondamenti, regole finestra, gesture e scorciatoie. |
| **nicoos-shell** | Shell del desktop nativa: pannello con lista finestre, launcher con ricerca fuzzy, centro di controllo (Wi-Fi, audio, bluetooth, luminosità, profili energetici), notifiche D-Bus, appunti, calendario, selettore sfondi, menu alimentazione, panoramica delle aree di lavoro. |
| **SDDM** | Display manager con tema Qt6 dedicato (`/usr/share/sddm/themes/nicoos`) e sessione Wayland `nicoos`. |
| **Calamares** | Installer grafico con strategia **offline**: partizionamento, utente, locale, bootloader e scompattamento del root file system. |
| **Shell di emergenza** | Se Hyprland non parte, la sessione consegna una TTY funzionante invece di rimandare l'utente al greeter in un ciclo. |
| **Software incluso** | Firmware (AMD/Intel/NVIDIA), Mesa/Vulkan, PipeWire, NetworkManager, BlueZ, strumenti per tutti i file system (ext4, btrfs, xfs, f2fs, ntfs, LUKS, LVM, RAID), GRUB, memtest. |

La shell è documentata in dettaglio in [`nicoos27-shell/README.md`](nicoos27-shell/README.md)
(indicato come `README_SHELL.md` nei commenti del codice) e le modifiche
versione per versione sono in [`nicoos27-shell/CHANGELOG.md`](nicoos27-shell/CHANGELOG.md).

---

## Requisiti

### Per compilare la ISO

- **Arch Linux** (o derivata) su `x86_64`.
- Pacchetto **`archiso`** (fornisce `mkarchiso`), `base-devel`, `cmake`,
  `ninja`, `pkgconf`, `python3`, `git`, `squashfs-tools`.
- **User namespace non privilegiati** abilitati: la build gira interamente
  senza root, dentro `unshare --map-auto --map-root-user`, con i wrapper
  `pacstrap`/`arch-chroot`/`mkpkg` in `.build/bin`. Nessun disco fisico viene
  mai toccato.
- **Rete** verso i mirror Arch (`geo.mirror.pkgbuild.com`,
  `mirror.rackspace.com`, `mirrors.kernel.org`) e spazio libero su disco
  sufficiente per chroot di compilazione, pacchetti e ISO.

### Per usare la ISO

- Macchina `x86_64` con BIOS legacy o UEFI, GPU supportata da Mesa (o
  virtualizzata, vedi [Macchine virtuali](#macchine-virtuali)).
- Nessun requisito di rete per installare: il sistema arriva interamente dalla
  ISO.

---

## Struttura del repository

```
.
├── build.sh                  # unico punto d'ingresso della build
├── archiso/                  # profilo archiso (profilo, pacchetti, overlay airootfs)
│   ├── profiledef.sh         # nome/versione ISO, modalità di avvio, permessi
│   ├── packages.x86_64       # software presente nella live *e* nel sistema installato
│   ├── pacman.conf.in        # template di pacman.conf per la build (repo [nicoos])
│   ├── efiboot/ syslinux/    # voci di avvio UEFI (systemd-boot) e BIOS (syslinux)
│   └── airootfs/             # /etc, /usr/local/bin, temi, configurazioni dell'immagine
│       ├── root/customize_airootfs.sh    # personalizzazione eseguita nel chroot
│       └── etc/calamares/                # configurazione dell'installer
├── nicoos27-shell/           # sorgenti della shell (C++20, Qt6, QML)
│   ├── src/ include/ resources/          # codice, QML, icone SVG
│   ├── config/nicoos.conf.example        # configurazione commentata della shell
│   └── packaging/            # PKGBUILD, voce .desktop, integrazione Hyprland
├── tools/make-branding.py    # genera sfondo, logo e splash (solo stdlib Python)
├── wallpapers/               # sfondi inclusi nella ISO (README dedicato)
├── out/                      # ISO prodotta e relativo .sha256
└── .build/                   # stato locale della build — NON versionato
```

> **Nota importante:** `.build/` è in `.gitignore`. Contiene il chroot della
> toolchain, i sorgenti scaricati, i pacchetti impacchettati e i passi
> `01-toolchain.sh` … `04-packages.sh` (che contengono percorsi assoluti della
> macchina di build). Su un clone nuovo `./build.sh` non può funzionare finché
> quell'infrastruttura non viene ricreata o adattata. I passi sono elencati in
> [`build.sh`](build.sh) e ogni script è autonomo.

---

## Compilare la ISO

```bash
./build.sh              # compila tutto ciò che manca, poi la ISO
./build.sh --clean      # ricompila da zero pacchetti e ISO
./build.sh --branding   # rigenera solo gli asset grafici
./build.sh --help
```

### Cosa fa la build, in ordine

1. **Identità visiva** — `tools/make-branding.py` disegna sfondo, logo,
   wallpaper della palette NicoOS e splash syslinux. È deterministico
   (nessuna dipendenza oltre alla libreria standard Python), quindi le ISO
   sono riproducibili.
2. **Toolchain** (`.build/steps/01-toolchain.sh`) — prepara una volta sola un
   chroot Arch con `base-devel`, CMake, Qt6 e le dipendenze dell'installer.
3. **Calamares 3.4.2** (`.build/steps/02-calamares.sh`) — compila l'installer
   dai sorgenti nel chroot e lo mette in stage.
4. **Shell** (`.build/steps/03-shell.sh`) — compila `nicoos27-shell` in
   `Release` con Ninja, prefisso `/usr`, e verifica che il binario usi
   davvero `layer-shell-qt`. Lo stage viene riusato: se un sorgente della
   shell è più recente del binario, la shell viene ricompilata
   automaticamente.
5. **Pacchetti** (`.build/steps/04-packages.sh`) — crea
   `calamares-3.4.2-2` e `nicoos-shell-1.0.0-1`, più il repository locale
   `[nicoos]` usato dal profilo archiso.
6. **Profilo** — genera `archiso/pacman.conf` dal template, copia
   `./wallpapers/*` in `/usr/share/nicoos/wallpapers` e inserisce il pacchetto
   della shell nel root file system live.
7. **ISO** — esegue `mkarchiso` (squashfs zstd, livello 12) e pubblica il
   risultato con nome definitivo.

### Output e verifica

```bash
out/NicoOS-27-Extreme-x86_64.iso
out/NicoOS-27-Extreme-x86_64.iso.sha256
```

```bash
cd out && sha256sum -c NicoOS-27-Extreme-x86_64.iso.sha256
```

La ISO non è versionata: in `out/` è presente solo il checksum dell'ultima
build pubblicata. La build seleziona l'immagine **più recente per data di
modifica**, non per nome, così una ISO vecchia rimasta nella cartella non può
essere pubblicata per errore.

---

## Provare la ISO e installare

Avvio: **BIOS legacy** (syslinux) o **UEFI** (systemd-boot), con le voci
“avvio normale” e “avvio dettagliato”; su UEFI è presente anche memtest.

### Sessione live

| Voce | Valore |
|---|---|
| Utente | `nicoos` |
| Password | `nicoos` |
| `sudo` | senza password (solo live) |
| `root` | bloccato di proposito, si usa `sudo` |
| Autologin | attivo su SDDM (solo live) |
| Tastiera | `it` |
| Lingua | italiano (`it_IT.UTF-8`) |

L'installer si trova nel menu applicazioni (**“Installa NicoOS 27 Extreme”**)
oppure si avvia da terminale con `nicoos-installer`.

### Installazione

- **Nessuna connessione a Internet è necessaria o tentata**: il root file
  system completo viene scompattato dalla ISO (`unpackfs`), quindi il desktop
  installato è bit per bit quello provato dal vivo.
- Il partizionamento **non ha una scelta preselezionata**: “Cancella disco”,
  “Installa accanto” e “Manuale” vanno scelti esplicitamente.
- File system disponibili: **ext4** (predefinito), **btrfs**, **xfs**, **f2fs**.
- Swap: nessuno, partizione piccola, swapfile o riuso di una esistente.
- Bootloader: **GRUB**, sia su UEFI sia su BIOS legacy (con fallback EFI).

### Cosa cambia nel sistema installato

`nicoos-target-prepare` e `nicoos-target-finalize` (eseguiti dentro la
destinazione da Calamares) rendono il sistema autonomo:

- rimossi autologin, utente live `nicoos` e `sudo` senza password;
- rimossi gli hook `archiso` dall'initramfs, che viene rigenerato e verificato
  con `lsinitcpio`;
- rimossi installer e pacchetti solo-live (`calamares`, `kpmcore`, `memtest`,
  `mkinitcpio-archiso`, …);
- kernel copiato in `/boot/vmlinuz-linux`, GRUB rigenerato senza parametri
  della ISO;
- SDDM con tema NicoOS e sessione `nicoos` predefinita, servizi attivi
  (NetworkManager, Bluetooth, PipeWire, power-profiles-daemon, fstrim);
- la shell resta un **pacchetto pacman tracciato**: `pacman -Q nicoos-shell`
  funziona dopo il riavvio dal disco;
- viene scritto `/var/lib/nicoos/install-info` con data, kernel e desktop.

La verifica finale **interrompe l'installazione** se Hyprland, SDDM, la shell,
il terminale `foot`, la shell di emergenza o la sessione Wayland mancano nella
destinazione.

---

## Desktop e scorciatoie

Catena di avvio:

```
SDDM → /usr/local/bin/nicoos-session → start-hyprland → nicoos-shell
```

`nicoos-session` prepara l'ambiente Wayland/Qt e, se Hyprland manca o termina
con errore, avvia `nicoos-emergency-shell`. `nicoos-ensure-shell` è la rete di
sicurezza che avvia la shell se l'hook di Hyprland non è scattato (mai una
seconda istanza).

| Tasto | Funzione |
|---|---|
| `SUPER+R` | Launcher |
| `SUPER+N` | Centro di controllo |
| `SUPER+Tab` | Panoramica multitasking (aree di lavoro + finestre) |
| `SUPER+X` | Menu alimentazione |
| `SUPER+W` / `SUPER+SHIFT+W` | Selettore sfondi / sfondo casuale |
| `SUPER+V` | Cronologia appunti |
| `SUPER+C` | Calendario |
| `SUPER+L` | Blocca schermo |
| `SUPER+SHIFT+N` | Non disturbare |
| `SUPER+SHIFT+R` | Avvia/ferma registrazione schermo |
| `Print` / `SHIFT+Print` | Screenshot area / schermo intero |
| `SUPER+Invio` | Terminale (`foot`) |
| `SUPER+Q` | Chiude la finestra attiva |

Le scorciatoie della shell vivono in `~/.config/hypr/nicoos-shell.lua` e
parlano con la shell tramite il socket di controllo
`$XDG_RUNTIME_DIR/nicoos/shell-control.sock` (`socat`). La shell **non cattura
tasti globali**: gli `bind` restano in Hyprland, dove gli utenti se li
aspettano.

### File di configurazione

| Percorso | Contenuto |
|---|---|
| `~/.config/nicoos/nicoos.conf` | Aspetto (colore d'accento, opacità, scala), pannello, comportamento, sfondo, blocco. Creato al primo avvio e ricaricato **a caldo**. |
| `~/.config/nicoos/hyprlock.conf` | Configurazione della schermata di blocco, rigenerata a ogni blocco finché non ne cancelli la prima riga. |
| `~/.config/hypr/hyprland.lua` | Configurazione Hyprland; termina con `require("nicoos-shell")`. |
| `~/.config/hypr/hyprlock.conf` | Usata solo come ripiego se la configurazione generata non basta. |

Esempio minimo di `nicoos.conf`:

```ini
[appearance]
accent_color=#00d9ff
panel_opacity=0.72
ui_scale=1.0

[panel]
position=top
height=34

[behavior]
animation_speed_ms=180
clock_format=HH:mm
launcher_shortcut=SUPER,R
workspace_wrap_around=true
```

La versione completa e commentata è in
[`nicoos27-shell/config/nicoos.conf.example`](nicoos27-shell/config/nicoos.conf.example).

---

## Personalizzare la distribuzione

- **Colori e pannello** — modifica `accent_color`, `panel_opacity`,
  `position=bottom` (per una barra delle applicazioni in stile Windows) e
  `height` in `nicoos.conf`. Il colore d'accento colora glifi, evidenziazioni,
  icone delle app fallback e schermata di blocco.
- **Sfondi** — metti i file in `./wallpapers/` (`png`, `jpg`/`jpeg`, `webp`,
  `bmp`; solo il primo livello). Ogni build li copia in
  `/usr/share/nicoos/wallpapers`, visibile nella live e nel sistema installato.
  Gli sfondi `nicoos-*.png` sono generati e vengono sovrascritti: usa altri
  nomi per i tuoi. Vedi [`wallpapers/README.md`](wallpapers/README.md).
- **Branding** — `tools/make-branding.py` rigenera sfondo, logo, wallpaper e
  splash; `./build.sh --branding` non ricompila il resto.
- **Pacchetti** — aggiungi voci a `archiso/packages.x86_64`: **tutto** ciò che
  è nel root file system live finisce anche nel sistema installato.
- **Configurazioni di sistema** — l'overlay in `archiso/airootfs/` viene
  copiato nell'immagine; `customize_airootfs.sh` crea l'utente live, abilita i
  servizi, configura mirror e keyring di pacman, genera la locale italiana e
  disattiva `systemd-firstboot`.
- **Shell** — modifica `nicoos27-shell/` e rilancia `./build.sh`: il controllo
  sulle date ricompila la shell solo se serve. Per una build fuori dall'ISO:
  `cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -GNinja && cmake --build build`.

---

## Macchine virtuali

Nelle VM la GPU emulata spesso non riesce a importare i buffer con cui Qt e
hyprlock disegnano, e Hyprland interrompe la connessione. NicoOS lo gestisce
da sola:

- `nicoos-session` usa `systemd-detect-virt` e, in una VM, imposta
  `LIBGL_ALWAYS_SOFTWARE=1` e `QT_QUICK_BACKEND=software` per la sessione;
- `main()` della shell rileva le VM e usa il renderer software di Qt Quick
  (`NICOOS_FORCE_GPU=1` per forzare comunque la GPU);
- hyprlock usa `LIBGL_ALWAYS_SOFTWARE=1` in auto (`[lock]
  software_render=auto|yes|no`).

Per un test in QEMU si può usare un disco `qcow2`: `*.qcow2` e la cartella
`.build/` sono ignorati da git, quindi i dischi di prova non finiscono nel
repository.

---

## Risoluzione dei problemi

| Sintomo | Cosa controllare |
|---|---|
| Hyprland non parte / sessione grafica assente | Vieni portato nella **Shell di emergenza**: `sudo pacman -S hyprland sddm nicoos-shell`, poi `systemctl status sddm` e `journalctl -b -u sddm`. |
| Log della sessione | `$XDG_RUNTIME_DIR/nicoos/session.log`, `/var/log/nicoos-emergency.log`, in ripiego `~/.cache/nicoos/emergency.log`. |
| La schermata di blocco si chiude o non si apre | `~/.cache/nicoos/lock.log`. Su Arch la causa più comune è un **aggiornamento parziale**: `sudo pacman -Syu` per riallineare `hyprlock` con `hyprutils`/`hyprlang`/`hyprgraphics`. |
| `pacman` non trova i pacchetti | L'immagine contiene mirror reali (`/etc/pacman.d/mirrorlist`) e un keyring inizializzato: `sudo pacman-key --init && sudo pacman-key --populate archlinux` solo se il keyring è stato alterato. |
| La shell non parte | Controlla che `~/.config/hypr/nicoos-shell.lua` esista e che `hyprland.lua` finisca con `require("nicoos-shell")`; `nicoos-ensure-shell` la avvia comunque entro 15 secondi. |
| Pannello flottante invece che ancorato | Manca `layer-shell-qt`: senza di esso la shell ripiega su finestre normali (la build avvisa in fase di configurazione). |

---

## Limiti noti

- **Anteprime delle finestre**: nella panoramica (`SUPER+Tab`)
  `WindowPreviewCapture` disegna un segnaposto tematizzato per app invece di
  una cattura reale. Servono i binding di `hyprland-toplevel-export-v1`
  generati con `wayland-scanner` (volutamente esclusi dall'albero portabile);
  il punto di innesto è documentato in
  `nicoos27-shell/include/nicoos/WindowPreviewCapture.hpp`.
- **Bluetooth**: `BluetoothClient` assume un solo adattatore
  (`/org/bluez/hci0`); i sistemi con più adattatori non vengono enumerati.
- **Wi-Fi**: l'elenco delle reti viene da `nmcli` (`WifiScanner`), mentre
  `NetworkManagerClient` descrive solo la connessione corrente.
- **Solo italiano**: non esistono altre locale nell'immagine, per scelta.
- **Installazione offline**: la ISO installa ciò che contiene; per software
  aggiuntivo si usa `pacman` dopo il primo avvio.

---

## Licenza

NicoOS 27 Extreme è distribuita sotto **GNU General Public License v3.0** —
vedi [`LICENSE`](LICENSE).
