-- ~/.config/hypr/hyprland.lua
--
-- NicoOS 27 Extreme — configurazione Lua di Hyprland 0.57+.
--
-- La shell NicoOS viene caricata in fondo a questo file con
--     require("nicoos-shell")
-- esattamente come descritto in README_SHELL.md. nicoos-shell.lua si trova
-- accanto a questo file (~/.config/hypr/nicoos-shell.lua) e possiede le
-- scorciatoie della shell.

local terminal = "foot"
local accent   = "rgba(e3272bee)"
local accent2  = "rgba(b41e22ee)"

-----------------
---- MONITOR ----
-----------------

hl.monitor({
    output   = "",
    mode     = "preferred",
    position = "auto",
    scale    = "auto",
})

---------------------
---- AVVIO AUTO ----
---------------------

-- Servizi del desktop che non fanno parte della shell NicoOS stessa.
hl.on("hyprland.start", function()
    -- gestione dell'inattività (blocco dopo 10 min, dpms dopo 15)
    hl.exec_cmd("hypridle")
    -- sfondo NicoOS (il selettore di sfondi della shell lo sostituisce a runtime)
    hl.exec_cmd("swaybg -m fill -i /usr/share/backgrounds/nicoos/nicoos-wallpaper.png")
    -- mantiene la sessione utente systemd allineata all'ambiente Wayland
    hl.exec_cmd("dbus-update-activation-environment --systemd WAYLAND_DISPLAY XDG_CURRENT_DESKTOP XDG_SESSION_TYPE")
    -- rete di sicurezza: l'hook della shell (require("nicoos-shell") più sotto)
    -- avvia nicoos-shell; questo assicura che sia in esecuzione anche se
    -- quell'hook non è scattato
    hl.exec_cmd("nicoos-ensure-shell")
end)

----------------------------
---- VARIABILI D'AMBIENTE ---
----------------------------

-- Tema del cursore: deve esistere davvero (pacchetto adwaita-cursors). Senza
-- un tema Xcursor installato il puntatore ricade sul cursore predefinito di
-- Xorg, cioè sulla "X", che è anche quello disegnato dal greeter SDDM prima
-- del login.
hl.env("XCURSOR_THEME", "Adwaita")
hl.env("XCURSOR_SIZE", "24")
hl.env("HYPRCURSOR_SIZE", "24")
-- NicoOS è una distribuzione Wayland-first: forza le app Qt su Wayland.
hl.env("QT_QPA_PLATFORM", "wayland")
hl.env("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1")
hl.env("GDK_BACKEND", "wayland,x11")
hl.env("MOZ_ENABLE_WAYLAND", "1")
hl.env("ELECTRON_OZONE_PLATFORM_HINT", "auto")

----------------------
---- ASPETTO E STILE -
----------------------

hl.config({
    general = {
        gaps_in     = 4,
        gaps_out    = 10,
        border_size = 2,
        col = {
            active_border   = { colors = { accent, accent2 }, angle = 45 },
            inactive_border = "rgba(2a0d0faa)",
        },
        resize_on_border = true,
        allow_tearing    = false,
        layout           = "dwindle",
    },

    decoration = {
        rounding       = 8,
        rounding_power = 2,
        active_opacity   = 1.0,
        inactive_opacity = 0.96,

        shadow = {
            enabled      = true,
            range        = 12,
            render_power = 3,
            color        = 0xee000000,
        },

        blur = {
            enabled  = true,
            size     = 4,
            passes   = 2,
            vibrancy = 0.15,
        },
    },

    animations = {
        enabled = true,
    },

    dwindle = {
        preserve_split = true,
    },

    master = {
        new_status = "master",
    },

    misc = {
        force_default_wallpaper = -1,
        disable_hyprland_logo   = true,
        disable_splash_rendering = true,
        focus_on_activate       = true,
    },

    input = {
        kb_layout    = "it",
        follow_mouse = 1,
        sensitivity  = 0,
        touchpad = {
            natural_scroll = true,
            tap_to_click   = true,
        },
    },

    cursor = {
        -- Il cursore viene disegnato dal compositore invece che sul piano
        -- hardware della GPU. È ciò che elimina la "X" rimasta sullo schermo:
        -- il greeter SDDM gira su Xorg e lascia la propria immagine di cursore
        -- nel piano hardware, che il kernel non azzera quando Hyprland prende
        -- possesso della GPU. Con i cursori software quel piano non viene mai
        -- usato, quindi l'immagine del greeter sparisce al primo fotogramma.
        -- (Hyprland lo consiglia anche su GPU virtuali e NVIDIA, dove il piano
        -- hardware a volte non mostra affatto il puntatore.)
        no_hardware_cursors = true,
        hide_on_key_press   = true,
    },

    binds = {
        workspace_back_and_forth = true,
    },
})

