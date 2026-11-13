# NicoOS 27 Extreme — Shell del desktop

Una shell desktop nativa in C++20/Qt6 per **Hyprland su Wayland**. Niente
Electron, niente interfaccia nel browser, nessuna dipendenza da X11. Pannello,
launcher, centro di controllo, panoramica delle aree di lavoro e un demone
delle notifiche: tutto comunica con Hyprland tramite il suo IPC e con il resto
del sistema tramite D-Bus / sysfs / `/proc`, senza mai sostituire o incapsulare
il compositore.

## Struttura del progetto

```
nicoos27-shell/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── shell/            # ShellApplication (radice della composizione), LayerSurfaceWindow
│   ├── panel/             # Barra superiore: orologio, punti delle aree di lavoro, glifi di stato
│   ├── launcher/           # Indicizzazione app (.desktop), ricerca fuzzy, avvio
│   ├── controlcenter/      # Client NetworkManager / audio / luminosità / bluetooth
│   ├── notifications/      # Demone org.freedesktop.Notifications + pila degli avvisi
│   ├── workspaces/         # Panoramica di aree di lavoro e finestre di Hyprland
│   ├── hyprland/           # HyprlandIPC + HyprlandEventListener (l'unico livello IPC)
│   └── system/             # SystemMonitor (/proc, /sys), ConfigManager
├── include/nicoos/         # Header pubblici corrispondenti
├── resources/
│   ├── resources.qrc
│   └── qml/                # Panel.qml, Launcher.qml, ControlCenter.qml,
│                             NotificationStack.qml, WorkspaceOverview.qml
├── config/
│   └── nicoos.conf.example
└── packaging/
    ├── PKGBUILD
    ├── nicoos-shell.desktop
    └── hyprland-nicoos.conf
```

## Funzionalità

