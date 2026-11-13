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
  `pacstrap`/`arch-chroot`/`mkpkg` in `build/bin`. Nessun disco fisico viene
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
│       ├── etc/calamares/                # configurazione dell'installer
│       └── usr/share/plymouth/themes/nicoos/  # schermata di avvio (video, anello, logo)
├── nicoos27-shell/           # sorgenti della shell (C++20, Qt6, QML)
│   ├── src/ include/ resources/          # codice, QML, icone SVG
│   ├── config/nicoos.conf.example        # configurazione commentata della shell
│   └── packaging/            # PKGBUILD, voce .desktop, integrazione Hyprland
├── nicoos27extreme.png        # marchio ufficiale: fonte dei logo Plymouth/SDDM/Calamares
├── logo.png                   # banner orizzontale "NicoOS 27 EXTREME"
├── tools/make-branding.py    # genera sfondo, logo, splash e asset Plymouth
├── wallpapers/               # sfondi inclusi nella ISO (README dedicato)
├── build/                    # pipeline di build: steps/, lib/, bin/ (versionata)
├── out/                      # ISO prodotta e relativo .sha256
└── .build/                   # solo stato locale della build — NON versionato
```

> **Nota importante:** `.build/` è in `.gitignore` e contiene **solo stato**
> locale: chroot della toolchain, sorgenti scaricati, pacchetti, cache. La
> pipeline che lo usa è versionata in `build/` — `steps/01-toolchain.sh` …
> `04-packages.sh`, `lib/common.sh` e i wrapper `bin/pacstrap`, `bin/arch-chroot`,
> `bin/mkpkg` — e ricava i percorsi dalla propria posizione, quindi un clone
> nuovo compila subito con `./build.sh` senza adattamenti.

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
   wallpaper della palette NicoOS, splash syslinux e gli asset della
   schermata di avvio Plymouth (`dot.png` e il `logo.png` del tema, ricavato
   dal marchio `nicoos27extreme.png`). È deterministico (nessuna dipendenza oltre alla
   libreria standard Python), quindi le ISO sono riproducibili.
2. **Toolchain** (`build/steps/01-toolchain.sh`) — prepara una volta sola un
   chroot Arch con `base-devel`, CMake, Qt6 e le dipendenze dell'installer.
3. **Calamares 3.4.2** (`build/steps/02-calamares.sh`) — compila l'installer
   dai sorgenti nel chroot e lo mette in stage.
4. **Shell** (`build/steps/03-shell.sh`) — compila `nicoos27-shell` in
   `Release` con Ninja, prefisso `/usr`, e verifica che il binario usi
   davvero `layer-shell-qt`. Lo stage viene riusato: se un sorgente della
   shell è più recente del binario, la shell viene ricompilata
   automaticamente.
5. **Pacchetti** (`build/steps/04-packages.sh`) — crea
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
“avvio normale” (silenzioso, con la schermata Plymouth) e “avvio dettagliato”
(messaggi del kernel e di systemd, senza splash); su UEFI è presente anche
memtest. In entrambi i casi `Esc` durante l'avvio mostra i messaggi al posto
della schermata grafica.

### Sessione live

| Voce | Valore |
|---|---|
| Utente | `nicoos` |
| Password | `nicoos` |
| `sudo` | senza password (solo live) |
| `root` | bloccato di proposito, si usa `sudo` |
| Autologin | attivo e permanente su SDDM (solo live, `Relogin=true`) |
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
Plymouth → SDDM → /usr/local/bin/nicoos-session → start-hyprland → nicoos-shell
```

### Schermata di avvio

