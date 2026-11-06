#pragma once

#include <QObject>
#include <QString>
#include <functional>

class QProcess;

namespace nicoos {

// Attivazioni e azioni singole mostrate come tessere nel centro di controllo
// (e associabili ai tasti tramite il socket di controllo). Tutto passa da
// strumenti ben noti e degrada con grazia: le proprietà has* dicono al QML
// quali tessere attenuare quando uno strumento non è installato.
//
//   Non disturbare    -> silenzia i popup delle notifiche (quelle critiche passano)
//   Luce notturna     -> hyprsunset o wlsunset
//   Caffeina          -> systemd-inhibit (blocca inattività/sospensione)
//   Profilo energetico -> powerprofilesctl (power-profiles-daemon)
//   Screenshot        -> grim (+ slurp per un'area), copiato con wl-copy
//   Registrazione     -> wf-recorder
class QuickActions : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool doNotDisturb READ doNotDisturb NOTIFY doNotDisturbChanged)
    Q_PROPERTY(bool nightLight READ nightLight NOTIFY nightLightChanged)
    Q_PROPERTY(bool caffeine READ caffeine NOTIFY caffeineChanged)
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)
    Q_PROPERTY(QString powerProfile READ powerProfile NOTIFY powerProfileChanged)
    Q_PROPERTY(bool hasNightLight READ hasNightLight CONSTANT)
    Q_PROPERTY(bool hasCaffeine READ hasCaffeine CONSTANT)
    Q_PROPERTY(bool hasProfiles READ hasProfiles CONSTANT)
    Q_PROPERTY(bool hasScreenshot READ hasScreenshot CONSTANT)
    Q_PROPERTY(bool hasRecorder READ hasRecorder CONSTANT)

public:
    explicit QuickActions(QObject* parent = nullptr);
    ~QuickActions() override;

    bool doNotDisturb() const { return m_dnd; }
    bool nightLight() const { return m_night != nullptr; }
    bool caffeine() const { return m_caffeine != nullptr; }
    bool recording() const { return m_recorder != nullptr; }
    QString powerProfile() const { return m_profile; }

    bool hasNightLight() const;
    bool hasCaffeine() const;
    bool hasProfiles() const;
    bool hasScreenshot() const;
    bool hasRecorder() const;

    Q_INVOKABLE void toggleDoNotDisturb();
    Q_INVOKABLE void toggleNightLight();
    Q_INVOKABLE void toggleCaffeine();
    Q_INVOKABLE void cycleProfile();
    Q_INVOKABLE void screenshot(bool region);
    Q_INVOKABLE void toggleRecording();

    // Gestisce nomi come "dnd.toggle" o "screenshot.region"; restituisce false
    // se l'azione non è tra le nostre.
    bool runAction(const QString& action);

signals:
    void doNotDisturbChanged();
    void nightLightChanged();
    void caffeineChanged();
    void recordingChanged();
    void powerProfileChanged();

private:
    void startManaged(QProcess*& slot, const QString& program, const QStringList& args,
                      std::function<void()> onChange);
    void refreshProfile();
    void setProfile(const QString& profile);
    void notify(const QString& title, const QString& body = QString());

    bool m_dnd{false};
    QString m_profile;
    QString m_recordPath;
    QProcess* m_night{nullptr};
    QProcess* m_caffeine{nullptr};
    QProcess* m_recorder{nullptr};
};

} // namespace nicoos