| | |
|---|---|
| **Aree di lavoro** | Le aree 1..N (`[behavior] workspace_count`, default 10) restano sempre presenti nel pannello e nella panoramica anche quando sono vuote — Hyprland distrugge un'area vuota appena la si lascia. Non sono segnaposto: il cambio area, il clic sul puntino e il trascinamento di una finestra creano l'area in Hyprland al primo uso, e ogni area mostra le proprie finestre e il proprio stato attivo. |
| **Lista finestre** | Pulsanti in stile Windows nella barra: icona + titolo, clic per dare il focus, clic su quella attiva per minimizzare, clic su una attenuata per ripristinare, clic centrale o × per chiudere. Animazioni fluide in aggiunta e rimozione. Imposta `[panel] position=bottom` per una vera barra delle applicazioni. |
| **Blocco** | Avvia `hyprlock` con una configurazione generata dalla shell (`~/.config/nicoos/hyprlock.conf`: sfondo corrente sfocato, pannello in vetro, orologio, campo password nel colore d'accento), con ripiego su `swaylock`/`gtklock`. Se il blocco va in crash (ad esempio con una password errata) viene riavviato automaticamente. Log: `~/.cache/nicoos/lock.log`. `SUPER+L` |
| **Menu alimentazione** | `SUPER+X`: sospendi (blocca prima), blocca, esci (uwsm → `exit` di Hyprland → `hyprctl` → `loginctl` → `pkill`, salendo di livello solo se Hyprland è ancora vivo), riavvia, spegni. Esci/riavvia/spegni chiedono un secondo clic. Tasti: frecce + Invio, oppure 1-5. |
| **Selettore di sfondi** | `SUPER+W`: griglia di anteprime, scelta casuale (`SUPER+SHIFT+W`), ricordata tra un accesso e l'altro. Richiede `swaybg` (o `swww`). |
| 1. Non disturbare | Silenzia i popup (le notifiche critiche restano visibili). `SUPER+SHIFT+N` |
| 2. Luce notturna | `hyprsunset` o `wlsunset`, 4000 K. |
| 3. Screenshot | Area o schermo intero con `grim`/`slurp`; salvati in Immagini/Screenshot e copiati negli appunti. `Print` / `Shift+Print` |
| 4. Cronologia appunti | `SUPER+V`: cerca, clic per ricopiare, elimina voci. Richiede `cliphist` + `wl-clipboard`. |
| 5. Controlli multimediali | In riproduzione + precedente/play/successivo per qualsiasi lettore MPRIS (`playerctl`). |
| 6. Profili energetici | Ciclo risparmio energetico / bilanciato / prestazioni (`power-profiles-daemon`). |
| 7. Reti Wi-Fi | Elenco, connessione, richiesta password (`nmcli`). |
| 8. Caffeina | Mantiene lo schermo attivo / blocca la sospensione (`systemd-inhibit`). |
| 9. Calendario | Clic sull'orologio o `SUPER+C`. |
| Rotellina del mouse | Sui punti delle aree di lavoro: area precedente/successiva (rispetta `workspace_wrap_around`). Sulle icone di stato: volume ±5%. |
| 10. Registrazione schermo | `wf-recorder`, salvata in Video/Registrazioni; il video si apre automaticamente quando interrompi. `SUPER+SHIFT+R` |

## Funzionalità "Extreme"

Due componenti vanno oltre una shell in stile waybar di base:

- **Icone.** Ogni glifo di stato (rete, audio, batteria, bluetooth,
  luminosità, alimentazione/blocco/riavvio/logout, ricerca, CPU, RAM) è un
  vero SVG in `resources/icons/system/`, colorato a runtime da `IconGlyph.qml`
  tramite la colorizzazione di `MultiEffect`: così un singolo file di icona
  funziona in qualsiasi colore di stato (inattivo/avviso/accento) senza
  varianti per colore. `AppIconProvider` (registrato come
  `image://nicoosicon/...` nel motore QML di ogni finestra della shell) risolve
  prima l'icona di ogni voce del launcher dal tema di icone installato nel
  sistema, e ripiega su un esagono neon in vetro generato con l'iniziale
  dell'app — colorato da un hash deterministico del nome dell'app mescolato al
  tuo colore d'accento — per ogni app che il tema di icone corrente non copre.
  Nel launcher non compare mai una tessera vuota.

- **Panoramica multitasking (vista "Extreme", `SUPER+Tab`).** Una sola
  superficie combinata invece di viste separate "cambia area di lavoro" e
  "vedi le mie finestre": ogni area di lavoro viene disegnata come una scheda
  che occupa gran parte dello schermo, e ogni scheda mostra una griglia di
  anteprime quasi dal vivo delle proprie finestre. Le finestre possono essere
  trascinate da una scheda all'altra (`WindowListModel::moveWindowToWorkspace`,
  che pilota `hyprctl dispatch movetoworkspacesilent`), un'unica casella di
  ricerca in alto filtra tutte le finestre aperte per titolo o classe dell'app
  nell'intera sessione, e il doppio clic su un'anteprima dà il focus a quella
  finestra e salta alla sua area di lavoro. Le anteprime sono prodotte da
  `WindowPreviewCapture`: finché non viene collegato un vero binding del
  protocollo di cattura dei frame (vedi il commento di classe nell'header — punta
  a `hyprland-toplevel-export-v1` di Hyprland, che richiede binding generati da
  wayland-scanner in fase di compilazione, volutamente esclusi da questo albero
  dei sorgenti portabile), disegna un segnaposto tematizzato per app (tessera
  in vetro colorata dall'hash + iniziale dell'app), così la panoramica è
  pienamente utilizzabile, trascinamento compreso, senza quel protocollo — e
  mostrerà frame realmente catturati nel momento in cui quel binding arriverà,
  senza alcuna modifica al QML.

## Note sull'architettura

- **`HyprlandIPC`** è l'*unico* punto che parla con il socket delle richieste
  di Hyprland (`.socket.sock`). Ogni modulo (pannello, panoramica delle aree di
  lavoro, centro di controllo) passa da lì invece di invocare `hyprctl` o
  toccare direttamente il socket.
