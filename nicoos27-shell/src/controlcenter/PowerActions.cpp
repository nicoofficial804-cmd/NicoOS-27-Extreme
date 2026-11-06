#include "nicoos/PowerActions.hpp"
#include "nicoos/ConfigManager.hpp"
#include "nicoos/HyprlandIPC.hpp"
#include "nicoos/Notify.hpp"
#include "nicoos/ProcessUtil.hpp"

#include <QColor>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimer>
#include <algorithm>
#include <memory>
#include <csignal>
#include <sys/types.h>

namespace nicoos {

namespace {

struct Locker {
    QString program;
    QStringList args;
    bool forceSoftware{false};  // LIBGL_ALWAYS_SOFTWARE=1 per questo tentativo
};

constexpr auto kManagedMarker = "# nicoos-managed";

// ---- configurazione di hyprlock ----------------------------------------

// Token: @ACCENT@ (rrggbb), @FONT@, @BACKGROUND@ (righe, può essere vuoto).
const char* kHyprlockTemplate = R"HL(# nicoos-managed - generato dalla shell NicoOS ad ogni blocco schermo.
# Per modificarlo a mano cancella questa riga: da quel momento il file non
# verra' piu' toccato. In alternativa imposta [lock] hyprlock_config in
# ~/.config/nicoos/nicoos.conf per usare un tuo file.

general {
    hide_cursor = true
    ignore_empty_input = true
}

animations {
    enabled = true
    bezier = smooth, 0.23, 1, 0.32, 1
    animation = fadeIn, 1, 6, smooth
    animation = fadeOut, 1, 4, smooth
    animation = inputFieldDots, 1, 2, smooth
}

background {
    monitor =
    color = rgba(070b12ff)
@BACKGROUND@
    blur_passes = 3
    blur_size = 7
    noise = 0.012
    contrast = 0.92
    brightness = 0.55
}

# pannello "vetro" dietro a orologio e password
shape {
    monitor =
    size = 460, 420
    color = rgba(0a0f18b0)
    rounding = 36
    border_size = 1
    border_color = rgba(ffffff26)
    position = 0, 0
    halign = center
    valign = center
}

label {
    monitor =
    text = $TIME
    color = rgba(ffffffee)
    font_size = 92
    font_family = @FONT@
    position = 0, 122
    halign = center
    valign = center
}

# barretta d'accento sotto l'ora
shape {
    monitor =
    size = 56, 3
    color = rgba(@ACCENT@ee)
    rounding = -1
    position = 0, 62
    halign = center
    valign = center
}

label {
    monitor =
    text = cmd[update:60000] echo "$(date +"%A %-d %B")"
    color = rgba(ffffffb8)
    font_size = 18
    font_family = @FONT@
    position = 0, 30
    halign = center
    valign = center
}

label {
    monitor =
    text = $USER
    color = rgba(ffffff80)
    font_size = 14
    font_family = @FONT@
    position = 0, -22
    halign = center
    valign = center
}

input-field {
    monitor =
    size = 340, 56
    outline_thickness = 2
    dots_size = 0.25
    dots_spacing = 0.35
    dots_center = true
    outer_color = rgba(@ACCENT@ee) rgba(ffffff55) 45deg
    inner_color = rgba(ffffff14)
    font_color = rgba(ffffffee)
    font_family = @FONT@
    check_color = rgba(@ACCENT@ee) rgba(ffffffaa) 120deg
    fail_color = rgba(ff5c5cee) rgba(ff8a5cee) 40deg
    fade_on_empty = false
    rounding = -1
    placeholder_text = Password
    fail_text = Password errata ($ATTEMPTS)
    fail_timeout = 2000
    fail_transition = 250
    position = 0, -100
    halign = center
    valign = center
}

label {
    monitor =
    text = NicoOS 27 Extreme
    color = rgba(ffffff55)
    font_size = 13
    font_family = @FONT@
    position = 0, 34
    halign = center
    valign = bottom
}
)HL";

// Ripiego usato quando la configurazione completa fa andare in crash hyprlock:
// niente che richieda passaggi di sfocatura, dissolvenze o target di rendering
// aggiuntivi.
const char* kHyprlockSafeTemplate = R"HL(# nicoos-managed - versione "sicura" (senza sfocatura ne' animazioni).
# Usata solo se quella completa fa chiudere hyprlock. Cancella questa riga per
# modificarla a mano.

general {
    hide_cursor = true
    ignore_empty_input = true
    no_fade_in = true
    no_fade_out = true
}

background {
    monitor =
    color = rgba(070b12ff)
}

label {
    monitor =
    text = $TIME
    color = rgba(ffffffee)
    font_size = 90
    font_family = @FONT@
    position = 0, 120
    halign = center
    valign = center
}

input-field {
    monitor =
    size = 320, 54
    outline_thickness = 2
    outer_color = rgba(@ACCENT@ee)
    inner_color = rgba(ffffff14)
    font_color = rgba(ffffffee)
    font_family = @FONT@
    fade_on_empty = false
    rounding = -1
    placeholder_text = Password
    fail_text = Password errata ($ATTEMPTS)
    position = 0, -60
    halign = center
    valign = center
}
)HL";

