-- ~/.config/hypr/nicoos-shell.lua
--
-- NicoOS 27 Extreme — integrazione Hyprland (formato Lua, Hyprland ≥ 0.55)
--
-- Includi dal tuo hyprland.lua principale con:
--   require("nicoos-shell")
-- (metti questo file nella stessa cartella di hyprland.lua, oppure
--  aggiungi la sua cartella a package.path prima del require)
--
-- La shell NON cattura tasti globali direttamente (interferirebbe con
-- l'input handling di Hyprland). Apre invece un socket di controllo in
--   $XDG_RUNTIME_DIR/nicoos/shell-control.sock
-- e questi bind ci inviano comandi testuali tramite `socat`.

local hl = require("hl")

-- -----------------------------------------------------------------------
-- Helper: percorso del socket e comando socat
-- -----------------------------------------------------------------------
local socket = (os.getenv("XDG_RUNTIME_DIR") or "/run/user/1000")
               .. "/nicoos/shell-control.sock"

-- Restituisce il dispatcher hl.dsp.exec_cmd() già costruito con il
-- comando socat per quel particolare messaggio alla shell.
local function nicoos(cmd)
    return hl.dsp.exec_cmd(
        string.format('echo "%s" | socat - UNIX-CONNECT:%s', cmd, socket)
    )
end

-- -----------------------------------------------------------------------
-- Avvio automatico (una sola volta per sessione, come exec-once)
-- -----------------------------------------------------------------------
-- README_SHELL.md: "Assicurati che hyprland avvii la shell con:
--   LIBGL_ALWAYS_SOFTWARE=1 QT_QUICK_BACKEND=software nicoos-shell"
-- (il renderer software è ciò che fa partire la shell su qualsiasi GPU, VM o
-- hardware reale, compresa la ISO live.)
hl.on("hyprland.start", function()
    hl.exec_cmd("LIBGL_ALWAYS_SOFTWARE=1 QT_QUICK_BACKEND=software nicoos-shell")
end)

-- -----------------------------------------------------------------------
-- Variabile modifier (cambia qui se il tuo modifier è diverso da SUPER)
-- -----------------------------------------------------------------------
-- I modificatori sono token separati: il parser dei bind Lua di Hyprland
-- divide il binding su "+" e legge ogni token come un modificatore o un tasto.
-- "SUPER+SHIFT" è un unico token sconosciuto (segnalava "Unknown key: SUPER
-- SHIFT"); la grafia corretta è quella che hyprland.lua usa per
-- SUPER + SHIFT + F.
local M  = "SUPER"
local MS = "SUPER + SHIFT"

-- -----------------------------------------------------------------------
-- Popup (toggle)
-- -----------------------------------------------------------------------
hl.bind(M .. " + R",   nicoos("launcher.toggle"))
hl.bind(M .. " + N",   nicoos("controlcenter.toggle"))
hl.bind(M .. " + Tab", nicoos("workspaces.toggle"))
hl.bind(M .. " + X",   nicoos("power.toggle"))
hl.bind(M .. " + W",   nicoos("wallpapers.toggle"))
hl.bind(M .. " + V",   nicoos("clipboard.toggle"))
hl.bind(M .. " + C",   nicoos("calendar.toggle"))

-- -----------------------------------------------------------------------
-- Azioni rapide
-- -----------------------------------------------------------------------
hl.bind("Print",       nicoos("screenshot.region"))
hl.bind("SHIFT + Print", nicoos("screenshot.full"))
hl.bind(M .. " + L",  nicoos("lock"))
hl.bind(MS .. " + R", nicoos("record.toggle"))
hl.bind(MS .. " + N", nicoos("dnd.toggle"))
hl.bind(MS .. " + W", nicoos("wallpaper.random"))

-- -----------------------------------------------------------------------
-- Regole finestra (fallback senza layer-shell)
--
-- Con layer-shell-qt installato la shell diventa una vera layer surface
-- e non ha bisogno di queste regole (l'exclusive zone è impostata via
-- protocollo Wayland). Senza layer-shell-qt Hyprland la tratta come
-- un'app normale: queste regole la fanno fluttuare invece di tilerarla.
-- -----------------------------------------------------------------------
hl.window_rule({
    match = { class = "nicoos-shell" },
    float = true,
})

hl.window_rule({
    match = { class = "nicoos-shell", title = "Panel" },
    no_focus = true,
})

hl.window_rule({
    match = { class = "nicoos-shell", title = "Launcher|Power|Clipboard|Wallpapers" },
    stay_focused = true,
})

--[[
    Tutti i comandi che il socket accetta:

    Popup toggle:
      launcher.toggle     controlcenter.toggle    workspaces.toggle
      power.toggle        wallpapers.toggle        clipboard.toggle
      calendar.toggle

    Azioni:
      lock                suspend
      screenshot.region   screenshot.full
      record.toggle
      dnd.toggle          nightlight.toggle        caffeine.toggle
      profile.cycle       wallpaper.random
]]
