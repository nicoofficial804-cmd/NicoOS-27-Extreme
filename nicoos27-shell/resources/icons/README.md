# Icone

Directory segnaposto per gli asset delle icone della shell NicoOS (icona
dell'app, icona della tray, ecc.). Attualmente vuota: la shell disegna da sé i
propri glifi di stato direttamente in QML invece di caricare file di icone, e
le icone per applicazione del launcher provengono dal tema di icone già
presente nel sistema tramite il campo `Icon=` di ogni voce `.desktop`
(risolto dalla ricerca del tema di icone della piattaforma Qt, non da file
distribuiti qui).

Metti qui un `nicoos-shell.svg` / `.png` se vuoi che `Icon=nicoos-shell` della
voce `.desktop` impacchettata risolva a qualcosa di diverso dal ripiego
generico.
