#include "library/ThemeRepository.h"

#include <QTemporaryDir>
#include <QTest>

using namespace churchpresenter;

class ThemeRepositoryTest final : public QObject {
    Q_OBJECT
private slots:
    void persistsCompleteVisualTheme()
    {
        QTemporaryDir directory;
        ThemeRepository repository(directory.filePath("presenter.db"));
        QVERIFY(repository.open());
        Theme theme{.name="Culto", .backgroundType=BackgroundType::Image, .backgroundColor="#112233",
                    .backgroundImage="/tmp/fundo.png", .fontFamily="Arial", .fontSize=72,
                    .fontWeight=700, .textColor="#ffeecc", .horizontalAlignment="center",
                    .verticalAlignment="center", .margin=64, .outline=true, .shadow=true};
        const auto id=repository.save(theme);
        QVERIFY(!id.isEmpty());
        const auto restored=repository.theme(id);
        QCOMPARE(restored.name, QStringLiteral("Culto"));
        QCOMPARE(restored.backgroundType, BackgroundType::Image);
        QCOMPARE(restored.fontSize, 72);
        QVERIFY(restored.outline);
        QCOMPARE(repository.themes().size(), 1);
        QVERIFY(repository.remove(id));
    }
};
QTEST_MAIN(ThemeRepositoryTest)
#include "ThemeRepositoryTest.moc"
