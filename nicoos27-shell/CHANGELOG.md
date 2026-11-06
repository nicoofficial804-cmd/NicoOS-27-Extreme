# Registro delle modifiche

## 1.2.2

### Corretto
- **Macchine virtuali (VMware/VirtualBox/QEMU).** La GPU emulata non riesce a
  importare i buffer con cui Qt e hyprlock disegnano; Hyprland interrompe quindi
  la connessione (`wl_surface.attach: invalid arguments`, hyprlock
  `ASSERTION FAILED ... Disconnected from pollfd`). In una VM la shell ora parte
  con il renderer software di Qt Quick (`NICOOS_FORCE_GPU=1` per disattivarlo)
  e hyprlock con `LIBGL_ALWAYS_SOFTWARE=1` (`[lock] software_render = auto|yes|no`).
- Le icone colorate non spariscono più con il renderer software.
- hyprlock >= 0.9 si rifiuta di partire senza un file di configurazione, quindi
  il vecchio passo "riprova con la configurazione predefinita" falliva sempre
  con "Could not find config". L'ordine di ripiego ora è: configurazione
  completa generata -> configurazione minimale generata
  (`hyprlock-safe.conf`: niente blur/animazioni, GL software) -> la tua
  `~/.config/hypr/hyprlock.conf` se esiste -> swaylock -> gtklock.

## 1.2.1

### Corretto
- **Ambiente Hyprland non aggiornato.** La shell leggeva
  `HYPRLAND_INSTANCE_SIGNATURE` una sola volta all'avvio e passava a hyprlock il
  proprio ambiente ereditato. Se quel valore era vecchio (Hyprland riavviato,
  shell lanciata da un'altra sessione), "Esci" e ogni chiamata IPC finivano su
  un socket morto e hyprlock non riusciva a raggiungere il compositore.
  L'istanza viva viene ora cercata a ogni richiesta
  (`$XDG_RUNTIME_DIR/hypr/*/hyprland.lock`), il listener degli eventi continua a
  riprovare finché non esiste, e i bloccanti/hyprctl ricevono esplicitamente il
  `WAYLAND_DISPLAY` e la signature vivi. I valori usati sono registrati in
  `~/.cache/nicoos/lock.log`.
- Il logout di ultima istanza ora segnala il PID esatto del compositore invece
  di usare `pkill`.

## 1.2.0

### Corretto
- **"Esci" non faceva nulla su alcune configurazioni.** Il logout ora prova
  `uwsm stop` (se la sessione è gestita da uwsm), poi il dispatcher `exit` di
  Hyprland, e se il compositore è ancora vivo dopo 1,5 s / 3 s sale a
  `hyprctl dispatch exit`, `loginctl terminate-session` e infine
  `pkill -x Hyprland`; se tutto fallisce una notifica lo dice (prima i
  fallimenti erano silenziosi).
- **Il blocco usava la configurazione predefinita di hyprlock.** hyprlock viene
  ora avviato con `-c ~/.config/nicoos/hyprlock.conf` (generata, oppure la tua
  tramite `[lock] hyprlock_config`).
- **Schermata di blocco lasciata morta dopo un crash di hyprlock** (ad esempio
  con una password errata): un bloccante che muore dopo essere stato in
  esecuzione più di 2,5 s viene riavviato automaticamente (massimo 5 volte al
  minuto, poi si prova il bloccante successivo), e tutto l'output dei bloccanti
  resta in `~/.cache/nicoos/lock.log`.

### Aggiunto
- Nuovo aspetto di hyprlock: sfondo sfocato, pannello in vetro, orologio e data
  grandi, nome utente, campo password arrotondato con gradiente d'accento,
  messaggio "Password errata (n)".
- Opzioni `[lock] hyprlock_config` e `[lock] font`.

## 1.1.0

### Corretto
- **Le aree di lavoro speciali non compaiono più come punti.** Gli scratchpad e
  l'area di parcheggio `special:minimized` (id negativi) venivano elencati
  nell'indicatore delle aree di lavoro.
- **Il pulsante di chiusura della barra delle applicazioni** dava anche il focus
  o minimizzava la finestra che stava chiudendo (partivano entrambi i
  `TapHandler` annidati). Ora la × chiude soltanto.
- **Informazioni di rete obsolete.** Dopo una disconnessione restavano il
  vecchio SSID e tipo; lo stato ora viene ricostruito da zero a ogni
  aggiornamento.
- **L'attivazione della radio Wi-Fi** usava `QDBusInterface::setProperty`, poco
  affidabile con NetworkManager; ora usa `org.freedesktop.DBus.Properties.Set`.
- **La riscansione Wi-Fi** chiamava `CheckConnectivity`; ora chiama
  `RequestScan` su ogni dispositivo Wi-Fi.
- L'intensità del segnale era fissa a 100; ora è quella reale del punto di
  accesso (e segue i suoi cambiamenti).
- `workspace_wrap_around` veniva letto ma mai usato.

### Prestazioni
- Gli aggiornamenti del modello delle aree di lavoro sono incrementali
  (rimozione/inserimento/dataChanged) invece di un reset completo, così le
  animazioni dei punti non vengono più distrutte a ogni evento, e le raffiche di
  eventi di Hyprland vengono unite (30 ms) in un solo insieme di chiamate IPC
  bloccanti.
- La panoramica delle aree di lavoro salta gli aggiornamenti mentre è nascosta.
- `SystemMonitor` segnala al QML solo quando cambia un valore visualizzato
  (arrotondato).
- L'orologio si risveglia una volta al minuto (allineato al minuto) a meno che
  il formato contenga i secondi, invece che ogni secondo.
- `NetworkManagerClient` raggruppa le raffiche di segnali D-Bus (120 ms) ed
  emette `stateChanged` solo su cambiamenti reali.

### Aggiunto
- La rotellina del mouse sui punti delle aree di lavoro cambia area di lavoro;
  sulle icone di stato cambia il volume a passi del 5%.
- L'icona Wi-Fi si attenua con l'intensità reale del segnale.
- `NetworkManagerClient`: proprietà `isWifi`, `wifiEnabled`.

### Rimosso
- `NetworkManagerClient::availableNetworks()` (uno stub che restituiva sempre
  una lista vuota; `WifiScanner` fornisce la lista reale).
