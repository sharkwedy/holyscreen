#include "remote/RemoteAuthManager.h"

#include <QCryptographicHash>
#include <QPasswordDigestor>
#include <QRandomGenerator>

#include <algorithm>
#include <cstring>

namespace {

QByteArray randomBytes(int size)
{
    QByteArray result(size, Qt::Uninitialized);
    auto *random = QRandomGenerator::system();
    for (int offset = 0; offset < size; offset += static_cast<int>(sizeof(quint64))) {
        const auto value = random->generate64();
        const auto count = std::min(static_cast<int>(sizeof(value)), size - offset);
        std::memcpy(result.data() + offset, &value, static_cast<size_t>(count));
    }
    return result;
}

QByteArray derivePassword(const QString &password, const QByteArray &salt, int iterations)
{
    return QPasswordDigestor::deriveKeyPbkdf2(QCryptographicHash::Sha256,
                                               password.toUtf8(), salt,
                                               iterations, 32);
}

QByteArray tokenHash(const QString &token)
{
    return QCryptographicHash::hash(token.toUtf8(), QCryptographicHash::Sha256);
}

bool constantTimeEqual(const QByteArray &left, const QByteArray &right)
{
    if (left.size() != right.size()) return false;
    unsigned char difference = 0;
    for (qsizetype index = 0; index < left.size(); ++index)
        difference |= static_cast<unsigned char>(left[index] ^ right[index]);
    return difference == 0;
}

} // namespace

namespace churchpresenter {

RemoteAuthManager::RemoteAuthManager(std::function<QDateTime()> clock)
    : m_clock(std::move(clock))
{
    if (!m_clock) m_clock = [] { return QDateTime::currentDateTimeUtc(); };
}

QVariantMap RemoteAuthManager::setPassword(const QString &password, int iterations)
{
    if (password.isEmpty() || iterations < 1000) return {};
    m_salt = randomBytes(16);
    m_passwordHash = derivePassword(password, m_salt, iterations);
    m_iterations = iterations;
    m_version = 1;
    m_sessions.clear();
    m_attempts.clear();
    return {
        {QStringLiteral("version"), m_version},
        {QStringLiteral("algorithm"), QStringLiteral("PBKDF2-HMAC-SHA256")},
        {QStringLiteral("iterations"), m_iterations},
        {QStringLiteral("salt"), m_salt},
        {QStringLiteral("hash"), m_passwordHash},
    };
}

bool RemoteAuthManager::loadCredentials(const QVariantMap &credentials)
{
    const auto version = credentials.value(QStringLiteral("version")).toInt();
    const auto algorithm = credentials.value(QStringLiteral("algorithm")).toString();
    const auto iterations = credentials.value(QStringLiteral("iterations")).toInt();
    const auto salt = credentials.value(QStringLiteral("salt")).toByteArray();
    const auto hash = credentials.value(QStringLiteral("hash")).toByteArray();
    if (version != 1 || algorithm != QStringLiteral("PBKDF2-HMAC-SHA256")
        || iterations < 1000 || salt.size() != 16 || hash.size() != 32) return false;
    m_version = version;
    m_iterations = iterations;
    m_salt = salt;
    m_passwordHash = hash;
    revokeAll();
    return true;
}

bool RemoteAuthManager::hasCredentials() const
{
    return m_version == 1 && m_iterations >= 1000 && m_salt.size() == 16
        && m_passwordHash.size() == 32;
}

RemoteLoginResult RemoteAuthManager::login(const QString &password, const QString &clientKey)
{
    removeExpiredSessions();
    if (!hasCredentials()) return {.errorCode = QStringLiteral("password_not_configured")};
    const auto current = now();
    const auto key = clientKey.trimmed().isEmpty() ? QStringLiteral("unknown") : clientKey.trimmed();
    auto &attempt = m_attempts[key];
    if (attempt.blockedUntil.isValid() && current < attempt.blockedUntil)
        return {.errorCode = QStringLiteral("login_blocked")};
    attempt.failures.removeIf([&current](const QDateTime &failure) {
        return failure.secsTo(current) > 5 * 60;
    });
    const auto candidate = derivePassword(password, m_salt, m_iterations);
    if (!constantTimeEqual(candidate, m_passwordHash)) {
        attempt.failures.append(current);
        if (attempt.failures.size() >= 5) {
            attempt.blockedUntil = current.addSecs(15 * 60);
            attempt.failures.clear();
        }
        return {.errorCode = QStringLiteral("invalid_credentials")};
    }
    attempt = {};
    const auto rawToken = randomBytes(32);
    const auto token = QString::fromLatin1(rawToken.toBase64(
        QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
    const auto expiresAt = current.addSecs(SessionHours * 3600);
    m_sessions.insert(tokenHash(token), {.expiresAt = expiresAt});
    return {.accepted = true, .token = token, .expiresAt = expiresAt};
}

bool RemoteAuthManager::validateToken(const QString &token)
{
    removeExpiredSessions();
    if (token.isEmpty()) return false;
    return m_sessions.contains(tokenHash(token));
}

bool RemoteAuthManager::authorizeCommand(const QString &token)
{
    return authorizeSession(tokenHash(token));
}

QByteArray RemoteAuthManager::sessionHash(const QString &token)
{
    removeExpiredSessions();
    const auto hash = tokenHash(token);
    return m_sessions.contains(hash) ? hash : QByteArray{};
}

bool RemoteAuthManager::validateSession(const QByteArray &sessionHash)
{
    removeExpiredSessions();
    return !sessionHash.isEmpty() && m_sessions.contains(sessionHash);
}

bool RemoteAuthManager::authorizeSession(const QByteArray &sessionHash)
{
    removeExpiredSessions();
    auto iterator = m_sessions.find(sessionHash);
    if (iterator == m_sessions.end()) return false;
    const auto current = now();
    iterator->commandTimes.removeIf([&current](const QDateTime &issuedAt) {
        return issuedAt.msecsTo(current) >= 1000;
    });
    if (iterator->commandTimes.size() >= 30) return false;
    iterator->commandTimes.append(current);
    return true;
}

bool RemoteAuthManager::logout(const QString &token)
{
    return !token.isEmpty() && m_sessions.remove(tokenHash(token));
}

void RemoteAuthManager::revokeAll()
{
    m_sessions.clear();
}

int RemoteAuthManager::sessionCount() const
{
    const auto current = now();
    int count = 0;
    for (const auto &session : m_sessions) if (current < session.expiresAt) ++count;
    return count;
}

QDateTime RemoteAuthManager::now() const
{
    return m_clock().toUTC();
}

void RemoteAuthManager::removeExpiredSessions()
{
    const auto current = now();
    for (auto iterator = m_sessions.begin(); iterator != m_sessions.end();) {
        if (current >= iterator->expiresAt) iterator = m_sessions.erase(iterator);
        else ++iterator;
    }
}

} // namespace churchpresenter
