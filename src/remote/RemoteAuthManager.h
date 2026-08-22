#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QHash>
#include <QString>
#include <QVariantMap>

#include <functional>

namespace churchpresenter {

struct RemoteLoginResult {
    bool accepted = false;
    QString token;
    QString errorCode;
    QDateTime expiresAt;
};

class RemoteAuthManager final {
public:
    static constexpr int DefaultIterations = 600000;
    static constexpr int SessionHours = 8;

    explicit RemoteAuthManager(std::function<QDateTime()> clock = {});
    [[nodiscard]] QVariantMap setPassword(const QString &password,
                                          int iterations = DefaultIterations);
    bool loadCredentials(const QVariantMap &credentials);
    [[nodiscard]] bool hasCredentials() const;
    [[nodiscard]] RemoteLoginResult login(const QString &password, const QString &clientKey);
    [[nodiscard]] bool validateToken(const QString &token);
    [[nodiscard]] bool authorizeCommand(const QString &token);
    [[nodiscard]] QByteArray sessionHash(const QString &token);
    [[nodiscard]] bool validateSession(const QByteArray &sessionHash);
    [[nodiscard]] bool authorizeSession(const QByteArray &sessionHash);
    bool logout(const QString &token);
    void revokeAll();
    [[nodiscard]] int sessionCount() const;

private:
    struct AttemptState {
        QList<QDateTime> failures;
        QDateTime blockedUntil;
    };
    struct SessionState {
        QDateTime expiresAt;
        QList<QDateTime> commandTimes;
    };
    [[nodiscard]] QDateTime now() const;
    void removeExpiredSessions();
    QByteArray m_salt;
    QByteArray m_passwordHash;
    int m_iterations = 0;
    int m_version = 0;
    QHash<QString, AttemptState> m_attempts;
    QHash<QByteArray, SessionState> m_sessions;
    std::function<QDateTime()> m_clock;
};

} // namespace churchpresenter