-- Movimento: breve, asciutto, nessuna molla rimbalzante.
hl.curve("nicoosEase", { type = "bezier", points = { {0.16, 1}, {0.3, 1} } })
hl.curve("nicoosQuick", { type = "bezier", points = { {0.2, 0}, {0, 1} } })

hl.animation({ leaf = "global",     enabled = true, speed = 6,  bezier = "nicoosEase" })
hl.animation({ leaf = "windows",    enabled = true, speed = 5,  bezier = "nicoosEase", style = "popin 92%" })
hl.animation({ leaf = "windowsOut", enabled = true, speed = 4,  bezier = "nicoosQuick", style = "popin 90%" })
hl.animation({ leaf = "fade",       enabled = true, speed = 4,  bezier = "nicoosEase" })
hl.animation({ leaf = "layers",     enabled = true, speed = 4,  bezier = "nicoosEase" })
hl.animation({ leaf = "workspaces", enabled = true, speed = 4,  bezier = "nicoosEase", style = "slidefade 12%" })
hl.animation({ leaf = "border",     enabled = true, speed = 6,  bezier = "nicoosEase" })

hl.gesture({ fingers = 3, direction = "horizontal", action = "workspace" })

----------------------
---- SCORCIATOIE -----
----------------------
-- La shell NicoOS possiede SUPER+R/N/Tab/X/W/V/C/L, Print e SUPER+SHIFT+R/N/W
-- (vedi ~/.config/hypr/nicoos-shell.lua). Tutto ciò che segue è Hyprland.

local mod = "SUPER"

hl.bind(mod .. " + Return", hl.dsp.exec_cmd(terminal))
hl.bind(mod .. " + Q",      hl.dsp.window.close())
hl.bind(mod .. " + F",      hl.dsp.exec_cmd("hyprctl dispatch fullscreen 0"))
hl.bind(mod .. " + SHIFT + F", hl.dsp.exec_cmd("hyprctl dispatch fullscreen 1"))
hl.bind(mod .. " + T",      hl.dsp.window.float({ action = "toggle" }))
hl.bind(mod .. " + P",      hl.dsp.window.pseudo())
hl.bind(mod .. " + J",      hl.dsp.layout("togglesplit"))
hl.bind(mod .. " + SHIFT + Q", hl.dsp.exec_cmd("hyprctl dispatch exit"))

-- focus
hl.bind(mod .. " + left",  hl.dsp.focus({ direction = "left" }))
hl.bind(mod .. " + right", hl.dsp.focus({ direction = "right" }))
hl.bind(mod .. " + up",    hl.dsp.focus({ direction = "up" }))
hl.bind(mod .. " + down",  hl.dsp.focus({ direction = "down" }))

-- aree di lavoro
for i = 1, 10 do
    local key = i % 10
    hl.bind(mod .. " + " .. key,         hl.dsp.focus({ workspace = i }))
    hl.bind(mod .. " + SHIFT + " .. key, hl.dsp.window.move({ workspace = i }))
end

