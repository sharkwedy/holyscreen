#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QHash>
#include <QVariantList>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QNetworkReply)

namespace churchpresenter {

class OnlineLyricsService final : public QObject {
    Q_OBJECT

public:
    explicit OnlineLyricsService(QObject *parent = nullptr);

    [[nodiscard]] QVariantList results() const;
    [[nodiscard]] bool busy() const;
    [[nodiscard]] QString error() const;
    [[nodiscard]] QVariantMap result(const QString &key) const;

    void setVagalumeApiKey(const QString &apiKey);
    void search(const QString &query);
    void cancel();
    void loadLyrics(const QString &key);
    void markSaved(const QString &key);

    [[nodiscard]] static QVariantList parseLrclibSearch(const QByteArray &payload,
                                                        QString *error = nullptr);
    [[nodiscard]] static QVariantList parseVagalumeSearch(const QByteArray &payload,
                                                          QString *error = nullptr);
    [[nodiscard]] static QString parseVagalumeLyrics(const QByteArray &payload,
                                                     QString *error = nullptr);
    [[nodiscard]] static QString toStructuredLyrics(const QString &plainLyrics);

signals:
    void changed();
    void lyricsLoaded(const QString &key);
    void lyricsLoadFailed(const QString &key, const QString &message);

private:
    using Completion = std::function<void(const QByteArray &, const QString &, int)>;

    QNetworkReply *getJson(const QUrl &url, Completion completion);
    void searchLrclib(const QString &query, quint64 generation);
    void searchVagalume(const QString &query, quint64 generation,
                        const QString &lrclibFailure = {});
    void finishSearch(const QVariantList &results, const QString &error = {});
    bool updateResult(const QString &key, const QString &lyrics);

    QNetworkAccessManager m_network;
    QVariantList m_results;
    QString m_error;
    QString m_vagalumeApiKey;
    QString m_query;
    bool m_searching = false;
    bool m_loading = false;
    quint64 m_searchGeneration = 0;
    quint64 m_loadGeneration = 0;
    QPointer<QNetworkReply> m_searchReply;
    QPointer<QNetworkReply> m_loadReply;
    struct CacheEntry {
        QVariantList results;
        qint64 storedAtMs = 0;
    };
    QHash<QString, CacheEntry> m_cache;
};

} // namespace churchpresenter
