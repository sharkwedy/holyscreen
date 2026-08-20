#include <QtTest/QTest>

#include "library/MediaRepository.h"

#include <QFile>
#include <QTemporaryDir>

using namespace churchpresenter;

class MediaRepositoryTest final : public QObject {
    Q_OBJECT

private slots:
    void importsOnceAndSurvivesRestart();
    void preservesPlaylistOrderWhenMovingAndRemoving();
    void preservesMixedAudioVideoImagePlaylist();
};

static QString createMediaFile(QTemporaryDir &directory, const QString &name)
{
    const auto path = directory.filePath(name);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return {};
    file.write("test");
    return path;
}

void MediaRepositoryTest::importsOnceAndSurvivesRestart()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = createMediaFile(directory, QStringLiteral("louvor.mp3"));
    const auto database = directory.filePath(QStringLiteral("presenter.db"));
    QString id;
    {
        MediaRepository repository(database);
        QVERIFY(repository.open());
        id = repository.add(MediaItem{.type = MediaType::Audio, .path = path});
        QVERIFY(!id.isEmpty());
        QCOMPARE(repository.add(MediaItem{.type = MediaType::Audio, .path = path}), id);
        QCOMPARE(repository.items(MediaType::Audio).size(), 1);
    }
    MediaRepository reopened(database);
    QVERIFY(reopened.open());
    const auto items = reopened.items(MediaType::Audio);
    QCOMPARE(items.size(), 1);
    QCOMPARE(items.front().id, id);
    QCOMPARE(items.front().title, QStringLiteral("louvor"));
}

void MediaRepositoryTest::preservesPlaylistOrderWhenMovingAndRemoving()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    MediaRepository repository(directory.filePath(QStringLiteral("presenter.db")));
    QVERIFY(repository.open());
    const auto first = repository.add(MediaItem{.type = MediaType::Audio, .path = createMediaFile(directory, "a.mp3")});
    const auto second = repository.add(MediaItem{.type = MediaType::Audio, .path = createMediaFile(directory, "b.mp3")});
    const auto third = repository.add(MediaItem{.type = MediaType::Audio, .path = createMediaFile(directory, "c.mp3")});

    QVERIFY(repository.move(third, 0));
    QCOMPARE(repository.items(MediaType::Audio)[0].id, third);
    QVERIFY(repository.remove(second));
    const auto remaining = repository.items(MediaType::Audio);
    QCOMPARE(remaining.size(), 2);
    QCOMPARE(remaining[0].id, third);
    QCOMPARE(remaining[1].id, first);
}

void MediaRepositoryTest::preservesMixedAudioVideoImagePlaylist()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    MediaRepository repository(directory.filePath(QStringLiteral("presenter.db")));
    QVERIFY(repository.open());

    const auto audio = repository.add(MediaItem{
        .type = MediaType::Audio,
        .path = createMediaFile(directory, QStringLiteral("entrada.mp3")),
    });
    const auto video = repository.add(MediaItem{
        .type = MediaType::Video,
        .path = createMediaFile(directory, QStringLiteral("avisos.mp4")),
    });
    const auto outroAudio = repository.add(MediaItem{
        .type = MediaType::Audio,
        .path = createMediaFile(directory, QStringLiteral("saida.wav")),
    });
    const auto image = repository.add(MediaItem{
        .type = MediaType::Image,
        .path = createMediaFile(directory, QStringLiteral("oferta.png")),
        .durationMs = 7000,
    });

    QCOMPARE(repository.playlistItems().size(), 4);
    QCOMPARE(repository.playlistItems()[0].id, audio);
    QCOMPARE(repository.playlistItems()[1].id, video);
    QCOMPARE(repository.playlistItems()[2].id, outroAudio);
    QCOMPARE(repository.playlistItems()[3].id, image);

    QVERIFY(repository.moveInPlaylist(image, 0));
    const auto reordered = repository.playlistItems();
    QCOMPARE(reordered[0].id, image);
    QCOMPARE(reordered[1].id, audio);
    QCOMPARE(reordered[2].id, video);
    QCOMPARE(reordered[3].id, outroAudio);

    QVERIFY(repository.remove(audio));
    const auto remaining = repository.playlistItems();
    QCOMPARE(remaining.size(), 3);
    QCOMPARE(remaining[0].id, image);
    QCOMPARE(remaining[1].id, video);
    QCOMPARE(remaining[2].id, outroAudio);
}

#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    MediaRepositoryTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "MediaRepositoryTest.moc"
