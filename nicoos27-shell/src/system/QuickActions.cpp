#include "nicoos/QuickActions.hpp"
#include "nicoos/Notify.hpp"
#include "nicoos/ProcessUtil.hpp"
#include "nicoos/ShellController.hpp"

#include <QDateTime>
#include <QDesktopServices>
#include <QFileInfo>
#include <QUrl>
#include <QDir>
#include <QProcess>
#include <QStandardPaths>
#include <csignal>
#include <sys/types.h>

namespace nicoos {

QuickActions::QuickActions(QObject* parent) : QObject(parent) {
    if (hasProfiles()) refreshProfile();
}

QuickActions::~QuickActions() {
    // I figli sono parentati a questo oggetto, quindi Qt li distrugge;
    // assicurati che i programmi ausiliari si fermino davvero (e che la
    // registrazione venga finalizzata) prima che cio' accada.
    if (m_recorder) {
        ::kill(static_cast<pid_t>(m_recorder->processId()), SIGINT);
        m_recorder->waitForFinished(2000);
    }
    if (m_night) { m_night->kill(); m_night->waitForFinished(300); }
    if (m_caffeine) { m_caffeine->kill(); m_caffeine->waitForFinished(300); }
}

bool QuickActions::hasNightLight() const { return hasExecutable("hyprsunset") || hasExecutable("wlsunset"); }
bool QuickActions::hasCaffeine() const { return hasExecutable("systemd-inhibit"); }
bool QuickActions::hasProfiles() const { return hasExecutable("powerprofilesctl"); }
bool QuickActions::hasScreenshot() const { return hasExecutable("grim"); }
bool QuickActions::hasRecorder() const { return hasExecutable("wf-recorder"); }

void QuickActions::notify(const QString& title, const QString& body) {
    notifyUser(title, body);
}

// Avvia un programma ausiliario di lunga durata e ne tiene il QProcess
// in `slot` finche' gira. All'uscita, lo slot viene prima azzerato e poi
// eseguito onChange(), cosi' i getter delle proprieta' riportano gia' il
// nuovo stato.
void QuickActions::startManaged(QProcess*& slot, const QString& program, const QStringList& args,
                                std::function<void()> onChange) {
    auto* p = new QProcess(this);
    slot = p;
    auto cleanup = [this, p, &slot, onChange] {
        if (slot == p) slot = nullptr;
        p->deleteLater();
        if (onChange) onChange();
    };
    connect(p, &QProcess::finished, this, [cleanup](int, QProcess::ExitStatus) { cleanup(); });
    connect(p, &QProcess::errorOccurred, this, [cleanup](QProcess::ProcessError err) {
        if (err == QProcess::FailedToStart) cleanup();
    });
    p->start(program, args);
}

// ---- Non disturbare --------------------------------------------------

void QuickActions::toggleDoNotDisturb() {
    m_dnd = !m_dnd;
    emit doNotDisturbChanged();
}

// ---- Luce notturna ---------------------------------------------------

void QuickActions::toggleNightLight() {
    if (m_night) {
        m_night->terminate();  // il gestore di fine processo azzera lo stato
        return;
    }
    QString program;
    QStringList args;
    if (hasExecutable("hyprsunset")) {
        program = "hyprsunset";
        args = {"-t", "4000"};
    } else if (hasExecutable("wlsunset")) {
        // Circa 4000 K costanti: temperatura minima e massima quasi uguali.
        program = "wlsunset";
        args = {"-t", "4000", "-T", "4001", "-S", "07:00", "-s", "19:00"};
    } else {
        notify("Luce notturna non disponibile", "Installa hyprsunset o wlsunset");
        return;
    }
    startManaged(m_night, program, args, [this] { emit nightLightChanged(); });
    emit nightLightChanged();
}

// ---- Caffeina --------------------------------------------------------

void QuickActions::toggleCaffeine() {
    if (m_caffeine) {
        m_caffeine->terminate();
        return;
    }
    if (!hasCaffeine()) {
        notify("Caffeina non disponibile", "systemd-inhibit non trovato");
        return;
    }
    startManaged(m_caffeine, "systemd-inhibit",
                 {"--what=idle:sleep", "--who=NicoOS", "--why=Caffeina", "sleep", "infinity"},
                 [this] { emit caffeineChanged(); });
    emit caffeineChanged();
}

// ---- Profilo energetico ---------------------------------------------

void QuickActions::refreshProfile() {
    runCapture(this, "powerprofilesctl", {"get"}, [this](int code, const QString& out) {
        if (code != 0) return;
        const QString value = out.trimmed();
        if (value != m_profile) {
            m_profile = value;
            emit powerProfileChanged();
        }
    });
}

void QuickActions::setProfile(const QString& profile) {
    runCapture(this, "powerprofilesctl", {"set", profile}, [this, profile](int code, const QString&) {
        // Molti portatili non hanno il profilo "performance": ricadi su power-saver.
        if (code != 0 && profile == "performance") {
            setProfile("power-saver");
            return;
        }
        refreshProfile();
    });
}

void QuickActions::cycleProfile() {
    if (!hasProfiles()) {
        notify("Profili energetici non disponibili", "Installa power-profiles-daemon");
        return;
    }
    const QStringList order{"power-saver", "balanced", "performance"};
    const int idx = order.indexOf(m_profile);
    setProfile(order[(idx + 1) % order.size()]);
}

// ---- Istantanea dello schermo ---------------------------------------

void QuickActions::screenshot(bool region) {
    if (!hasScreenshot() || (region && !hasExecutable("slurp"))) {
        notify("Screenshot non disponibile", "Installa grim e slurp");
        return;
    }
    ShellController::instance().hideAll();  // tiene i popup fuori dall'immagine

    const QString dir =
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/Screenshots";
    QDir().mkpath(dir);
    const QString file =
        dir + "/" + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss") + ".png";

    // $1 e' il file di output; passato come argomento cosi' non serve quoting.
    const QString tail = R"(command -v wl-copy >/dev/null 2>&1 && wl-copy < "$1";
command -v notify-send >/dev/null 2>&1 && notify-send -a NicoOS -i "$1" "Screenshot salvato" "$1";
exit 0)";
    const QString script = region
        ? QString(R"(sleep 0.25; geom=$(slurp) || exit 0; grim -g "$geom" "$1" || exit 1; )") + tail
        : QString(R"(sleep 0.35; grim "$1" || exit 1; )") + tail;

