#include "presentation/TextPresentationController.h"

#include <QTest>

using namespace churchpresenter;

class TextPresentationControllerTest final : public QObject {
    Q_OBJECT
private slots:
    void editsAndNavigatesSlides()
    {
        TextPresentationController controller;
        Presentation presentation{.id = "p", .type = PresentationType::Text, .title = "Avisos"};
        presentation.slides = {{.id = "a", .label = "1", .text = "Primeiro"},
                               {.id = "b", .label = "2", .text = "Segundo", .order = 1}};
        controller.setPresentation(presentation);
        QVERIFY(controller.show(0));
        controller.next();
        QCOMPARE(controller.currentSlide().id, QStringLiteral("b"));
        controller.next();
        QCOMPARE(controller.currentSlide().id, QStringLiteral("b"));
        controller.previous();
        QCOMPARE(controller.currentSlide().id, QStringLiteral("a"));
        controller.stop();
        QVERIFY(!controller.visible());
    }

    void duplicatesAndSplitsSlides()
    {
        TextPresentationController controller;
        Presentation presentation{.id = "p", .type = PresentationType::Text, .title = "Mensagem"};
        presentation.slides = {{.id = "a", .label = "1", .text = "Parte um\nParte dois"}};
        controller.setPresentation(presentation);
        QVERIFY(controller.duplicateSlide("a"));
        QCOMPARE(controller.presentation().slides.size(), 2);
        QVERIFY(controller.splitSlide("a", 9));
        QCOMPARE(controller.presentation().slides.size(), 3);
        QCOMPARE(controller.presentation().slides[0].text, QStringLiteral("Parte um"));
        QCOMPARE(controller.presentation().slides[1].text, QStringLiteral("Parte dois"));
    }

    void followsSongSequenceWithoutDuplicatingSections()
    {
        TextPresentationController controller;
        Presentation song{.id="song",.type=PresentationType::Song,.title="Louvor"};
        song.slides={{.id="v1",.label="V1",.text="Verso"},{.id="c",.label="C",.text="Coro",.order=1}};
        song.sequence={"v1","c","v1","c"};
        controller.setPresentation(song);
        QVERIFY(controller.show(0));
        controller.next(); QCOMPARE(controller.currentSlide().id,QStringLiteral("c"));
        controller.next(); QCOMPARE(controller.currentSlide().id,QStringLiteral("v1"));
        QCOMPARE(controller.presentation().slides.size(),2);
        controller.last(); QCOMPARE(controller.currentSlide().id,QStringLiteral("c"));
    }

    void exposesTheNextSlideWithoutAdvancing()
    {
        TextPresentationController controller;
        Presentation presentation{.id="p",.type=PresentationType::Text,.title="Culto"};
        presentation.slides={{.id="a",.label="Atual",.text="Grande é o Senhor"},
                             {.id="b",.label="Próximo",.text="Na cidade do nosso Deus",.order=1}};
        controller.setPresentation(presentation);
        QVERIFY(controller.show(0));
        QCOMPARE(controller.nextSlide().text, QStringLiteral("Na cidade do nosso Deus"));
        QCOMPARE(controller.currentSlide().id, QStringLiteral("a"));
        controller.next();
        QVERIFY(controller.nextSlide().id.isEmpty());
    }
};

QTEST_MAIN(TextPresentationControllerTest)
#include "TextPresentationControllerTest.moc"
