cmake_minimum_required(VERSION 3.21)
project(nicoos27-shell VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Quick
    QuickControls2
    Network
    DBus
)

find_package(PkgConfig REQUIRED)

qt_standard_project_setup()

# --- Layer shell tramite LayerShellQt di KDE -------------------------
#
# LayerShellQt avvolge wlr-layer-shell-unstable-v1 per le finestre Qt senza
# bisogno di header privati di Qt o di codice di protocollo generato a mano.
# Se non è installato la shell si compila comunque e ripiega su normali
# finestre di primo livello (vedi LayerSurfaceWindow.cpp).
find_package(LayerShellQt QUIET)
set(NICOOS_LAYER_SHELL_AVAILABLE ${LayerShellQt_FOUND})
if(LayerShellQt_FOUND)
    message(STATUS "LayerShellQt trovato - compilazione con vero supporto layer-shell")
else()
    message(WARNING
        "LayerShellQt non trovato - compilazione SENZA vero supporto layer-shell. "
        "Installa il pacchetto 'layer-shell-qt' e riconfigura.")
endif()

set(SOURCES
    src/main.cpp
    src/controlcenter/AudioClient.cpp
    src/controlcenter/BluetoothClient.cpp
    src/controlcenter/BrightnessClient.cpp
    src/controlcenter/ClipboardManager.cpp
    src/controlcenter/ControlCenterWindow.cpp
    src/controlcenter/MediaClient.cpp
    src/controlcenter/NetworkManagerClient.cpp
    src/controlcenter/PowerActions.cpp
    src/controlcenter/WifiScanner.cpp
    src/hyprland/HyprlandEventListener.cpp
    src/hyprland/HyprlandIPC.cpp
    src/launcher/AppIconProvider.cpp
    src/launcher/AppIndexer.cpp
    src/launcher/FuzzyMatcher.cpp
    src/launcher/LauncherWindow.cpp
    src/notifications/NotificationDaemon.cpp
    src/notifications/NotificationWindow.cpp
    src/panel/ClockWidget.cpp
    src/panel/PanelWindow.cpp
    src/panel/StatusIndicators.cpp
    src/panel/TaskbarModel.cpp
    src/shell/LayerSurfaceWindow.cpp
    src/shell/PopupWindow.cpp
    src/shell/ShellApplication.cpp
    src/shell/ShellController.cpp
    src/system/ConfigManager.cpp
    src/system/QuickActions.cpp
    src/system/SystemMonitor.cpp
    src/system/WallpaperManager.cpp
    src/workspaces/WindowPreviewCapture.cpp
    src/workspaces/WorkspaceOverview.cpp
)

set(HEADERS
    include/nicoos/AppIconProvider.hpp
    include/nicoos/AppIndexer.hpp
    include/nicoos/AudioClient.hpp
    include/nicoos/BluetoothClient.hpp
    include/nicoos/BrightnessClient.hpp
    include/nicoos/ClipboardManager.hpp
    include/nicoos/ClockWidget.hpp
    include/nicoos/ConfigManager.hpp
    include/nicoos/ControlCenterWindow.hpp
    include/nicoos/FuzzyMatcher.hpp
    include/nicoos/HyprlandEventListener.hpp
    include/nicoos/HyprlandIPC.hpp
    include/nicoos/LauncherWindow.hpp
    include/nicoos/LayerSurfaceWindow.hpp
    include/nicoos/MediaClient.hpp
    include/nicoos/NetworkManagerClient.hpp
    include/nicoos/NotificationDaemon.hpp
    include/nicoos/NotificationWindow.hpp
    include/nicoos/PanelWindow.hpp
    include/nicoos/PopupWindow.hpp
    include/nicoos/PowerActions.hpp
    include/nicoos/ProcessUtil.hpp
    include/nicoos/QuickActions.hpp
    include/nicoos/ShellApplication.hpp
    include/nicoos/ShellController.hpp
    include/nicoos/StatusIndicators.hpp
    include/nicoos/SystemMonitor.hpp
    include/nicoos/TaskbarModel.hpp
    include/nicoos/WallpaperManager.hpp
    include/nicoos/WifiScanner.hpp
    include/nicoos/WindowPreviewCapture.hpp
    include/nicoos/WorkspaceOverview.hpp
)

qt_add_executable(nicoos-shell
    ${SOURCES}
    ${HEADERS}
    resources/resources.qrc
)

target_include_directories(nicoos-shell PRIVATE
    include
)

target_link_libraries(nicoos-shell PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
    Qt6::DBus
)

if(NICOOS_LAYER_SHELL_AVAILABLE)
    target_compile_definitions(nicoos-shell PRIVATE NICOOS_HAVE_LAYER_SHELL)
    target_link_libraries(nicoos-shell PRIVATE LayerShellQt::Interface)
endif()

install(TARGETS nicoos-shell RUNTIME DESTINATION bin)

# Le icone sono facoltative: installa la directory solo se l'albero dei
# sorgenti ne contiene davvero una popolata (evita "file INSTALL cannot find"
# quando non è ancora stato aggiunto alcun asset).
if(EXISTS "${CMAKE_SOURCE_DIR}/resources/icons")
    install(DIRECTORY resources/icons DESTINATION share/nicoos-shell
            FILES_MATCHING PATTERN "*.svg" PATTERN "*.png")
endif()

install(FILES config/nicoos.conf.example DESTINATION share/nicoos-shell/config)
install(FILES packaging/nicoos-shell.desktop DESTINATION share/applications)
install(FILES packaging/nicoos-shell.lua DESTINATION share/nicoos-shell)
