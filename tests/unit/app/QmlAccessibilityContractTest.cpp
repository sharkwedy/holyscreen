#include <QtTest/QTest>

#include <QDirIterator>
#include <QFile>
#include <QRegularExpression>

namespace {

qsizetype matchingBrace(const QString &contents, qsizetype openingBrace)
{
    int depth = 0;
    QChar quote;
    bool escaped = false;
    bool lineComment = false;
    bool blockComment = false;

    for (qsizetype index = openingBrace; index < contents.size(); ++index) {
        const auto current = contents.at(index);
        const auto next = index + 1 < contents.size() ? contents.at(index + 1) : QChar{};

        if (lineComment) {
            if (current == u'\n') lineComment = false;
            continue;
        }
        if (blockComment) {
            if (current == u'*' && next == u'/') {
                blockComment = false;
                ++index;
            }
            continue;
        }
        if (!quote.isNull()) {
            if (escaped) {
                escaped = false;
            } else if (current == u'\\') {
                escaped = true;
            } else if (current == quote) {
                quote = {};
            }
            continue;
        }
        if (current == u'/' && next == u'/') {
            lineComment = true;
            ++index;
            continue;
        }
        if (current == u'/' && next == u'*') {
            blockComment = true;
            ++index;
            continue;
        }
        if (current == u'"' || current == u'\'') {
            quote = current;
            continue;
        }
        if (current == u'{') {
            ++depth;
        } else if (current == u'}' && --depth == 0) {
            return index;
        }
    }
    return -1;
}

} // namespace

class QmlAccessibilityContractTest final : public QObject {
    Q_OBJECT

private slots:
    void iconOnlyButtonsHaveAccessibleNames();
};

void QmlAccessibilityContractTest::iconOnlyButtonsHaveAccessibleNames()
{
    const QString uiRoot = QStringLiteral(HOLYSCREEN_SOURCE_DIR) + QStringLiteral("/src/ui");
    QDirIterator files(uiRoot, {QStringLiteral("*.qml")}, QDir::Files,
                       QDirIterator::Subdirectories);
    static const QRegularExpression buttonStart(
        QStringLiteral("\\b(?:Button|ToolButton|PlayerButton)\\s*\\{"));
    static const QRegularExpression literalText(
        QStringLiteral("\\btext\\s*:\\s*\"((?:\\\\.|[^\"\\\\])*)\""));
    static const QRegularExpression lettersOrNumbers(QStringLiteral("[\\p{L}\\p{N}]"));
    static const QRegularExpression accessibleName(
        QStringLiteral("\\bAccessible\\.name\\s*:"));
    QStringList failures;

    while (files.hasNext()) {
        const auto absolutePath = files.next();
        QFile file(absolutePath);
        QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(absolutePath));
        const auto contents = QString::fromUtf8(file.readAll());
        auto buttons = buttonStart.globalMatch(contents);
        while (buttons.hasNext()) {
            const auto button = buttons.next();
            const auto openingBrace = contents.indexOf(u'{', button.capturedStart());
            const auto closingBrace = matchingBrace(contents, openingBrace);
            QVERIFY2(closingBrace > openingBrace,
                     qPrintable(QStringLiteral("Bloco QML inválido em %1").arg(absolutePath)));
            const auto block = contents.mid(openingBrace, closingBrace - openingBrace + 1);
            const auto text = literalText.match(block);
            if (!text.hasMatch()) continue;
            const auto label = text.captured(1).trimmed();
            if (label.isEmpty() || lettersOrNumbers.match(label).hasMatch()) continue;
            if (accessibleName.match(block).hasMatch()) continue;

            const auto line = contents.left(button.capturedStart()).count(u'\n') + 1;
            failures.append(QStringLiteral("%1:%2 (%3)")
                                .arg(QDir(uiRoot).relativeFilePath(absolutePath))
                                .arg(line)
                                .arg(label));
        }
    }

    QVERIFY2(failures.isEmpty(),
             qPrintable(QStringLiteral("Botões de ícone sem Accessible.name:\n%1")
                            .arg(failures.join(u'\n'))));
}

QTEST_GUILESS_MAIN(QmlAccessibilityContractTest)
#include "QmlAccessibilityContractTest.moc"