bool isUsableImage(const QString& path) {
    if (path.isEmpty() || path.contains('\n')) return false;
    const QFileInfo fi(path);
    if (!fi.exists() || !fi.isFile()) return false;
    const QString ext = fi.suffix().toLower();
    return ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "webp";
}

QString renderHyprlockConfig(const char* tmpl = kHyprlockTemplate) {
    auto& cfg = ConfigManager::instance();
    QString accent = cfg.accentColor().name(QColor::HexRgb);  // "#rrggbb"
    accent.remove('#');
    // I font finiscono in un valore hyprlang: tienilo privo di separatori.
    QString font = cfg.lockFont();
    font.remove(QRegularExpression(QStringLiteral("[\\n\\r,#$]")));
    if (font.isEmpty()) font = QStringLiteral("Noto Sans");

    QString background;
    const QString wall = cfg.wallpaperPath();
    if (isUsableImage(wall)) {
        background = QStringLiteral("    path = ") + wall;
    }

    QString text = QString::fromUtf8(tmpl);
    text.replace(QStringLiteral("@ACCENT@"), accent);
    text.replace(QStringLiteral("@FONT@"), font);
    text.replace(QStringLiteral("\n@BACKGROUND@"), background.isEmpty() ? QString() : "\n" + background);
    return text;
}

// Scrive `text` in `path` a meno che l'utente non abbia preso in mano il file
// (la prima riga non è più il marcatore nicoos) o che sia già aggiornato.
// Restituisce il percorso, o una stringa vuota quando non è possibile
// scrivere.
QString writeManaged(const QString& path, const QString& text) {
    QDir().mkpath(QFileInfo(path).absolutePath());
    if (QFileInfo::exists(path)) {
        QFile f(path);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QByteArray all = f.readAll();
            if (!all.startsWith(kManagedMarker)) return path;  // dell'utente
            if (QString::fromUtf8(all) == text) return path;   // già aggiornato
        }
    }
    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[PowerActions] impossibile scrivere" << path;
        return QString();
    }
    out.write(text.toUtf8());
    if (!out.commit()) {
        qWarning() << "[PowerActions] impossibile salvare" << path;
        return QString();
    }
    return path;
}

QString lockLogPath() {
    const QString dir = QDir::homePath() + "/.cache/nicoos";
    QDir().mkpath(dir);
    return dir + "/lock.log";
}

void appendLog(const QByteArray& data) {
    QFile f(lockLogPath());
    if (f.open(QIODevice::Append)) f.write(data);
}

const QVector<Locker>& lockerList() {
    static QVector<Locker> list;
    list.clear();
    if (hasExecutable("hyprlock")) {
        // 1) il design completo (o il file dell'utente via [lock] hyprlock_config)
        const QString cfg = PowerActions::hyprlockConfigPath();
        if (!cfg.isEmpty()) list.push_back({"hyprlock", {"-c", cfg}});
        // 2) configurazione minimale, GL software: se (1) è andata in crash,
        //    questa è la più probabile a sopravvivere su una GPU instabile o
        //    una macchina virtuale
        const QString safe = writeManaged(QDir::homePath() + "/.config/nicoos/hyprlock-safe.conf",
                                          renderHyprlockConfig(kHyprlockSafeTemplate));
        if (!safe.isEmpty()) list.push_back({"hyprlock", {"-c", safe}, true});
        // 3) la configurazione hyprlock dell'utente, se esiste. (Senza alcuna
        //    configurazione hyprlock >= 0.9 si rifiuta del tutto di partire,
        //    quindi un tentativo "hyprlock" nudo fallirebbe con "Could not
        //    find config".)
        const QString own = QDir::homePath() + "/.config/hypr/hyprlock.conf";
        if (QFileInfo::exists(own)) list.push_back({"hyprlock", {}});
    }
    list.push_back({"swaylock", {"-f", "-c", "0a0f14"}});  // -f: fork dopo il blocco
    list.push_back({"gtklock", {}});
    return list;
}

