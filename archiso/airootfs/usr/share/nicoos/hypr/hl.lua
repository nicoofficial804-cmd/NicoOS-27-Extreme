-- /usr/share/nicoos/hypr/hl.lua
--
-- Lo stesso shim di tre righe di ~/.config/hypr/hl.lua: il runtime Lua di
-- Hyprland fornisce l'API di configurazione come tabella globale `hl`, e il
-- file di integrazione della shell NicoOS inizia con
-- `local hl = require("hl")`. ~/.config/hypr/hyprland.lua aggiunge questa
-- directory a package.path, così il require si risolve anche per un utente
-- con una configurazione creata a mano.
return hl