Dall'accensione al greeter si vede la schermata **Plymouth** di NicoOS. Prima
l'animazione di avvio: i fotogrammi di `splash.mp4` (estratti in
`splash0.png` … `splash39.png`) scorrono uno per volta a tutto schermo, a
**10 fps**: ognuno resta visibile **0,1 s** e i 40 fotogrammi coprono i 4 s del
video. Finito il video compare
lo sfondo nero con tre elementi in colonna, tutti centrati: in alto il **logo
del produttore del PC** (quello che il firmware mostra all'accensione), al
centro l'**anello di sei punti che ruota** — lo stesso indicatore di Windows
11, senza barra di avanzamento — e in basso il **logo NicoOS**. Tutto entra e
esce in dissolvenza, e nell'angolo in basso a sinistra scorre il nome del
servizio che si sta avviando.

La cadenza la dà il timer di Plymouth, non un contatore di refresh: il plugin
“script” riarma il timer con `1/refresh_rate` **prima** di eseguire il
callback, e `refresh_rate` si cambia a runtime con `Plymouth.SetRefreshRate`.
Il tema chiede quindi **10 Hz durante il video** (un tick = un fotogramma =
100 ms, con il lavoro del callback che non si somma all'intervallo) e torna a
**50 Hz dopo**, perché l'anello e le dissolvenze a 10 Hz sarebbero a scatti.
Plymouth non espone alcuna funzione di orologio agli script (`GetTime` non
esiste: le uniche API sono `Window`, `Sprite`, `Image`, `Math`,
`Plymouth.Set*`), quindi il timer è l'unico meccanismo a tempo disponibile:
garantisce l'intervallo tra le *richieste* di fotogramma, non l'istante in cui
il compositore le mostra (kernel, GPU e virtualizzazione possono ritardare la
presentazione). Se si cambia `VIDEO_FRAMES` in `nicoos.script` o `VIDEO_SECONDS`
in `nicoos-plymouth-fade-out`, vanno aggiornati entrambi: il generatore di
branding avvisa se il numero di fotogrammi non coincide.

La dissolvenza in uscita ha bisogno di un aiutino: Plymouth chiude la splash
nello stesso istante in cui riceve `quit`, senza regalare un fotogramma al
tema. Il drop-in `plymouth-quit.service.d/nicoos-fade-out.conf` chiama
`nicoos-plymouth-fade-out`, che manda lostato `__nicoos_fade__` (che `nicoos.script` riconosce come “inizia a
spegnerti”) e aspetta la fine del video più la dissolvenza: 4,4 s se la splash
è appena partita, i soli 0,4 s della dissolvenza se il video è già finito,
perché l'attesa si calcola dall'età di plymouthd. Così il video non viene
tagliato a metà e un boot più lento dell'animazione non si porta dietro 4 s di
schermo nero.

### Prestazioni

Durante il video il callback fa il minimo: due letture di `Window.Get*`, un
confronto e un `SetImage`; `Scale()` viene chiamato solo se il fotogramma non
ha già la misura dello schermo, e la sprite non si sposta perché sta ferma in
(0,0). Anello, dissolvenze e opacità restano spenti finché il video non è
finito; dopo, l'opacità si riapplica solo quando cambia e l'anello smette di
ricalcolarsi a dissolvenza completata. Le immagini scalate una volta sola
(logo, punti, logo OEM) stanno in `layout_update`, che gira solo al cambio di
risoluzione. Nessun file viene aperto durante l'animazione: i fotogrammi sono
già decodificati prima che parta il timer.

Gli asset sono dimensionati per l'initramfs: campionamento a 10 fps (i
fotogrammi intermedi di un sorgente a 30 fps non verrebbero mai mostrati) e
larghezza massima 1280 px, PNG RGB invece di RGBA.

| | prima | dopo |
|---|---|---|
| fotogrammi | 120 × 1920×1080 RGBA | 40 × 1280×720 RGB |
| peso su disco | 11 MB | 2,1 MB |
| decodifica di tutti i fotogrammi (CPU) | 9,4 s (78 ms ciascuno) | 1,2 s (30 ms ciascuno) |
| RAM in ARGB32 | ~950 MB | ~141 MB |

La decodifica è misurata con `ffmpeg -benchmark` (libpng, lo stesso decoder di
Plymouth) su un Ryzen 5 7520U. Quel lavoro non allunga l'intervallo tra i
fotogrammi: il timer viene riarmato prima del callback, quindi conta solo che
resti sotto i 100 ms.

Per rigenerare l'initramfs dopo una modifica al tema, sul sistema installato:

```bash
sudo mkinitcpio -P            # rigenera gli initramfs, tema compreso
```

Sulla ISO il tema entra nell'initramfs con l'hook `plymouth`; `./build.sh`
rigenera gli asset, ricostruisce l'immagine e quindi l'initramfs.

Il tema sta in `archiso/airootfs/usr/share/plymouth/themes/nicoos/`:
`nicoos.script` descrive l'animazione in frazioni della risoluzione (identica da
1024×768 a 4K), `dot.png` e `logo.png` arrivano da `tools/make-branding.py`: il
logo che compare sotto l'anello è il marchio ufficiale `nicoos27extreme.png`,
lo stesso disegno di SDDM e Calamares — non il banner `logo.png` della radice.