// Ambiente per i processi che avviamo. Non affidarti mai a ciò che questa
// shell ha ereditato all'avvio: dopo un riavvio di Hyprland (o quando la shell
// è stata lanciata da un'altra sessione) WAYLAND_DISPLAY / la signature
// dell'istanza possono essere vecchi, e hyprlock non riesce a raggiungere il
// compositore.
QProcessEnvironment liveEnvironment(bool forceSoftware = false) {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    auto& ipc = HyprlandIPC::instance();
    env.insert("XDG_RUNTIME_DIR", HyprlandIPC::runtimeDir());

    const QString sig = ipc.instanceSignature();
    if (!sig.isEmpty()) env.insert("HYPRLAND_INSTANCE_SIGNATURE", sig);

    QString display = ipc.waylandDisplay();
    if (display.isEmpty()) {
        const QString inherited = env.value("WAYLAND_DISPLAY");
        if (!inherited.isEmpty() && QFileInfo::exists(HyprlandIPC::runtimeDir() + "/" + inherited)) {
            display = inherited;
        } else {
            const auto socks = QDir(HyprlandIPC::runtimeDir())
                                   .entryList({"wayland-*"}, QDir::System | QDir::Files, QDir::Name);
            for (const QString& name : socks) {
                if (!name.endsWith(".lock")) { display = name; break; }
            }
        }
    }
    if (!display.isEmpty()) env.insert("WAYLAND_DISPLAY", display);

    // GL software (llvmpipe): serve quando la GPU (virtuale) non riesce a
    // importare i buffer che hyprlock disegna, cosa che fa interrompere la
    // connessione al compositore.
    const QString mode = ConfigManager::instance().lockSoftwareRender();
    const bool software = forceSoftware || mode == "yes" || mode == "true" || mode == "1" ||
                          ((mode.isEmpty() || mode == "auto") && runningInVirtualMachine());
    if (software && mode != "no" && mode != "false" && mode != "0") {
        env.insert("LIBGL_ALWAYS_SOFTWARE", "1");
    }
    return env;
}

void startDetachedLive(const QString& program, const QStringList& args) {
    QProcess p;
    p.setProgram(program);
    p.setArguments(args);
    p.setProcessEnvironment(liveEnvironment());
    p.startDetached();
}

// Vero se un bloccante con questo esatto nome di processo è già in esecuzione
// (lo schermo è bloccato: non ne sovrapporre un secondo).
bool isRunning(const QString& program) {
    return QProcess::execute("pgrep", {"-x", program}) == 0;
}

} // namespace

QString PowerActions::hyprlockConfigPath() {
    auto& cfg = ConfigManager::instance();

    // 1) il file dell'utente, usato così com'è
    const QString custom = cfg.lockConfigPath();
    if (!custom.isEmpty()) {
        QString expanded = custom;
        if (expanded.startsWith("~/")) expanded = QDir::homePath() + expanded.mid(1);
        if (QFileInfo::exists(expanded)) return expanded;
        qWarning() << "[PowerActions] [lock] hyprlock_config non trovato:" << expanded
                   << "- uso la configurazione generata";
    }

    // 2) il file generato, a meno che l'utente non l'abbia preso in mano
    //    rimuovendo il marcatore
    return writeManaged(QDir::homePath() + "/.config/nicoos/hyprlock.conf", renderHyprlockConfig());
}

PowerActions::PowerActions(QObject* parent) : QObject(parent) {}

