#include "screens/ScreenManager.h"

#include <QCryptographicHash>
#include <QGuiApplication>
#include <QScreen>

#include <algorithm>

namespace churchpresenter {

ScreenManager::ScreenManager(IScreenProvider &provider, QObject *parent)
    : QObject(parent)
    , m_provider(provider)
{
    qRegisterMetaType<ScreenDescriptor>();
    connect(&m_provider, &IScreenProvider::screensChanged, this, &ScreenManager::refresh);
    refresh();
}

const QVector<ScreenDescriptor> &ScreenManager::screens() const
{
    return m_screens;
}

void ScreenManager::refresh()
{
    const auto updated = m_provider.screens();
    bool changed = updated.size() != m_screens.size();

    for (const auto &screen : updated) {
        const auto previous = std::find_if(m_screens.cbegin(), m_screens.cend(), [&](const auto &item) {
            return item.fingerprint == screen.fingerprint;
        });
        if (previous == m_screens.cend()) {
            emit screenConnected(screen);
            changed = true;
        } else if (previous->geometry != screen.geometry
                   || previous->devicePixelRatio != screen.devicePixelRatio
                   || previous->primary != screen.primary) {
            changed = true;
        }
    }

    for (const auto &screen : m_screens) {
        const auto stillPresent = std::any_of(updated.cbegin(), updated.cend(), [&](const auto &item) {
            return item.fingerprint == screen.fingerprint;
        });
        if (!stillPresent) {
            emit screenDisconnected(screen.fingerprint);
            changed = true;
        }
    }

    m_screens = updated;
    if (changed) {
        emit screenConfigurationChanged();
    }
}

QtScreenProvider::QtScreenProvider(QObject *parent)
    : IScreenProvider(parent)
{
    for (auto *screen : QGuiApplication::screens()) {
        watch(screen);
    }
    connect(qGuiApp, &QGuiApplication::screenAdded, this, [this](QScreen *screen) {
        watch(screen);
        emit screensChanged();
    });
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, [this](QScreen *) {
        emit screensChanged();
    });
    connect(qGuiApp, &QGuiApplication::primaryScreenChanged, this, [this](QScreen *) {
        emit screensChanged();
    });
}

QVector<ScreenDescriptor> QtScreenProvider::screens() const
{
    QVector<ScreenDescriptor> result;
    for (auto *screen : QGuiApplication::screens()) {
        const auto geometry = screen->geometry();
        const auto primary = screen == QGuiApplication::primaryScreen();
        result.append(ScreenDescriptor{
            .id = screen->name(),
            .fingerprint = fingerprintFor(screen),
            .displayName = QStringLiteral("%1 — %2×%3")
                .arg(screen->name())
                .arg(geometry.width())
                .arg(geometry.height()),
            .geometry = geometry,
            .resolution = geometry.size(),
            .devicePixelRatio = screen->devicePixelRatio(),
            .primary = primary,
            .connected = true,
            .nativeScreen = screen,
        });
    }
    return result;
}

QString QtScreenProvider::fingerprintFor(QScreen *screen)
{
    QStringList identity{
        screen->manufacturer(), screen->model(), screen->serialNumber(), screen->name(),
    };
    if (screen->serialNumber().isEmpty()) {
        identity << QStringLiteral("%1x%2mm").arg(screen->physicalSize().width()).arg(screen->physicalSize().height());
    }
    const auto digest = QCryptographicHash::hash(identity.join(QLatin1Char('|')).toUtf8(),
                                                  QCryptographicHash::Sha256);
    return QString::fromLatin1(digest.toHex());
}

void QtScreenProvider::watch(QScreen *screen)
{
    connect(screen, &QScreen::geometryChanged, this, [this](const QRect &) { emit screensChanged(); });
    connect(screen, &QScreen::physicalDotsPerInchChanged, this, [this](qreal) { emit screensChanged(); });
}

} // namespace churchpresenter