- **`HyprlandEventListener`** si collega al socket degli eventi di Hyprland
  (`.socket2.sock`) e riemette gli eventi analizzati come segnali Qt, così
  l'interfaccia si aggiorna in modo reattivo invece di interrogare `hyprctl` a
  timer. Solo `SystemMonitor` fa polling (CPU/RAM/batteria, che non hanno
  un'API del kernel push-based), e anche in quel caso con un modesto intervallo
  di 2 secondi per mantenere trascurabile l'uso di CPU in inattività.
- **`LayerSurfaceWindow`** è la base condivisa di ogni superficie della shell.
  Quando LayerShellQt viene trovato in fase di configurazione, `main()` chiama
  `LayerShellQt::Shell::useLayerShell()` prima di creare la
  `QGuiApplication`, e ogni finestra configura layer, ancore, exclusive zone,
  margini e interattività da tastiera tramite `LayerShellQt::Window::get(window)`.
  LayerShellQt riapplica il ruolo ogni volta che Qt ricrea la `wl_surface`
  (nascondi/mostra), quindi i popup attivabili non richiedono alcuna gestione
  manuale. Senza LayerShellQt si ripiega su normali finestre di primo livello
  senza cornice.
- **Nessuna cattura di scorciatoie globali.** La shell espone un piccolo socket
  di controllo locale (`$XDG_RUNTIME_DIR/nicoos/shell-control.sock`); le righe
  `bind =` di Hyprland nel tuo `hyprland.conf` gli inviano comandi di una riga
  tramite `socat`. Così la configurazione delle scorciatoie resta dove gli
  utenti di Hyprland se la aspettano già, e si evita di interferire con la
  gestione dell'input di Hyprland.
- **Notifiche**: registrano la shell come `org.freedesktop.Notifications` sul
  bus di sessione, così qualsiasi applicazione standard (`notify-send`, browser,
  client di chat) la raggiunge tramite il normale meccanismo dei desktop Linux.
  Se un altro demone (mako, dunst, swaync) possiede già quel nome di servizio,
  il demone di NicoOS registra un avviso e si disattiva invece di contendersi il
  nome sul bus.

## Pacchetti richiesti (Arch Linux)

```bash
# obbligatori
sudo pacman -S --needed \
    base-devel cmake ninja pkgconf \
    qt6-base qt6-declarative qt6-svg \
    layer-shell-qt \
    hyprland socat \
    networkmanager pipewire pipewire-pulse wireplumber \
    bluez bluez-utils

# facoltativi: ognuno attiva una funzionalità (gli strumenti mancanti rendono
# solo più opaca la loro tessera)
sudo pacman -S --needed \
    swaybg hyprlock \
    grim slurp wl-clipboard libnotify \
    cliphist playerctl power-profiles-daemon \
    wf-recorder hyprsunset \
    papirus-icon-theme
```

`layer-shell-qt` (il wrapper Qt di KDE attorno a `wlr-layer-shell`) è ciò che
permette a ogni finestra della shell di diventare una vera layer surface
(pannello ancorato, exclusive zone, popup overlay). CMake lo cerca con
`find_package(LayerShellQt)`; se manca la compilazione riesce comunque ma
ripiega su normali finestre di primo livello, e `cmake` stampa un avviso
durante la configurazione.

## Compilazione

```bash
git clone <questo-repo> nicoos27-shell
cd nicoos27-shell
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -GNinja
cmake --build build
```

Il binario finisce in `build/nicoos-shell`.

### Installazione a livello di sistema

```bash
sudo cmake --install build
```

Installa il binario in `/usr/bin/nicoos-shell`, le icone in
`/usr/share/nicoos-shell/icons`, la configurazione di esempio in
`/usr/share/nicoos-shell/config/nicoos.conf.example` e la voce `.desktop` in
`/usr/share/applications/`.

### Oppure compila un vero pacchetto pacman

```bash
cd packaging
makepkg -si
```

(Adatta `source=()` del `PKGBUILD` perché punti a un tarball o a un checkout
VCS reale prima di compilare — la versione qui assume un albero dei sorgenti
già estratto accanto, come punto di partenza per l'impacchettamento.)

## Integrazione con Hyprland

### Formato Lua (Hyprland ≥ 0.55 — raccomandato)

Copia il file di integrazione nella tua cartella di configurazione:

```bash
cp packaging/nicoos-shell.lua ~/.config/hypr/nicoos-shell.lua
```

Poi aggiungilo al tuo `hyprland.lua` principale con una riga `require`:

```lua
-- ~/.config/hypr/hyprland.lua
local hl = require("hl")

-- ... la tua configurazione ...

require("nicoos-shell")   -- shell di NicoOS 27 Extreme
```

Se il file `nicoos-shell.lua` non è nella stessa cartella di `hyprland.lua`,
aggiusta il path di ricerca prima del require:

```lua
package.path = package.path .. ";/usr/share/nicoos-shell/?.lua"
require("nicoos-shell")
```

### Formato .conf (Hyprland < 0.55 o configurazione legacy)

```bash
mkdir -p ~/.config/hypr/conf.d
cp packaging/hyprland-nicoos.conf ~/.config/hypr/conf.d/nicoos-shell.conf
```

Poi in `hyprland.conf`:

```ini
source = ~/.config/hypr/conf.d/nicoos-shell.conf
```

### Dopo l'integrazione

Ricarica Hyprland con `hyprctl reload` o esci e rientra nella sessione.
La shell si avvia automaticamente.

### Tasti

| Tasto | Funzione |
|---|---|
| `SUPER+R` | Launcher |
| `SUPER+N` | Centro di controllo |
| `SUPER+Tab` | Vista multitasking |
| `SUPER+X` | Menu alimentazione |
| `SUPER+W` | Sfondi |
| `SUPER+V` | Appunti |
| `SUPER+C` | Calendario |
| `SUPER+L` | Blocca schermo |
| `Print` | Screenshot area |
| `SHIFT+Print` | Screenshot schermo intero |
| `SUPER+SHIFT+R` | Avvia/ferma registrazione |
| `SUPER+SHIFT+N` | Non disturbare |
| `SUPER+SHIFT+W` | Sfondo casuale |


## Schermata di blocco

La shell non si affida più a `~/.config/hypr/hyprlock.conf`. A ogni blocco
(ri)scrive `~/.config/nicoos/hyprlock.conf` da un modello integrato usando il
tuo `accent_color` e lo sfondo corrente, e avvia `hyprlock -c` su quel file.

- Per modificarlo a mano: cancella la prima riga (`# nicoos-managed ...`); da
  quel momento il file non viene più sovrascritto.
- Per usare la tua configurazione: `[lock] hyprlock_config=/percorso/di/hyprlock.conf`.
- Per cambiare il font: `[lock] font=Inter`.
- Se hyprlock continua a schiantarsi, guarda `~/.cache/nicoos/lock.log`. Su
  Arch la causa più comune è un aggiornamento parziale (hyprlock compilato
  contro `hyprutils`/`hyprlang`/`hyprgraphics` più vecchi): esegui
  `sudo pacman -Syu` perché tornino allineati.

## Configurazione

Al primo avvio `~/.config/nicoos/nicoos.conf` viene creato automaticamente con
valori predefiniti sensati (vedi `config/nicoos.conf.example` per la versione
completa e commentata). È sorvegliato a caldo: le modifiche si applicano subito,
senza riavvio:

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
```

## Esecuzione senza installare (sviluppo)

```bash
export HYPRLAND_INSTANCE_SIGNATURE=$HYPRLAND_INSTANCE_SIGNATURE  # già impostata dentro una sessione Hyprland
./build/nicoos-shell
```

Fuori da una sessione Hyprland la shell parte comunque (utile per lavorare
sull'interfaccia in isolamento) ma registra un avviso e le funzionalità di aree
di lavoro e finestre restano inerti, dato che non c'è alcun IPC del compositore
con cui parlare.

## Limiti noti / dove estendere

- `WindowPreviewCapture` (usata dalla panoramica multitasking di `SUPER+Tab`)
  disegna un segnaposto tematizzato per finestra invece di una vera cattura
  dello schermo: collegare il protocollo `hyprland-toplevel-export-v1` di
  Hyprland per anteprime davvero dal vivo richiede che il suo XML di protocollo
  (dal pacchetto dei protocolli di Hyprland) venga trasformato in binding C con
  `wayland-scanner` e compilato insieme ai sorgenti C++ — vedi il commento di
  classe di `WindowPreviewCapture.hpp` per il punto in cui si innesta la vera
  chiamata di cattura.
- L'elenco delle reti vicine viene da `WifiScanner` (`nmcli`);
  `NetworkManagerClient` descrive soltanto la connessione *corrente* (nome,
  tipo, intensità reale del segnale) e controlla la radio Wi-Fi.
- `BluetoothClient` assume un solo adattatore in `/org/bluez/hci0`; i sistemi
  con più adattatori enumererebbero invece gli oggetti di
  `org.bluez.Manager1`.
