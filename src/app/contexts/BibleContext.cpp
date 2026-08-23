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
    connect(&controller, &ApplicationController::statusMessageChanged,
            this, &BibleContext::statusMessageChanged);
}

QVariantList BibleContext::bibleTranslations() const { return m_controller.bibleTranslations(); }
QVariantList BibleContext::bibleBooks() const { return m_controller.bibleBooks(); }
QString BibleContext::bibleReferenceInput() const { return m_controller.bibleReferenceInput(); }
void BibleContext::setBibleReferenceInput(const QString &value) { m_controller.setBibleReferenceInput(value); }
QString BibleContext::statusMessage() const { return m_controller.statusMessage(); }
bool BibleContext::searchBibleReference() { return m_controller.searchBibleReference(); }
void BibleContext::showBibleVerse(int index) { m_controller.showBibleVerse(index); }
QVariantList BibleContext::bibleChapterNumbers(int bookId) const { return m_controller.bibleChapterNumbers(bookId); }
QVariantList BibleContext::bibleVerseNumbers(int bookId, int chapter) const { return m_controller.bibleVerseNumbers(bookId, chapter); }
bool BibleContext::presentBibleReference(int bookId, int chapter, int verse) { return m_controller.presentBibleReference(bookId, chapter, verse); }

} // namespace churchpresenter
