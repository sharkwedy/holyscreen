#include "app/contexts/BibleContext.h"

#include "app/ApplicationController.h"

namespace churchpresenter {

BibleContext::BibleContext(ApplicationController &controller, QObject *parent)
    : QObject(parent), m_controller(controller)
{
    connect(&controller, &ApplicationController::bibleTranslationsChanged,
            this, &BibleContext::bibleTranslationsChanged);
    connect(&controller, &ApplicationController::bibleSelectionChanged,
            this, &BibleContext::bibleSelectionChanged);
    connect(&controller, &ApplicationController::bibleResultsChanged,
            this, &BibleContext::bibleResultsChanged);
    connect(&controller, &ApplicationController::favoriteBibleVersesChanged,
            this, &BibleContext::favoriteBibleVersesChanged);
    connect(&controller, &ApplicationController::historyChanged,
            this, &BibleContext::bibleHistoryChanged);
    connect(&controller, &ApplicationController::bibleImportStateChanged,
            this, &BibleContext::bibleImportStateChanged);
    connect(&controller, &ApplicationController::statusMessageChanged,
            this, &BibleContext::statusMessageChanged);
}

QVariantList BibleContext::bibleTranslations() const { return m_controller.bibleTranslations(); }
QVariantList BibleContext::bibleBooks() const { return m_controller.bibleBooks(); }
QString BibleContext::biblePrimaryTranslationId() const { return m_controller.biblePrimaryTranslationId(); }
QString BibleContext::bibleSecondaryTranslationId() const { return m_controller.bibleSecondaryTranslationId(); }
QString BibleContext::bibleTertiaryTranslationId() const { return m_controller.bibleTertiaryTranslationId(); }
void BibleContext::setBiblePrimaryTranslationId(const QString &id) { m_controller.setBiblePrimaryTranslationId(id); }
void BibleContext::setBibleSecondaryTranslationId(const QString &id) { m_controller.setBibleSecondaryTranslationId(id); }
void BibleContext::setBibleTertiaryTranslationId(const QString &id) { m_controller.setBibleTertiaryTranslationId(id); }
QString BibleContext::bibleReferenceInput() const { return m_controller.bibleReferenceInput(); }
void BibleContext::setBibleReferenceInput(const QString &value) { m_controller.setBibleReferenceInput(value); }
QVariantList BibleContext::bibleResults() const { return m_controller.bibleResults(); }
QVariantList BibleContext::favoriteBibleVerses() const
{
    return m_controller.favoriteBibleVerses();
}
QVariantList BibleContext::bibleHistory() const
{
    QVariantList result;
    for (const auto &entryValue : m_controller.history()) {
        const auto entry = entryValue.toMap();
        if (entry.value(QStringLiteral("type")).toString() == QStringLiteral("bible"))
            result.append(entry);
    }
    return result;
}
bool BibleContext::bibleImportRunning() const { return m_controller.bibleImportRunning(); }
int BibleContext::bibleImportProgress() const { return m_controller.bibleImportProgress(); }
QString BibleContext::bibleImportMessage() const { return m_controller.bibleImportMessage(); }
bool BibleContext::bibleImportRequiresLicenseConfirmation() const { return m_controller.bibleImportRequiresLicenseConfirmation(); }
QString BibleContext::bibleImportLicenseWarning() const { return m_controller.bibleImportLicenseWarning(); }
QString BibleContext::statusMessage() const { return m_controller.statusMessage(); }
int BibleContext::importBibleTranslation(const QUrl &source) { return m_controller.importBibleTranslation(source); }
bool BibleContext::importBibleFolder(const QUrl &folder) { return m_controller.importBibleFolder(folder); }
bool BibleContext::importBibleGit(const QString &url) { return m_controller.importBibleGit(url); }
bool BibleContext::importBibleZip(const QString &url) { return m_controller.importBibleZip(url); }
bool BibleContext::confirmBibleImportLicenses() { return m_controller.confirmBibleImportLicenses(); }
void BibleContext::cancelBibleImport() { m_controller.cancelBibleImport(); }
bool BibleContext::updateBibleTranslationFromSource(const QString &translationId)
{
    return m_controller.updateBibleTranslationFromSource(translationId);
}
bool BibleContext::searchBibleReference() { return m_controller.searchBibleReference(); }
void BibleContext::showBibleVerse(int index) { m_controller.showBibleVerse(index); }
void BibleContext::toggleFavoriteBibleVerse(int index)
{
    m_controller.toggleFavoriteBibleVerse(index);
}
QVariantList BibleContext::bibleChapterNumbers(int bookId) const { return m_controller.bibleChapterNumbers(bookId); }
QVariantList BibleContext::bibleVerseNumbers(int bookId, int chapter) const { return m_controller.bibleVerseNumbers(bookId, chapter); }
bool BibleContext::presentBibleReference(int bookId, int chapter, int verse) { return m_controller.presentBibleReference(bookId, chapter, verse); }
QString BibleContext::bibleTextForSlide(int slideIndex, const QString &translationId) const
{
    return m_controller.bibleTextForSlide(slideIndex, translationId);
}
QVariantList BibleContext::compareBibleReference(const QString &reference) const
{
    return m_controller.compareBibleReference(reference);
}
bool BibleContext::presentBibleHistory(const QString &reference)
{
    const auto normalizedReference = reference.trimmed();
    if (normalizedReference.isEmpty()) return false;
    m_controller.setBibleReferenceInput(normalizedReference);
    if (!m_controller.searchBibleReference()) return false;
    m_controller.showBibleVerse(0);
    return true;
}

} // namespace churchpresenter
