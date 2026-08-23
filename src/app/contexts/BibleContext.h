#pragma once

#include <QObject>
#include <QVariantList>

namespace churchpresenter {

class ApplicationController;

class BibleContext final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList bibleTranslations READ bibleTranslations NOTIFY bibleTranslationsChanged)
    Q_PROPERTY(QVariantList bibleBooks READ bibleBooks CONSTANT)
    Q_PROPERTY(QString bibleReferenceInput READ bibleReferenceInput WRITE setBibleReferenceInput NOTIFY bibleSelectionChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit BibleContext(ApplicationController &controller, QObject *parent = nullptr);

    [[nodiscard]] QVariantList bibleTranslations() const;
    [[nodiscard]] QVariantList bibleBooks() const;
    [[nodiscard]] QString bibleReferenceInput() const;
    void setBibleReferenceInput(const QString &reference);
    [[nodiscard]] QString statusMessage() const;

    Q_INVOKABLE bool searchBibleReference();
    Q_INVOKABLE void showBibleVerse(int index);
    Q_INVOKABLE QVariantList bibleChapterNumbers(int bookId) const;
    Q_INVOKABLE QVariantList bibleVerseNumbers(int bookId, int chapter) const;
    Q_INVOKABLE bool presentBibleReference(int bookId, int chapter, int verse);

signals:
    void bibleTranslationsChanged();
    void bibleSelectionChanged();
    void statusMessageChanged();

private:
    ApplicationController &m_controller;
};

} // namespace churchpresenter