void PowerActions::trigger(const QString& action) {
    if (action == "lock") {
        lock();
    } else if (action == "suspend") {
        suspend();
    } else if (action == "logout") {
        logout();
    } else if (action == "reboot") {
        reboot();
    } else if (action == "poweroff") {
        poweroff();
    } else {
        qWarning() << "[PowerActions] richiesta un'azione sconosciuta:" << action;
        emit actionFailed(action, "azione sconosciuta");
    }
}

bool PowerActions::hasLocker() const {
    return hasExecutable("hyprlock") || hasExecutable("swaylock") || hasExecutable("gtklock");
}

void PowerActions::lock() {
    // Evita che il log cresca all'infinito.
    if (QFileInfo(lockLogPath()).size() > 256 * 1024) QFile::remove(lockLogPath());
    startLocker(0, {});
}

void PowerActions::startLocker(int index, QStringList failures) {
    const QVector<Locker> list = lockerList();
    while (index < list.size() && !hasExecutable(list[index].program)) ++index;
    if (index >= list.size()) {
        lockUnavailable(failures);
        return;
    }

    const Locker locker = list[index];
    if (isRunning(locker.program)) return;  // già bloccato

    // Volutamente SENZA questo oggetto come genitore: se la shell viene
    // riavviata mentre lo schermo è bloccato, il bloccante deve restare in
    // esecuzione.
    auto* p = new QProcess;
    p->setProcessChannelMode(QProcess::MergedChannels);
    auto clock = std::make_shared<QElapsedTimer>();
    clock->start();
    auto tail = std::make_shared<QByteArray>();  // ultimi KB di output

    const QProcessEnvironment env = liveEnvironment(locker.forceSoftware);
    p->setProcessEnvironment(env);
    appendLog(QStringLiteral("\n=== %1 %2 %3 ===\nWAYLAND_DISPLAY=%4 HYPRLAND_INSTANCE_SIGNATURE=%5 LIBGL_ALWAYS_SOFTWARE=%6\n")
                  .arg(QDateTime::currentDateTime().toString(Qt::ISODate), locker.program,
                       locker.args.join(' '), env.value("WAYLAND_DISPLAY"),
                       env.value("HYPRLAND_INSTANCE_SIGNATURE"),
                       env.value("LIBGL_ALWAYS_SOFTWARE", "-")).toUtf8());

    connect(p, &QProcess::readyRead, this, [p, tail] {
        const QByteArray data = p->readAll();
        appendLog(data);
        tail->append(data);
        if (tail->size() > 4096) tail->remove(0, tail->size() - 4096);
    });

    connect(p, &QProcess::finished, this,
            [this, p, clock, tail, index, failures, locker](int code, QProcess::ExitStatus status) mutable {
                const QByteArray rest = p->readAll();
                if (!rest.isEmpty()) { appendLog(rest); tail->append(rest); }
                p->deleteLater();

                // Uscita 0 = sbloccato normalmente (anche swaylock -f esce con
                // 0 dopo essere passato in background).
                if (status == QProcess::NormalExit && code == 0) {
                    m_respawns.clear();
                    return;
                }

                const QString output = QString::fromUtf8(*tail).trimmed();
                const QString reason = output.section('\n', -1).left(160);
                qWarning() << "[PowerActions]" << locker.program << "è terminato in modo anomalo:" << code
                           << "stato" << status << output.right(300);

                if (clock->elapsed() < 2500) {
                    // Fallito all'avvio (config errata, file PAM mancante, ...):
                    // passa al bloccante successivo.
                    failures << QString("%1: %2").arg(
                        locker.program,
                        reason.isEmpty() ? QString("terminato con codice %1").arg(code) : reason);
                    startLocker(index + 1, failures);
                    return;
                }

                // Ha girato per un po' e poi è morto, ad esempio è andato in
                // crash con una password errata. Il compositore tiene la
                // sessione bloccata, quindi avvia un bloccante nuovo invece di
                // lasciare uno schermo morto.
                const qint64 now = QDateTime::currentMSecsSinceEpoch();
                m_respawns.erase(std::remove_if(m_respawns.begin(), m_respawns.end(),
                                                [now](qint64 t) { return now - t > 60000; }),
                                 m_respawns.end());
                if (m_respawns.size() >= 5) {
                    failures << QString("%1: si e' chiuso troppe volte (vedi ~/.cache/nicoos/lock.log)")
                                    .arg(locker.program);
                    m_respawns.clear();
                    startLocker(index + 1, failures);
                    return;
                }
                m_respawns.push_back(now);
                QTimer::singleShot(300, this, [this, index, failures] { startLocker(index, failures); });
            });
    connect(p, &QProcess::errorOccurred, this,
            [this, p, index, failures, locker](QProcess::ProcessError err) mutable {
                if (err != QProcess::FailedToStart) return;
                p->deleteLater();
                failures << locker.program + ": impossibile avviarlo";
                startLocker(index + 1, failures);
            });

    p->start(locker.program, locker.args);
}