hl.bind(mod .. " + mouse_down", hl.dsp.focus({ workspace = "e+1" }))
hl.bind(mod .. " + mouse_up",   hl.dsp.focus({ workspace = "e-1" }))

-- sposta / ridimensiona con il mouse
hl.bind(mod .. " + mouse:272", hl.dsp.window.drag(),   { mouse = true })
hl.bind(mod .. " + mouse:273", hl.dsp.window.resize(), { mouse = true })

-- tasti del portatile
hl.bind("XF86AudioRaiseVolume",  hl.dsp.exec_cmd("wpctl set-volume -l 1 @DEFAULT_AUDIO_SINK@ 5%+"), { locked = true, repeating = true })
hl.bind("XF86AudioLowerVolume",  hl.dsp.exec_cmd("wpctl set-volume @DEFAULT_AUDIO_SINK@ 5%-"),      { locked = true, repeating = true })
hl.bind("XF86AudioMute",         hl.dsp.exec_cmd("wpctl set-mute @DEFAULT_AUDIO_SINK@ toggle"),     { locked = true, repeating = true })
hl.bind("XF86AudioMicMute",      hl.dsp.exec_cmd("wpctl set-mute @DEFAULT_AUDIO_SOURCE@ toggle"),   { locked = true, repeating = true })
hl.bind("XF86MonBrightnessUp",   hl.dsp.exec_cmd("brightnessctl -e4 -n2 set 5%+"),                  { locked = true, repeating = true })
hl.bind("XF86MonBrightnessDown", hl.dsp.exec_cmd("brightnessctl -e4 -n2 set 5%-"),                  { locked = true, repeating = true })
hl.bind("XF86AudioNext",         hl.dsp.exec_cmd("playerctl next"),        { locked = true })
hl.bind("XF86AudioPause",        hl.dsp.exec_cmd("playerctl play-pause"),  { locked = true })
hl.bind("XF86AudioPlay",         hl.dsp.exec_cmd("playerctl play-pause"),  { locked = true })
hl.bind("XF86AudioPrev",         hl.dsp.exec_cmd("playerctl previous"),    { locked = true })

-------------------------------
---- FINESTRE E AREE DI LAVORO
-------------------------------

-- Nessuna richiesta di massimizzazione dalle app; NicoOS è tiling-first.
hl.window_rule({
    name  = "nicoos-suppress-maximize",
    match = { class = ".*" },
    suppress_event = "maximize",
})

-- Valori predefiniti migliori per i comuni strumenti flottanti.
hl.window_rule({
    name  = "nicoos-calamares",
    match = { class = "calamares" },
    float = true,
    size  = "1100 720",
    center = true,
})

hl.window_rule({
    name  = "nicoos-pavucontrol",
    match = { class = "org.pulseaudio.pavucontrol" },
    float = true,
    size  = "800 560",
    center = true,
})

hl.window_rule({
    name  = "nicoos-xwayland-fix",
    match = { class = "^$", title = "^$", xwayland = true, float = true, fullscreen = false, pin = false },
    no_focus = true,
})

--------------------------------------
---- SHELL NICOOS (README_SHELL) -----
--------------------------------------
-- File di integrazione affiancato, installato accanto a questo
-- (/usr/share/nicoos-shell/nicoos-shell.lua, copiato qui dall'installer
-- NicoOS, esattamente come descritto in README_SHELL.md).
--
-- nicoos-shell.lua inizia con `local hl = require("hl")`, mentre Hyprland
-- passa alla configurazione una tabella `hl` *globale*. Lo shim `hl.lua`
-- accanto a questo file fa sì che `require("hl")` risolva a quella stessa
-- tabella, così le scorciatoie della shell e il suo hook di avvio automatico
-- vengono caricati correttamente.
package.path = package.path
    .. ";" .. ((os.getenv("HOME") or "") .. "/.config/hypr/?.lua")
    .. ";/usr/share/nicoos/hypr/?.lua"

require("nicoos-shell")
