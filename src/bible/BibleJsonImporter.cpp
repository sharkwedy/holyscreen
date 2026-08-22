#include "bible/BibleJsonImporter.h"

#include "bible/BibleReferenceParser.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

namespace churchpresenter {

BibleJsonImportResult BibleJsonImporter::parse(const QByteArray &json) const
{
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return {.error = QStringLiteral("JSON inválido: %1").arg(parseError.errorString())};
    }

    const auto root = document.object();
    const auto translationObject = root.value(QStringLiteral("translation")).toObject();
    BibleTranslation translation{
        .id = translationObject.value(QStringLiteral("id")).toString().trimmed(),
        .name = translationObject.value(QStringLiteral("name")).toString().trimmed(),
        .abbreviation = translationObject.value(QStringLiteral("abbreviation")).toString().trimmed().toUpper(),
        .language = translationObject.value(QStringLiteral("language")).toString().trimmed(),
    };
    if (translation.name.isEmpty() || translation.abbreviation.isEmpty() || translation.language.isEmpty()) {
        return {.error = QStringLiteral("Metadados da tradução estão incompletos.")};
    }

    const auto versesArray = root.value(QStringLiteral("verses")).toArray();
    if (versesArray.isEmpty()) {
        return {.error = QStringLiteral("O arquivo não contém versículos.")};
    }

    BibleReferenceParser referenceParser;
    QVector<BibleVerse> verses;
    verses.reserve(versesArray.size());
    for (int index = 0; index < versesArray.size(); ++index) {
        const auto object = versesArray.at(index).toObject();
        BibleBook book = BibleBook::Unknown;
        const auto bookValue = object.value(QStringLiteral("book"));
        if (bookValue.isDouble()) {
            const auto number = bookValue.toInt();
            if (number >= static_cast<int>(BibleBook::Genesis)
                && number <= static_cast<int>(BibleBook::Revelation)) {
                book = static_cast<BibleBook>(number);
            }
        } else if (bookValue.isString()) {
            const auto parsed = referenceParser.parse(
                bookValue.toString() + QStringLiteral(" 1:1"));
            if (parsed.has_value()) book = parsed->book;
        }

        const auto chapter = object.value(QStringLiteral("chapter")).toInt();
        const auto verseNumber = object.value(QStringLiteral("verse")).toInt();
        const auto text = object.value(QStringLiteral("text")).toString().trimmed();
        if (book == BibleBook::Unknown || chapter <= 0 || verseNumber <= 0 || text.isEmpty()) {
            return {.error = QStringLiteral("Versículo inválido na posição %1.").arg(index + 1)};
        }
        verses.append({{}, book, chapter, verseNumber, text});
    }

    return {.translation = std::move(translation), .verses = std::move(verses)};
}

} // namespace churchpresenter
