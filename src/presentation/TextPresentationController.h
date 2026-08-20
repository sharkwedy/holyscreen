#pragma once

#include "presentation/PresentationTypes.h"

#include <QObject>

namespace churchpresenter {

class TextPresentationController final : public QObject {
    Q_OBJECT
public:
    explicit TextPresentationController(QObject *parent = nullptr);
    void setPresentation(Presentation presentation);
    [[nodiscard]] const Presentation &presentation() const;
    [[nodiscard]] Slide currentSlide() const;
    [[nodiscard]] Slide nextSlide() const;
    [[nodiscard]] int currentIndex() const;
    [[nodiscard]] bool visible() const;

    bool show(int index);
    void next();
    void previous();
    void first();
    void last();
    void stop();
    bool addSlide(QString label, QString text);
    bool updateSlide(const QString &id, QString label, QString text);
    bool duplicateSlide(const QString &id);
    bool splitSlide(const QString &id, int cursorPosition);
    bool removeSlide(const QString &id);
    bool moveSlide(const QString &id, int newIndex);

signals:
    void presentationChanged();
    void currentSlideChanged(const churchpresenter::Slide &slide);
    void visibleChanged(bool visible);

private:
    int indexOf(const QString &id) const;
    void normalizeOrder();
    [[nodiscard]] int navigationCount() const;
    Presentation m_presentation;
    int m_currentIndex = -1;
    bool m_visible = false;
};

} // namespace churchpresenter