    QProcess::startDetached("sh", {"-c", script, "sh", file});
}

// ---- Registrazione dello schermo ------------------------------------

void QuickActions::toggleRecording() {
    if (m_recorder) {
        // wf-recorder finalizza il file alla SIGINT.
        ::kill(static_cast<pid_t>(m_recorder->processId()), SIGINT);
        return;
    }
    if (!hasRecorder()) {
        notify("Registrazione non disponibile", "Installa wf-recorder");
        return;
    }
    ShellController::instance().hideAll();

    const QString dir =
        QStandardPaths::writableLocation(QStandardPaths::MoviesLocation) + "/Recordings";
    QDir().mkpath(dir);
    m_recordPath = dir + "/" + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss") + ".mp4";

    startManaged(m_recorder, "wf-recorder", {"-f", m_recordPath}, [this] {
        emit recordingChanged();
        const QFileInfo info(m_recordPath);
        if (info.exists() && info.size() > 0) {
            notify("Registrazione salvata", m_recordPath);
            // Aprilo subito nel lettore video predefinito. xdg-open viene
            // usato direttamente (funziona con qualsiasi plugin di
            // piattaforma Qt); QDesktopServices e mpv sono i ripieghi.
            bool opened = false;
            if (hasExecutable("xdg-open")) {
                opened = QProcess::startDetached("xdg-open", {m_recordPath});
            }
            if (!opened) opened = QDesktopServices::openUrl(QUrl::fromLocalFile(m_recordPath));
            if (!opened && hasExecutable("mpv")) QProcess::startDetached("mpv", {m_recordPath});
        } else {
            notify("Registrazione non riuscita", "Nessun video creato");
        }
    });
    emit recordingChanged();
}

// ---- Azioni con nome (socket di controllo / scorciatoie) --------------

bool QuickActions::runAction(const QString& action) {
    if (action == "dnd.toggle") toggleDoNotDisturb();
    else if (action == "nightlight.toggle") toggleNightLight();
    else if (action == "caffeine.toggle") toggleCaffeine();
    else if (action == "profile.cycle") cycleProfile();
    else if (action == "screenshot.region") screenshot(true);
    else if (action == "screenshot.full") screenshot(false);
    else if (action == "record.toggle") toggleRecording();
    else return false;
    return true;
}

} // namespace nicoos