Il logo del produttore non è un file del tema: si legge a runtime da
`/sys/firmware/acpi/bgrt/image`, la tabella **BGRT** che i firmware UEFI
espongono con il marchio mostrato al POST. Se il firmware non la pubblica
(BIOS legacy, molte schede virtuali) l'immagine manca e il logo in alto
semplicemente non compare: anello e wordmark restano al loro posto. Il percorso
in `nicoos.script` è relativo alla cartella del tema perché `Image()` cerca
sempre dentro `ImageDir`.

La riga dei servizi non richiede nulla di nostro: **systemd** (PID 1) manda a
Plymouth un aggiornamento per ogni unità che parte, e il tema lo disegna
nell'angolo con `Plymouth.SetUpdateStatusFunction`. Si vede nel log di
plymouthd attivando `plymouth.debug` sulla riga di comando: `updating status to
'systemd-logind.service'`, con `connection is from toplevel init process`
accanto. Se la splash non c'è, systemd non manda niente e la riga resta vuota.

Il tema viene messo **dentro l'initramfs** dall'hook `plymouth` di mkinitcpio:
sull'ISO lo aggiunge `/etc/mkinitcpio.conf.d/archiso.conf` (e
`customize_airootfs.sh` seleziona il tema con `plymouth-set-default-theme` e
rigenera l'initramfs), sul sistema installato Calamares riscrive gli hook e
aggiunge `plymouth` prima di `encrypt`, così anche la password di un disco
cifrato si digita nella schermata di avvio. La splash compare **solo** con `splash` nella riga di
comando del kernel: le voci normali dei menu la passano, quella dettagliata no.

`nicoos-session` prepara l'ambiente Wayland/Qt e, se Hyprland manca o termina
con errore, chiede a `nicoos-emergency-console.path` di aprire
`nicoos-emergency-shell` su una VT di testo libera (`tty4`). Serve un servizio
di root perché SDDM lascia la VT della sessione in `KD_GRAPHICS` e manda lo
stdout della sessione su `/dev/null`: una shell avviata lì dentro resterebbe
invisibile, cioè un login che sembra bloccato. `Ctrl+Alt+F1` torna al greeter.
`nicoos-ensure-shell` è la rete di sicurezza che avvia la shell se l'hook di
Hyprland non è scattato (mai una seconda istanza).

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
accent_color=#e3272b
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
workspace_count=10
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
| Hyprland non parte / sessione grafica assente | Vieni portato nella **Shell di emergenza** sulla VT di testo `tty4` (`Ctrl+Alt+F1` torna al greeter): `sudo pacman -S hyprland sddm nicoos-shell`, poi `systemctl status sddm` e `journalctl -b -u sddm`. Se non compare, controlla `systemctl status nicoos-emergency-console.path` e usa `Ctrl+Alt+F2`. |
| Log della sessione | `$XDG_RUNTIME_DIR/nicoos/session.log`, `/var/log/nicoos-emergency.log`, in ripiego `~/.cache/nicoos/emergency.log`. Il motivo dell'ultimo fallimento è in `~/.cache/nicoos/last-failure` (lo stampa anche la Shell di emergenza). |
| La schermata di blocco si chiude o non si apre | `~/.cache/nicoos/lock.log`. Su Arch la causa più comune è un **aggiornamento parziale**: `sudo pacman -Syu` per riallineare `hyprlock` con `hyprutils`/`hyprlang`/`hyprgraphics`. |
| `pacman` non trova i pacchetti | L'immagine contiene mirror reali (`/etc/pacman.d/mirrorlist`) e un keyring inizializzato: `sudo pacman-key --init && sudo pacman-key --populate archlinux` solo se il keyring è stato alterato. |
| Schermata di avvio nera, bloccata o con il tema sbagliato | Usa la voce **“avvio dettagliato”** del menu (esclude la splash) oppure aggiungi `plymouth.enable=0 disablehooks=plymouth` alla riga di comando del kernel. Il tema deve stare dentro l'initramfs: `lsinitcpio /boot/initramfs-linux.img \| grep themes/nicoos`; se manca, `sudo mkinitcpio -P`. |
| Manca il logo del produttore in alto | Il firmware non espone BGRT: `ls /sys/firmware/acpi/bgrt/` deve contenere `image`. Sui sistemi avviati in BIOS legacy (o su molte macchine virtuali) non c'è, ed è normale. |
| La riga dei servizi non compare | La aggiorna systemd; se il tema è quello giusto ma resta vuota, prova ad avviare con `plymouth.debug` e cerca `updating status to` in `/var/log/plymouth-debug.log`. |
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
