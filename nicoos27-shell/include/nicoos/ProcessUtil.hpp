#pragma once

#include <QByteArray>
#include <QFile>
#include <QObject>
#include <QProcess>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
#include <functional>

namespace nicoos {

// Vero se `name` è un eseguibile trovato nel $PATH.
inline bool hasExecutable(const QString& name) {
    return !QStandardPaths::findExecutable(name).isEmpty();
}

// Vero quando si esegue dentro una macchina virtuale (flag CPU x86
// "hypervisor", oppure un noto fornitore virtuale nel DMI). Legge solo /proc e
// /sys, quindi è sicuro chiamarla prima che esista la QGuiApplication. Le GPU
// virtuali (vmwgfx, virtio, QXL...) spesso non riescono a importare i buffer
// con cui disegnano Qt/hyprlock, il che finisce in
// "wl_surface.attach: invalid arguments" e in una connessione persa.
inline bool runningInVirtualMachine() {
    static const bool vm = [] {
        QFile cpu("/proc/cpuinfo");
        if (cpu.open(QIODevice::ReadOnly)) {
            const QByteArray data = cpu.read(16384);
            if (data.contains(" hypervisor")) return true;
        }
        for (const char* path : {"/sys/class/dmi/id/product_name", "/sys/class/dmi/id/sys_vendor"}) {
            QFile f(path);
            if (!f.open(QIODevice::ReadOnly)) continue;
            const QByteArray v = f.readAll().toLower();
            for (const char* needle : {"vmware", "virtualbox", "qemu", "kvm", "virtual machine", "bochs", "parallels", "xen"}) {
                if (v.contains(needle)) return true;
            }
        }
        return false;
    }();
    return vm;
}

// Esegue `program` in modo asincrono e chiama cb(exitCode, stdout) quando
// termina (exitCode == -1 se il programma non è potuto partire affatto).
// Il QProcess ha `owner` come genitore e si elimina da sé in seguito.
inline void runCapture(QObject* owner, const QString& program, const QStringList& args,
                       std::function<void(int, const QString&)> cb) {
    auto* p = new QProcess(owner);
    QObject::connect(p, &QProcess::finished, owner,
                     [p, cb](int code, QProcess::ExitStatus) {
                         const QString out = QString::fromUtf8(p->readAllStandardOutput());
                         p->deleteLater();
                         if (cb) cb(code, out);
                     });
    QObject::connect(p, &QProcess::errorOccurred, owner,
                     [p, cb](QProcess::ProcessError err) {
                         if (err == QProcess::FailedToStart) {
                             p->deleteLater();
                             if (cb) cb(-1, QString());
                         }
                     });
    p->start(program, args);
}

} // namespace nicoos
