#include "presentation/TextPresentationController.h"

#include <QUuid>
#include <algorithm>

namespace churchpresenter {

TextPresentationController::TextPresentationController(QObject *parent) : QObject(parent) {}

void TextPresentationController::setPresentation(Presentation presentation)
{
    m_presentation = std::move(presentation);
    normalizeOrder();
    m_currentIndex = navigationCount() == 0 ? -1 : 0;
    if (m_visible && m_currentIndex < 0) stop();
    emit presentationChanged();
    if (m_currentIndex >= 0) emit currentSlideChanged(currentSlide());
}

const Presentation &TextPresentationController::presentation() const { return m_presentation; }
Slide TextPresentationController::currentSlide() const
{
    if (m_currentIndex < 0 || m_currentIndex >= navigationCount()) return {};
    if (m_presentation.type == PresentationType::Song && !m_presentation.sequence.isEmpty()) {
        const auto id=m_presentation.sequence[m_currentIndex];
        const auto index=indexOf(id); return index>=0?m_presentation.slides[index]:Slide{};
    }
    return m_presentation.slides[m_currentIndex];
}
Slide TextPresentationController::nextSlide() const
{
    const auto nextIndex = m_currentIndex + 1;
    if (nextIndex < 0 || nextIndex >= navigationCount()) return {};
    if (m_presentation.type == PresentationType::Song && !m_presentation.sequence.isEmpty()) {
        const auto index = indexOf(m_presentation.sequence[nextIndex]);
        return index >= 0 ? m_presentation.slides[index] : Slide{};
    }
    return m_presentation.slides[nextIndex];
}
int TextPresentationController::currentIndex() const { return m_currentIndex; }
bool TextPresentationController::visible() const { return m_visible; }

bool TextPresentationController::show(int index)
{
    if (index < 0 || index >= navigationCount()) return false;
    m_currentIndex = index;
    emit currentSlideChanged(currentSlide());
    if (!m_visible) { m_visible = true; emit visibleChanged(true); }
    return true;
}
void TextPresentationController::next() { if (m_currentIndex + 1 < navigationCount()) show(m_currentIndex + 1); }
void TextPresentationController::previous() { if (m_currentIndex > 0) show(m_currentIndex - 1); }
void TextPresentationController::first() { if (!m_presentation.slides.isEmpty()) show(0); }
void TextPresentationController::last() { if (navigationCount()>0) show(navigationCount()-1); }
void TextPresentationController::stop() { if (m_visible) { m_visible = false; emit visibleChanged(false); } }

bool TextPresentationController::addSlide(QString label, QString text)
{
    m_presentation.slides.append(Slide{.id = QUuid::createUuid().toString(QUuid::WithoutBraces),
                                       .label = label.trimmed(), .text = text.trimmed()});
    normalizeOrder(); emit presentationChanged(); return true;
}
bool TextPresentationController::updateSlide(const QString &id, QString label, QString text)
{
    const auto index = indexOf(id); if (index < 0) return false;
    m_presentation.slides[index].label = label.trimmed();
    m_presentation.slides[index].text = text.trimmed();
    emit presentationChanged();
    if (index == m_currentIndex) emit currentSlideChanged(currentSlide());
    return true;
}
bool TextPresentationController::duplicateSlide(const QString &id)
{
    const auto index = indexOf(id); if (index < 0) return false;
    auto copy = m_presentation.slides[index]; copy.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    copy.label += QStringLiteral(" cópia");
    m_presentation.slides.insert(index + 1, copy); normalizeOrder(); emit presentationChanged(); return true;
}
bool TextPresentationController::splitSlide(const QString &id, int cursorPosition)
{
    const auto index = indexOf(id); if (index < 0) return false;
    auto &slide = m_presentation.slides[index];
    cursorPosition = std::clamp(cursorPosition, 0, static_cast<int>(slide.text.size()));
    const auto secondText = slide.text.mid(cursorPosition).trimmed();
    const auto firstText = slide.text.left(cursorPosition).trimmed();
    if (firstText.isEmpty() || secondText.isEmpty()) return false;
    slide.text = firstText;
    m_presentation.slides.insert(index + 1, Slide{.id = QUuid::createUuid().toString(QUuid::WithoutBraces),
                                                  .label = slide.label + QStringLiteral("b"), .text = secondText});
    normalizeOrder(); emit presentationChanged(); return true;
}
bool TextPresentationController::removeSlide(const QString &id)
{
    const auto index = indexOf(id); if (index < 0) return false;
    m_presentation.slides.removeAt(index); normalizeOrder();
    if (m_presentation.slides.isEmpty()) { m_currentIndex = -1; stop(); }
    else m_currentIndex = std::clamp(m_currentIndex, 0, static_cast<int>(m_presentation.slides.size()) - 1);
    emit presentationChanged(); return true;
}
bool TextPresentationController::moveSlide(const QString &id, int newIndex)
{
    const auto index = indexOf(id); if (index < 0) return false;
    newIndex = std::clamp(newIndex, 0, static_cast<int>(m_presentation.slides.size()) - 1);
    m_presentation.slides.move(index, newIndex); normalizeOrder();
    m_currentIndex = newIndex; emit presentationChanged(); return true;
}
int TextPresentationController::indexOf(const QString &id) const
{
    for (int index = 0; index < m_presentation.slides.size(); ++index)
        if (m_presentation.slides[index].id == id) return index;
    return -1;
}
void TextPresentationController::normalizeOrder()
{
    for (int index = 0; index < m_presentation.slides.size(); ++index) m_presentation.slides[index].order = index;
}
int TextPresentationController::navigationCount() const
{
    return m_presentation.type==PresentationType::Song&&!m_presentation.sequence.isEmpty()
        ? static_cast<int>(m_presentation.sequence.size()) : static_cast<int>(m_presentation.slides.size());
}

} // namespace churchpresenter
