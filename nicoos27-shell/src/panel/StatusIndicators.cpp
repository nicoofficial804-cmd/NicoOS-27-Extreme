#include "nicoos/StatusIndicators.hpp"
#include "nicoos/NetworkManagerClient.hpp"
#include "nicoos/AudioClient.hpp"
#include "nicoos/SystemMonitor.hpp"

namespace nicoos {

StatusIndicators::StatusIndicators(NetworkManagerClient* network, AudioClient* audio,
                                    SystemMonitor* monitor, QObject* parent)
    : QObject(parent), m_network(network), m_audio(audio), m_monitor(monitor) {
    connect(network, &NetworkManagerClient::stateChanged, this, &StatusIndicators::changed);
    connect(audio, &AudioClient::volumeChanged, this, &StatusIndicators::changed);
    connect(monitor, &SystemMonitor::statsUpdated, this, &StatusIndicators::changed);
}

QString StatusIndicators::networkGlyph() const {
    return m_network->isConnected() ? "\uE9E1" : "\uE9E3";
}

QString StatusIndicators::audioGlyph() const {
    return m_audio->isMuted() ? "\uE9E7" : "\uE9E5";
}

QString StatusIndicators::batteryGlyph() const {
    if (!m_monitor->hasBattery()) return QString();
    return m_monitor->batteryCharging() ? "\uE9F0" : "\uE9EF";
}

} // namespace nicoos
