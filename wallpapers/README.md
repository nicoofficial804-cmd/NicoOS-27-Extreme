# Sfondi di NicoOS 27 Extreme

Metti qui le immagini di sfondo che vuoi vedere nel selettore di sfondi della
shell (`SUPER+W`).

Ogni `./build.sh` copia il contenuto di questa cartella in

```
/usr/share/nicoos/wallpapers
```

dentro l'immagine, quindi gli sfondi sono disponibili sia nella ISO live sia
nel sistema installato da Calamares (che scompatta lo stesso root file system).
`/usr/share/nicoos/wallpapers` è una delle cartelle cercate da
`WallpaperManager` (vedi `nicoos27-shell/src/system/WallpaperManager.cpp`).

- Formati: `png`, `jpg`/`jpeg`, `webp`, `bmp` (le sottocartelle non vengono
  copiate, solo i file al primo livello).
- Gli sfondi `nicoos-*.png` sono generati da `tools/make-branding.py` con la
  palette di NicoOS: se ne modifichi uno, la prossima compilazione lo
  sovrascrive. Aggiungi i tuoi file con un altro nome.
- Non serve nient'altro: `./build.sh` (o `./build.sh --branding` per rigenerare
  solo gli asset grafici) aggiorna la ISO.

Al primo avvio la sessione usa lo sfondo predefinito
`/usr/share/backgrounds/nicoos/nicoos-wallpaper.png` (avviato da
`hyprland.lua` con `swaybg`); da quel momento la shell ricorda quello scelto.
