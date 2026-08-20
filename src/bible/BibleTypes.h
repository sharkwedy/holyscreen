#pragma once

#include <QString>
#include <QVector>

namespace churchpresenter {

enum class BibleBook {
    Unknown,
    Genesis, Exodus, Leviticus, Numbers, Deuteronomy,
    Joshua, Judges, Ruth, FirstSamuel, SecondSamuel,
    FirstKings, SecondKings, FirstChronicles, SecondChronicles,
    Ezra, Nehemiah, Esther, Job, Psalms, Proverbs, Ecclesiastes,
    SongOfSongs, Isaiah, Jeremiah, Lamentations, Ezekiel, Daniel,
    Hosea, Joel, Amos, Obadiah, Jonah, Micah, Nahum, Habakkuk,
    Zephaniah, Haggai, Zechariah, Malachi,
    Matthew, Mark, Luke, John, Acts, Romans,
    FirstCorinthians, SecondCorinthians, Galatians, Ephesians,
    Philippians, Colossians, FirstThessalonians, SecondThessalonians,
    FirstTimothy, SecondTimothy, Titus, Philemon, Hebrews, James,
    FirstPeter, SecondPeter, FirstJohn, SecondJohn, ThirdJohn,
    Jude, Revelation,
};

struct BibleReference {
    BibleBook book = BibleBook::Unknown;
    int chapter = 0;
    int firstVerse = 0;
    int lastVerse = 0;

    [[nodiscard]] bool isValid() const
    {
        return book != BibleBook::Unknown && chapter > 0 && firstVerse > 0
            && lastVerse >= firstVerse;
    }

    bool operator==(const BibleReference &) const = default;
};

struct BibleTranslation {
    QString id;
    QString name;
    QString abbreviation;
    QString language;

    bool operator==(const BibleTranslation &) const = default;
};

struct BibleVerse {
    QString translationId;
    BibleBook book = BibleBook::Unknown;
    int chapter = 0;
    int verse = 0;
    QString text;

    bool operator==(const BibleVerse &) const = default;
};

[[nodiscard]] QString bibleBookName(BibleBook book);

} // namespace churchpresenter
