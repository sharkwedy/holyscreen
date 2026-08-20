#include "bible/BibleReferenceParser.h"

#include <QHash>
#include <QRegularExpression>
#include <QVector>

namespace churchpresenter {
namespace {

struct BookDefinition {
    BibleBook book;
    QString name;
    QStringList aliases;
};

QString normalizedBookKey(QString value)
{
    value = value.normalized(QString::NormalizationForm_C).trimmed().toLower();
    value.remove(QLatin1Char('.'));
    value.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
    return value;
}

const QVector<BookDefinition> &bookDefinitions()
{
    static const QVector<BookDefinition> definitions{
        {BibleBook::Genesis, "Gênesis", {"gn", "gen", "gênesis", "genesis"}},
        {BibleBook::Exodus, "Êxodo", {"ex", "êx", "exodo", "êxodo"}},
        {BibleBook::Leviticus, "Levítico", {"lv", "lev", "levítico", "levitico"}},
        {BibleBook::Numbers, "Números", {"nm", "num", "números", "numeros"}},
        {BibleBook::Deuteronomy, "Deuteronômio", {"dt", "deut", "deuteronômio", "deuteronomio"}},
        {BibleBook::Joshua, "Josué", {"js", "jos", "josué", "josue"}},
        {BibleBook::Judges, "Juízes", {"jz", "juízes", "juizes"}},
        {BibleBook::Ruth, "Rute", {"rt", "rute"}},
        {BibleBook::FirstSamuel, "1 Samuel", {"1 sm", "1samuel", "1 samuel", "i samuel"}},
        {BibleBook::SecondSamuel, "2 Samuel", {"2 sm", "2samuel", "2 samuel", "ii samuel"}},
        {BibleBook::FirstKings, "1 Reis", {"1 rs", "1reis", "1 reis", "i reis"}},
        {BibleBook::SecondKings, "2 Reis", {"2 rs", "2reis", "2 reis", "ii reis"}},
        {BibleBook::FirstChronicles, "1 Crônicas", {"1 cr", "1crônicas", "1 cronicas", "1 crônicas"}},
        {BibleBook::SecondChronicles, "2 Crônicas", {"2 cr", "2crônicas", "2 cronicas", "2 crônicas"}},
        {BibleBook::Ezra, "Esdras", {"ed", "edr", "esdras"}},
        {BibleBook::Nehemiah, "Neemias", {"ne", "nee", "neemias"}},
        {BibleBook::Esther, "Ester", {"et", "est", "ester"}},
        {BibleBook::Job, "Jó", {"jó", "job"}},
        {BibleBook::Psalms, "Salmos", {"sl", "sal", "salmo", "salmos"}},
        {BibleBook::Proverbs, "Provérbios", {"pv", "prov", "provérbios", "proverbios"}},
        {BibleBook::Ecclesiastes, "Eclesiastes", {"ec", "ecl", "eclesiastes"}},
        {BibleBook::SongOfSongs, "Cânticos", {"ct", "cânticos", "canticos", "cantares"}},
        {BibleBook::Isaiah, "Isaías", {"is", "isa", "isaías", "isaias"}},
        {BibleBook::Jeremiah, "Jeremias", {"jr", "jer", "jeremias"}},
        {BibleBook::Lamentations, "Lamentações", {"lm", "lam", "lamentações", "lamentacoes"}},
        {BibleBook::Ezekiel, "Ezequiel", {"ez", "eze", "ezequiel"}},
        {BibleBook::Daniel, "Daniel", {"dn", "dan", "daniel"}},
        {BibleBook::Hosea, "Oseias", {"os", "ose", "oseias"}},
        {BibleBook::Joel, "Joel", {"jl", "joel"}},
        {BibleBook::Amos, "Amós", {"am", "amós", "amos"}},
        {BibleBook::Obadiah, "Obadias", {"ob", "obadias"}},
        {BibleBook::Jonah, "Jonas", {"jn", "jonas"}},
        {BibleBook::Micah, "Miqueias", {"mq", "miq", "miqueias"}},
        {BibleBook::Nahum, "Naum", {"na", "naum"}},
        {BibleBook::Habakkuk, "Habacuque", {"hc", "hab", "habacuque"}},
        {BibleBook::Zephaniah, "Sofonias", {"sf", "sof", "sofonias"}},
        {BibleBook::Haggai, "Ageu", {"ag", "ageu"}},
        {BibleBook::Zechariah, "Zacarias", {"zc", "zac", "zacarias"}},
        {BibleBook::Malachi, "Malaquias", {"ml", "mal", "malaquias"}},
        {BibleBook::Matthew, "Mateus", {"mt", "mat", "mateus"}},
        {BibleBook::Mark, "Marcos", {"mc", "mar", "marcos"}},
        {BibleBook::Luke, "Lucas", {"lc", "luc", "lucas"}},
        {BibleBook::John, "João", {"jo", "joão", "joao", "john"}},
        {BibleBook::Acts, "Atos", {"at", "atos"}},
        {BibleBook::Romans, "Romanos", {"rm", "rom", "romanos"}},
        {BibleBook::FirstCorinthians, "1 Coríntios", {"1 co", "1 cor", "1coríntios", "1 coríntios", "1 corintios"}},
        {BibleBook::SecondCorinthians, "2 Coríntios", {"2 co", "2 cor", "2coríntios", "2 coríntios", "2 corintios"}},
        {BibleBook::Galatians, "Gálatas", {"gl", "gal", "gálatas", "galatas"}},
        {BibleBook::Ephesians, "Efésios", {"ef", "efe", "efésios", "efesios"}},
        {BibleBook::Philippians, "Filipenses", {"fp", "fil", "filipenses"}},
        {BibleBook::Colossians, "Colossenses", {"cl", "col", "colossenses"}},
        {BibleBook::FirstThessalonians, "1 Tessalonicenses", {"1 ts", "1 tess", "1 tessalonicenses"}},
        {BibleBook::SecondThessalonians, "2 Tessalonicenses", {"2 ts", "2 tess", "2 tessalonicenses"}},
        {BibleBook::FirstTimothy, "1 Timóteo", {"1 tm", "1 tim", "1 timóteo", "1 timoteo"}},
        {BibleBook::SecondTimothy, "2 Timóteo", {"2 tm", "2 tim", "2 timóteo", "2 timoteo"}},
        {BibleBook::Titus, "Tito", {"tt", "tito"}},
        {BibleBook::Philemon, "Filemom", {"fm", "filemom", "filêmon", "filemon"}},
        {BibleBook::Hebrews, "Hebreus", {"hb", "heb", "hebreus"}},
        {BibleBook::James, "Tiago", {"tg", "tia", "tiago"}},
        {BibleBook::FirstPeter, "1 Pedro", {"1 pe", "1 pd", "1 pedro"}},
        {BibleBook::SecondPeter, "2 Pedro", {"2 pe", "2 pd", "2 pedro"}},
        {BibleBook::FirstJohn, "1 João", {"1 jo", "1joão", "1 joão", "1 joao"}},
        {BibleBook::SecondJohn, "2 João", {"2 jo", "2joão", "2 joão", "2 joao"}},
        {BibleBook::ThirdJohn, "3 João", {"3 jo", "3joão", "3 joão", "3 joao"}},
        {BibleBook::Jude, "Judas", {"jd", "jud", "judas"}},
        {BibleBook::Revelation, "Apocalipse", {"ap", "apo", "apocalipse"}},
    };
    return definitions;
}

const QHash<QString, BibleBook> &bookAliases()
{
    static const QHash<QString, BibleBook> aliases = [] {
        QHash<QString, BibleBook> result;
        for (const auto &definition : bookDefinitions()) {
            result.insert(normalizedBookKey(definition.name), definition.book);
            for (const auto &alias : definition.aliases) {
                result.insert(normalizedBookKey(alias), definition.book);
            }
        }
        return result;
    }();
    return aliases;
}

} // namespace

std::optional<BibleReference> BibleReferenceParser::parse(const QString &text) const
{
    static const QRegularExpression pattern(QStringLiteral(
        R"(^\s*(.+?)\s+(\d+)\s*(?::|\.|\s)\s*(\d+)(?:\s*[-–—]\s*(\d+))?\s*$)"));
    const auto match = pattern.match(text);
    if (!match.hasMatch()) return std::nullopt;

    const auto book = bookAliases().value(normalizedBookKey(match.captured(1)), BibleBook::Unknown);
    bool chapterOk = false;
    bool firstVerseOk = false;
    bool lastVerseOk = true;
    const auto chapter = match.captured(2).toInt(&chapterOk);
    const auto firstVerse = match.captured(3).toInt(&firstVerseOk);
    const auto lastVerse = match.captured(4).isEmpty()
        ? firstVerse : match.captured(4).toInt(&lastVerseOk);

    const BibleReference reference{book, chapter, firstVerse, lastVerse};
    if (!chapterOk || !firstVerseOk || !lastVerseOk || !reference.isValid()) {
        return std::nullopt;
    }
    return reference;
}

QString bibleBookName(BibleBook book)
{
    for (const auto &definition : bookDefinitions()) {
        if (definition.book == book) return definition.name;
    }
    return {};
}

} // namespace churchpresenter
