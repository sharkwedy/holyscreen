#pragma once

#include <QObject>
#include <QUrl>
#include <QVariantList>

namespace churchpresenter {

class ApplicationController;

class BibleContext final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList bibleTranslations READ bibleTranslations NOTIFY bibleTranslationsChanged)
    Q_PROPERTY(QVariantList bibleBooks READ bibleBooks CONSTANT)
    Q_PROPERTY(QString biblePrimaryTranslationId READ biblePrimaryTranslationId WRITE setBiblePrimaryTranslationId NOTIFY bibleSelectionChanged)
    Q_PROPERTY(QString bibleSecondaryTranslationId READ bibleSecondaryTranslationId WRITE setBibleSecondaryTranslationId NOTIFY bibleSelectionChanged)
    Q_PROPERTY(QString bibleTertiaryTranslationId READ bibleTertiaryTranslationId WRITE setBibleTertiaryTranslationId NOTIFY bibleSelectionChanged)
    Q_PROPERTY(QString bibleReferenceInput READ bibleReferenceInput WRITE setBibleReferenceInput NOTIFY bibleSelectionChanged)
    Q_PROPERTY(QVariantList bibleResults READ bibleResults NOTIFY bibleResultsChanged)
    Q_PROPERTY(QVariantList favoriteBibleVerses READ favoriteBibleVerses NOTIFY favoriteBibleVersesChanged)
    Q_PROPERTY(bool bibleImportRunning READ bibleImportRunning NOTIFY bibleImportStateChanged)
    Q_PROPERTY(int bibleImportProgress READ bibleImportProgress NOTIFY bibleImportStateChanged)
    Q_PROPERTY(QString bibleImportMessage READ bibleImportMessage NOTIFY bibleImportStateChanged)
    Q_PROPERTY(bool bibleImportRequiresLicenseConfirmation READ bibleImportRequiresLicenseConfirmation NOTIFY bibleImportStateChanged)
    Q_PROPERTY(QString bibleImportLicenseWarning READ bibleImportLicenseWarning NOTIFY bibleImportStateChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit BibleContext(ApplicationController &controller, QObject *parent = nullptr);

    [[nodiscard]] QVariantList bibleTranslations() const;
    [[nodiscard]] QVariantList bibleBooks() const;
    [[nodiscard]] QString biblePrimaryTranslationId() const;
    [[nodiscard]] QString bibleSecondaryTranslationId() const;
    [[nodiscard]] QString bibleTertiaryTranslationId() const;
    void setBiblePrimaryTranslationId(const QString &id);
    void setBibleSecondaryTranslationId(const QString &id);
    void setBibleTertiaryTranslationId(const QString &id);
    [[nodiscard]] QString bibleReferenceInput() const;
    void setBibleReferenceInput(const QString &reference);
    [[nodiscard]] QVariantList bibleResults() const;
    [[nodiscard]] QVariantList favoriteBibleVerses() const;
    [[nodiscard]] bool bibleImportRunning() const;
    [[nodiscard]] int bibleImportProgress() const;
    [[nodiscard]] QString bibleImportMessage() const;
    [[nodiscard]] bool bibleImportRequiresLicenseConfirmation() const;
    [[nodiscard]] QString bibleImportLicenseWarning() const;
    [[nodiscard]] QString statusMessage() const;

    Q_INVOKABLE int importBibleTranslation(const QUrl &source);
    Q_INVOKABLE bool importBibleFolder(const QUrl &folder);
    Q_INVOKABLE bool importBibleGit(const QString &url);
    Q_INVOKABLE bool importBibleZip(const QString &url);
    Q_INVOKABLE bool confirmBibleImportLicenses();
    Q_INVOKABLE void cancelBibleImport();
    Q_INVOKABLE bool updateBibleTranslationFromSource(const QString &translationId);
    Q_INVOKABLE bool searchBibleReference();
    Q_INVOKABLE void showBibleVerse(int index);
    Q_INVOKABLE void toggleFavoriteBibleVerse(int index);
    Q_INVOKABLE QVariantList bibleChapterNumbers(int bookId) const;
    Q_INVOKABLE QVariantList bibleVerseNumbers(int bookId, int chapter) const;
    Q_INVOKABLE bool presentBibleReference(int bookId, int chapter, int verse);
    Q_INVOKABLE QString bibleTextForSlide(int slideIndex, const QString &translationId) const;

signals:
    void bibleTranslationsChanged();
    void bibleSelectionChanged();
    void bibleResultsChanged();
    void favoriteBibleVersesChanged();
    void bibleImportStateChanged();
    void statusMessageChanged();

private:
    ApplicationController &m_controller;
};

} // namespace churchpresenter
