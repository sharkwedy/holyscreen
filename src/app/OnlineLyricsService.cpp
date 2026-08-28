#include "app/OnlineLyricsService.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrlQuery>

#include <memory>
#include <algorithm>

namespace churchpresenter {

namespace {
constexpr qsizetype maximumPayloadBytes = 2 * 1024 * 1024;
constexpr int maximumResults = 20;
constexpr qint64 cacheLifetimeMs = 10 * 60 * 1000;
constexpr int maximumCacheEntries = 30;

QString clientHeader()
{
    auto version = QCoreApplication::applicationVersion().trimmed();
    if (version.isEmpty()) version = QStringLiteral("development");
    return QStringLiteral("HolyScreen/%1 (https://github.com/sharkwedy/holyscreen)")
        .arg(version);
}

QString errorFromReply(QNetworkReply *reply, int status)
{
    if (status == 429) {
        const auto retryAfter = QString::fromUtf8(reply->rawHeader("Retry-After")).trimmed();
        return retryAfter.isEmpty()
            ? QObject::tr("A fonte limitou temporariamente as pesquisas.")
            : QObject::tr("A fonte limitou as pesquisas. Tente novamente em %1 segundos.")
                  .arg(retryAfter);
    }
    if (status >= 400) return QObject::tr("A fonte respondeu com HTTP %1.").arg(status);
    return reply->errorString();
}

QString plainFromSynced(QString synced)
{
    static const QRegularExpression timestamp(
        QStringLiteral(R"(^\s*(?:\[[0-9]{1,3}:[0-9]{2}(?:[.:][0-9]{1,3})?\])+\s*)"));
    auto lines = synced.replace(QStringLiteral("\r\n"), QStringLiteral("\n"))
                     .replace(u'\r', u'\n').split(u'\n');
    for (auto &line : lines) line.remove(timestamp);
    return lines.join(u'\n').trimmed();
}

QString normalizedSectionLabel(QString label, int &verseNumber)
{
    label = label.trimmed();
    const auto lower = label.toLower();
    if (lower.startsWith(QStringLiteral("verse"))
        || lower.startsWith(QStringLiteral("verso"))) {
        static const QRegularExpression number(QStringLiteral("(\\d+)$"));
        const auto match = number.match(lower);
        if (match.hasMatch()) verseNumber = qMax(verseNumber, match.captured(1).toInt() + 1);
        else label = QStringLiteral("VERSO %1").arg(verseNumber++);
    } else if (lower == QStringLiteral("chorus") || lower == QStringLiteral("refrain")
               || lower == QStringLiteral("refrão") || lower == QStringLiteral("refrao")) {
        label = QStringLiteral("REFRÃO");
    } else if (lower == QStringLiteral("pre-chorus") || lower == QStringLiteral("pre chorus")
               || lower == QStringLiteral("pré-refrão") || lower == QStringLiteral("pre-refrao")) {
        label = QStringLiteral("PRÉ-REFRÃO");
    } else if (lower == QStringLiteral("bridge") || lower == QStringLiteral("ponte")) {
        label = QStringLiteral("PONTE");
    } else if (lower == QStringLiteral("intro") || lower == QStringLiteral("introduction")) {
        label = QStringLiteral("INTRO");
    } else if (lower == QStringLiteral("outro") || lower == QStringLiteral("ending")
               || lower == QStringLiteral("final")) {
        label = QStringLiteral("FINAL");
    }
    return label.toUpper();
}

QVariantMap lrclibResult(const QJsonObject &object)
{
    const auto id = object.value(QStringLiteral("id")).toVariant().toString();
    const auto title = object.value(QStringLiteral("trackName")).toString(
        object.value(QStringLiteral("name")).toString()).trimmed();
    const auto artist = object.value(QStringLiteral("artistName")).toString().trimmed();
    auto lyrics = object.value(QStringLiteral("plainLyrics")).toString().trimmed();
    const auto synced = object.value(QStringLiteral("syncedLyrics")).toString().trimmed();
    if (lyrics.isEmpty() && !synced.isEmpty()) lyrics = plainFromSynced(synced);
    return {
        {QStringLiteral("key"), QStringLiteral("lrclib:%1").arg(id)},
        {QStringLiteral("provider"), QStringLiteral("LRCLIB")},
        {QStringLiteral("providerId"), id},
        {QStringLiteral("title"), title},
        {QStringLiteral("artist"), artist},
        {QStringLiteral("album"), object.value(QStringLiteral("albumName")).toString()},
        {QStringLiteral("durationSeconds"), object.value(QStringLiteral("duration")).toDouble()},
        {QStringLiteral("lyrics"), lyrics},
        {QStringLiteral("syncedLyrics"), synced},
        {QStringLiteral("instrumental"), object.value(QStringLiteral("instrumental")).toBool()},
        {QStringLiteral("hasLyrics"), !lyrics.isEmpty()},
        {QStringLiteral("sourceUrl"), QStringLiteral("https://lrclib.net/api/get/%1").arg(id)},
        {QStringLiteral("savedCount"), 0},
    };
}

} // namespace

OnlineLyricsService::OnlineLyricsService(QObject *parent)
    : QObject(parent)
{
}

QVariantList OnlineLyricsService::results() const { return m_results; }
bool OnlineLyricsService::busy() const { return m_searching || m_loading; }
QString OnlineLyricsService::error() const { return m_error; }

QVariantMap OnlineLyricsService::result(const QString &key) const
{
    for (const auto &value : m_results) {
        const auto item = value.toMap();
        if (item.value(QStringLiteral("key")).toString() == key) return item;
    }
    return {};
}

void OnlineLyricsService::setVagalumeApiKey(const QString &apiKey)
{
    m_vagalumeApiKey = apiKey.trimmed();
}

void OnlineLyricsService::cancel()
{
    ++m_searchGeneration;
    ++m_loadGeneration;
    if (m_searchReply) m_searchReply->abort();
    if (m_loadReply) m_loadReply->abort();
    const bool wasBusy = busy();
    m_searching = false;
    m_loading = false;
    if (wasBusy) emit changed();
}

void OnlineLyricsService::search(const QString &query)
{
    const auto normalized = query.simplified();
    ++m_searchGeneration;
    if (m_searchReply) m_searchReply->abort();
    m_searchReply.clear();
    m_query = normalized;
    m_results.clear();
    m_error.clear();
    m_searching = normalized.size() >= 3;
    const auto cacheKey = normalized.toCaseFolded();
    const auto cached = m_cache.constFind(cacheKey);
    if (m_searching && cached != m_cache.cend()
        && QDateTime::currentMSecsSinceEpoch() - cached->storedAtMs <= cacheLifetimeMs) {
        m_results = cached->results;
        m_searching = false;
    }
    emit changed();
    if (!m_searching) return;
    searchLrclib(normalized, m_searchGeneration);
}

void OnlineLyricsService::searchLrclib(const QString &query, quint64 generation)
{
    QUrl url(QStringLiteral("https://lrclib.net/api/search"));
    QUrlQuery urlQuery;
    urlQuery.addQueryItem(QStringLiteral("q"), query);
    url.setQuery(urlQuery);
    m_searchReply = getJson(url, [this, query, generation](const QByteArray &payload,
                                                         const QString &networkError, int) {
        if (generation != m_searchGeneration) return;
        QString parseError;
        const auto parsed = networkError.isEmpty() ? parseLrclibSearch(payload, &parseError)
                                                    : QVariantList{};
        const auto failure = networkError.isEmpty() ? parseError : networkError;
        if (!parsed.isEmpty()) {
            finishSearch(parsed);
            return;
        }
        searchVagalume(query, generation, failure);
    });
}

void OnlineLyricsService::searchVagalume(const QString &query, quint64 generation,
                                         const QString &lrclibFailure)
{
    if (m_vagalumeApiKey.isEmpty()) {
        finishSearch({}, lrclibFailure.isEmpty()
            ? tr("Nenhuma letra encontrada na LRCLIB. Configure a chave do Vagalume para usar o fallback.")
            : tr("LRCLIB: %1 O fallback Vagalume não está configurado.").arg(lrclibFailure));
        return;
    }
    QUrl url(QStringLiteral("https://api.vagalume.com.br/search.excerpt"));
    QUrlQuery urlQuery;
    urlQuery.addQueryItem(QStringLiteral("q"), query);
    urlQuery.addQueryItem(QStringLiteral("limit"), QString::number(maximumResults));
    url.setQuery(urlQuery);
    m_searchReply = getJson(url, [this, generation, lrclibFailure](const QByteArray &payload,
                                                                 const QString &networkError, int) {
        if (generation != m_searchGeneration) return;
        QString parseError;
        const auto parsed = networkError.isEmpty() ? parseVagalumeSearch(payload, &parseError)
                                                    : QVariantList{};
        QString failure = networkError.isEmpty() ? parseError : networkError;
        if (failure.isEmpty() && parsed.isEmpty()) failure = tr("Nenhuma letra encontrada.");
        if (!lrclibFailure.isEmpty() && !failure.isEmpty()) {
            failure = tr("LRCLIB: %1 Vagalume: %2").arg(lrclibFailure, failure);
        }
        finishSearch(parsed, parsed.isEmpty() ? failure : QString{});
    });
}

void OnlineLyricsService::finishSearch(const QVariantList &results, const QString &error)
{
    m_results = results;
    m_error = error;
    m_searching = false;
    m_searchReply.clear();
    if (!results.isEmpty()) {
        if (m_cache.size() >= maximumCacheEntries) m_cache.erase(m_cache.begin());
        m_cache.insert(m_query.toCaseFolded(),
                       CacheEntry{results, QDateTime::currentMSecsSinceEpoch()});
    }
    emit changed();
}

void OnlineLyricsService::loadLyrics(const QString &key)
{
    const auto item = result(key);
    if (item.isEmpty()) {
        emit lyricsLoadFailed(key, tr("O resultado selecionado não existe mais."));
        return;
    }
    if (item.value(QStringLiteral("hasLyrics")).toBool()) {
        emit lyricsLoaded(key);
        return;
    }
    if (item.value(QStringLiteral("provider")).toString() != QStringLiteral("Vagalume")) {
        emit lyricsLoadFailed(key, tr("A fonte não forneceu a letra completa."));
        return;
    }
    if (m_vagalumeApiKey.isEmpty()) {
        emit lyricsLoadFailed(key, tr("Configure a chave de API do Vagalume."));
        return;
    }

    ++m_loadGeneration;
    if (m_loadReply) m_loadReply->abort();
    const auto generation = m_loadGeneration;
    auto providerId = item.value(QStringLiteral("providerId")).toString();
    if (providerId.size() > 1) providerId.remove(0, 1);
    QUrl url(QStringLiteral("https://api.vagalume.com.br/search.php"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("musid"), providerId);
    query.addQueryItem(QStringLiteral("apikey"), m_vagalumeApiKey);
    url.setQuery(query);

    m_loading = true;
    m_error.clear();
    emit changed();
    m_loadReply = getJson(url, [this, generation, key](const QByteArray &payload,
                                                      const QString &networkError, int) {
        if (generation != m_loadGeneration) return;
        m_loading = false;
        m_loadReply.clear();
        QString parseError;
        const auto lyrics = networkError.isEmpty() ? parseVagalumeLyrics(payload, &parseError)
                                                    : QString{};
        const auto failure = networkError.isEmpty() ? parseError : networkError;
        if (lyrics.isEmpty() || !updateResult(key, lyrics)) {
            m_error = failure.isEmpty() ? tr("O Vagalume não forneceu a letra completa.") : failure;
            emit changed();
            emit lyricsLoadFailed(key, m_error);
            return;
        }
        m_error.clear();
        emit changed();
        emit lyricsLoaded(key);
    });
}

void OnlineLyricsService::markSaved(const QString &key)
{
    for (int index = 0; index < m_results.size(); ++index) {
        auto item = m_results.at(index).toMap();
        if (item.value(QStringLiteral("key")).toString() != key) continue;
        item.insert(QStringLiteral("savedCount"),
                    item.value(QStringLiteral("savedCount")).toInt() + 1);
        m_results[index] = item;
        emit changed();
        return;
    }
}

bool OnlineLyricsService::updateResult(const QString &key, const QString &lyrics)
{
    for (int index = 0; index < m_results.size(); ++index) {
        auto item = m_results.at(index).toMap();
        if (item.value(QStringLiteral("key")).toString() != key) continue;
        item.insert(QStringLiteral("lyrics"), lyrics.trimmed());
        item.insert(QStringLiteral("hasLyrics"), !lyrics.trimmed().isEmpty());
        m_results[index] = item;
        return true;
    }
    return false;
}

QNetworkReply *OnlineLyricsService::getJson(const QUrl &url, Completion completion)
{
    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("User-Agent", clientHeader().toUtf8());
    request.setTransferTimeout(10'000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::SameOriginRedirectPolicy);
    auto *reply = m_network.get(request);
    auto payload = std::make_shared<QByteArray>();
    auto tooLarge = std::make_shared<bool>(false);
    connect(reply, &QIODevice::readyRead, reply, [reply, payload, tooLarge] {
        payload->append(reply->readAll());
        if (payload->size() > maximumPayloadBytes) {
            *tooLarge = true;
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::finished, this,
            [reply, payload, tooLarge, completion = std::move(completion)] {
        payload->append(reply->readAll());
        const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QString error;
        if (*tooLarge || payload->size() > maximumPayloadBytes) {
            error = QObject::tr("A resposta da fonte ultrapassou 2 MiB.");
        } else if (reply->error() != QNetworkReply::NoError) {
            error = errorFromReply(reply, status);
        } else if (status != 200) {
            error = errorFromReply(reply, status);
        }
        completion(*payload, error, status);
        reply->deleteLater();
    });
    return reply;
}

QVariantList OnlineLyricsService::parseLrclibSearch(const QByteArray &payload, QString *error)
{
    if (error) error->clear();
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isArray()) {
        if (error) *error = tr("Resposta inválida da LRCLIB.");
        return {};
    }
    QVariantList results;
    for (const auto &value : document.array()) {
        if (!value.isObject() || results.size() >= maximumResults) continue;
        const auto mapped = lrclibResult(value.toObject());
        if (mapped.value(QStringLiteral("providerId")).toString().isEmpty()
            || mapped.value(QStringLiteral("title")).toString().isEmpty()
            || mapped.value(QStringLiteral("artist")).toString().isEmpty()
            || mapped.value(QStringLiteral("instrumental")).toBool()
            || !mapped.value(QStringLiteral("hasLyrics")).toBool()) continue;
        results.append(mapped);
    }
    return results;
}

QVariantList OnlineLyricsService::parseVagalumeSearch(const QByteArray &payload, QString *error)
{
    if (error) error->clear();
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(payload, &parseError);
    const auto response = document.object().value(QStringLiteral("response")).toObject();
    const auto documents = response.value(QStringLiteral("docs")).toArray();
    if (parseError.error != QJsonParseError::NoError || !document.isObject()
        || !response.contains(QStringLiteral("docs"))) {
        if (error) *error = tr("Resposta inválida do Vagalume.");
        return {};
    }
    QVariantList results;
    for (const auto &value : documents) {
        if (!value.isObject() || results.size() >= maximumResults) continue;
        const auto object = value.toObject();
        const auto id = object.value(QStringLiteral("id")).toVariant().toString();
        const auto title = object.value(QStringLiteral("title")).toString().trimmed();
        const auto artist = object.value(QStringLiteral("band")).toString().trimmed();
        if (id.isEmpty() || title.isEmpty() || artist.isEmpty()) continue;
        QUrl source(QStringLiteral("https://www.vagalume.com.br/search"));
        QUrlQuery sourceQuery;
        sourceQuery.addQueryItem(QStringLiteral("q"), QStringLiteral("%1 %2").arg(title, artist));
        source.setQuery(sourceQuery);
        results.append(QVariantMap{
            {QStringLiteral("key"), QStringLiteral("vagalume:%1").arg(id)},
            {QStringLiteral("provider"), QStringLiteral("Vagalume")},
            {QStringLiteral("providerId"), id},
            {QStringLiteral("title"), title},
            {QStringLiteral("artist"), artist},
            {QStringLiteral("album"), QString{}},
            {QStringLiteral("durationSeconds"), 0},
            {QStringLiteral("lyrics"), QString{}},
            {QStringLiteral("syncedLyrics"), QString{}},
            {QStringLiteral("instrumental"), false},
            {QStringLiteral("hasLyrics"), false},
            {QStringLiteral("sourceUrl"), source.toString()},
            {QStringLiteral("savedCount"), 0},
        });
    }
    return results;
}

QString OnlineLyricsService::parseVagalumeLyrics(const QByteArray &payload, QString *error)
{
    if (error) error->clear();
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error) *error = tr("Resposta inválida do Vagalume.");
        return {};
    }
    const auto songs = document.object().value(QStringLiteral("mus")).toArray();
    if (songs.isEmpty() || !songs.first().isObject()) {
        if (error) *error = tr("Letra não encontrada no Vagalume.");
        return {};
    }
    const auto lyrics = songs.first().toObject().value(QStringLiteral("text")).toString().trimmed();
    if (lyrics.isEmpty() && error) *error = tr("Letra vazia retornada pelo Vagalume.");
    return lyrics;
}

QString OnlineLyricsService::toStructuredLyrics(const QString &plainLyrics)
{
    auto normalized = plainLyrics;
    normalized.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    normalized.replace(u'\r', u'\n');
    normalized = plainFromSynced(normalized);
    static const QRegularExpression metadataLine(
        QStringLiteral(R"(^\s*\[(?:ar|al|ti|by|offset|length):[^\]]*\]\s*$)"),
        QRegularExpression::CaseInsensitiveOption);
    auto lines = normalized.split(u'\n');
    for (int index = lines.size() - 1; index >= 0; --index) {
        if (metadataLine.match(lines.at(index)).hasMatch()) lines.removeAt(index);
    }
    normalized = lines.join(u'\n').trimmed();
    if (normalized.isEmpty()) return {};

    static const QRegularExpression sectionHeader(QStringLiteral(R"(^\s*\[([^\]]+)\]\s*$)"));
    const bool hasHeaders = std::any_of(lines.cbegin(), lines.cend(), [](const QString &line) {
        return sectionHeader.match(line).hasMatch();
    });
    QStringList blocks;
    int verseNumber = 1;
    if (hasHeaders) {
        QString label;
        QStringList content;
        const auto flush = [&] {
            const auto text = content.join(u'\n').trimmed();
            if (text.isEmpty()) return;
            if (label.isEmpty()) label = QStringLiteral("VERSO %1").arg(verseNumber++);
            blocks.append(QStringLiteral("%1\n%2").arg(label, text));
            content.clear();
        };
        for (const auto &line : lines) {
            const auto match = sectionHeader.match(line);
            if (match.hasMatch()) {
                flush();
                label = normalizedSectionLabel(match.captured(1), verseNumber);
            } else {
                content.append(line);
            }
        }
        flush();
    } else {
        const auto paragraphs = normalized.split(QRegularExpression(QStringLiteral("\\n\\s*\\n")),
                                                 Qt::SkipEmptyParts);
        for (const auto &paragraph : paragraphs) {
            auto paragraphLines = paragraph.split(u'\n', Qt::SkipEmptyParts);
            while (!paragraphLines.isEmpty()) {
                const auto take = qMin(4, paragraphLines.size());
                QStringList chunk;
                for (int index = 0; index < take; ++index) chunk.append(paragraphLines.takeFirst().trimmed());
                blocks.append(QStringLiteral("VERSO %1\n%2")
                                  .arg(verseNumber++).arg(chunk.join(u'\n')));
            }
        }
    }
    return blocks.join(QStringLiteral("\n\n"));
}

} // namespace churchpresenter
