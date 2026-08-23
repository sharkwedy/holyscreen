#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

namespace churchpresenter {

class ApplicationController;

class OutputContext final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList screens READ screens NOTIFY screensChanged)
    Q_PROPERTY(QVariantList outputWindows READ outputWindows NOTIFY outputWindowsChanged)
    Q_PROPERTY(bool blackout READ blackout WRITE setBlackout NOTIFY blackoutChanged)
    Q_PROPERTY(bool identifyVisible READ identifyVisible NOTIFY identifyVisibleChanged)
    Q_PROPERTY(bool broadcastTransparencySupported READ broadcastTransparencySupported CONSTANT)
    Q_PROPERTY(QString broadcastTransparencyWarning READ broadcastTransparencyWarning CONSTANT)

public:
    explicit OutputContext(ApplicationController &controller, QObject *parent = nullptr);

    [[nodiscard]] QVariantList screens() const;
    [[nodiscard]] QVariantList outputWindows() const;
    [[nodiscard]] bool blackout() const;
    void setBlackout(bool enabled);
    [[nodiscard]] bool identifyVisible() const;
    [[nodiscard]] bool broadcastTransparencySupported() const;
    [[nodiscard]] QString broadcastTransparencyWarning() const;

    Q_INVOKABLE bool toggleScreen(const QString &screenFingerprint, bool enabled);
    Q_INVOKABLE void enableAllScreens();
    Q_INVOKABLE bool setOutputBibleTranslation(const QString &screenFingerprint,
                                               const QString &translationId);
    Q_INVOKABLE bool setOutputRole(const QString &screenFingerprint, const QString &role);
    Q_INVOKABLE bool setOutputMediaEnabled(const QString &screenFingerprint, bool enabled);
    Q_INVOKABLE QVariantMap outputBroadcastProfile(const QString &screenFingerprint) const;
    Q_INVOKABLE bool setOutputBroadcastProfile(const QString &screenFingerprint,
                                               const QVariantMap &changes);
    Q_INVOKABLE bool setOutputDisplayName(const QString &screenFingerprint,
                                          const QString &displayName);
    Q_INVOKABLE void identifyScreens();

signals:
    void screensChanged();
    void outputWindowsChanged();
    void blackoutChanged();
    void identifyVisibleChanged();

private:
    ApplicationController &m_controller;
};

} // namespace churchpresenter