void PowerActions::lockUnavailable(const QStringList& failures) {
    // Ultima risorsa: qualcosa potrebbe essere in ascolto del segnale Lock di
    // logind.
    QProcess::startDetached("loginctl", {"lock-session"});

    if (failures.isEmpty()) {
        notifyUser("Nessun blocco schermo installato",
                   "Installa hyprlock (o swaylock): sudo pacman -S hyprlock");
        emit actionFailed("lock", "nessun blocco schermo installato");
    } else {
        notifyUser("Blocco schermo non riuscito",
                   failures.join('\n') + "\nLog: ~/.cache/nicoos/lock.log");
        emit actionFailed("lock", failures.join("; "));
    }
}

void PowerActions::suspend() {
    // Blocca prima, così la sessione è già bloccata quando la macchina si
    // risveglia.
    const bool lockerAvailable = hasLocker();
    lock();
    QTimer::singleShot(lockerAvailable ? 1200 : 0, this, [this] {
        if (!QProcess::startDetached("systemctl", {"suspend"})) {
            emit actionFailed("suspend", "impossibile invocare systemctl");
        }
    });
}

void PowerActions::logout() {
    // Sessione avviata tramite uwsm: va fermata tramite uwsm, altrimenti le
    // sue unità systemd restano attive.
    if (hasExecutable("uwsm") && QProcess::execute("uwsm", {"check", "is-active"}) == 0) {
        QProcess::startDetached("uwsm", {"stop"});
        scheduleLogoutFallbacks();
        return;
    }

    // Hyprland chiude il socket appena esce, quindi qui non si aspetta alcuna
    // risposta "ok" (che dispatch() restituisca false è normale in caso di
    // successo).
    HyprlandIPC::instance().dispatch("exit");
    scheduleLogoutFallbacks();
}

// Se siamo ancora vivi dopo questi ritardi, il compositore non è uscito,
// quindi si sale di livello. Quando il compositore esce davvero, questo
// processo muore con lui e nessuno dei timer scatta mai.
void PowerActions::scheduleLogoutFallbacks() {
    QTimer::singleShot(1500, this, [] {
        if (hasExecutable("hyprctl")) startDetachedLive("hyprctl", {"dispatch", "exit"});
    });
    QTimer::singleShot(3000, this, [] {
        const QString sid = QProcessEnvironment::systemEnvironment().value("XDG_SESSION_ID");
        if (!sid.isEmpty() && hasExecutable("loginctl")) {
            QProcess::startDetached("loginctl", {"terminate-session", sid});
        }
    });
    QTimer::singleShot(4500, this, [] {
        // Ultima risorsa: chiedi di uscire al processo esatto del compositore.
        const qint64 pid = HyprlandIPC::instance().compositorPid();
        if (pid > 1) ::kill(static_cast<pid_t>(pid), SIGTERM);
        else QProcess::startDetached("pkill", {"-x", "Hyprland"});
    });
    QTimer::singleShot(7000, this, [this] {
        notifyUser("Uscita non riuscita", "Hyprland non si e' chiuso. Prova da terminale: hyprctl dispatch exit");
        emit actionFailed("logout", "il compositore non è uscito");
    });
}

void PowerActions::reboot() {
    if (!QProcess::startDetached("systemctl", {"reboot"})) {
        emit actionFailed("reboot", "impossibile invocare systemctl");
    }
}

void PowerActions::poweroff() {
    if (!QProcess::startDetached("systemctl", {"poweroff"})) {
        emit actionFailed("poweroff", "impossibile invocare systemctl");
    }
}

} // namespace nicoos
