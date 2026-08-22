#include "bible/PathContainment.h"

#include <QDir>

namespace churchpresenter {

namespace {

QString normalizedPath(QString path)
{
    path.replace(QLatin1Char('\\'), QLatin1Char('/'));
    path = QDir::cleanPath(path);
    while (path.size() > 1 && path.endsWith(QLatin1Char('/'))
           && !(path.size() == 3 && path.at(1) == QLatin1Char(':'))) {
        path.chop(1);
    }
    return path;
}

} // namespace

Qt::CaseSensitivity nativePathCaseSensitivity()
{
#ifdef Q_OS_WIN
    return Qt::CaseInsensitive;
#else
    return Qt::CaseSensitive;
#endif
}

bool isPathContained(const QString &root, const QString &candidate,
                     Qt::CaseSensitivity caseSensitivity)
{
    if (root.trimmed().isEmpty() || candidate.trimmed().isEmpty()) return false;

    const auto normalizedRoot = normalizedPath(root);
    const auto normalizedCandidate = normalizedPath(candidate);
    if (normalizedRoot.compare(normalizedCandidate, caseSensitivity) == 0) return true;

    const auto prefix = normalizedRoot.endsWith(QLatin1Char('/'))
        ? normalizedRoot
        : normalizedRoot + QLatin1Char('/');
    return normalizedCandidate.startsWith(prefix, caseSensitivity);
}

bool isPathContained(const QString &root, const QString &candidate)
{
    return isPathContained(root, candidate, nativePathCaseSensitivity());
}

} // namespace churchpresenter
