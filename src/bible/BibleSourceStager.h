#pragma once

#include "bible/BibleImportTypes.h"

#include <QUrl>

namespace churchpresenter {

struct StagedBibleSource {
    bool success = false;
    BibleSource source;
    QString localPath;
    QString error;
    bool cancelled = false;
};

class ZipArchiveExtractor final {
public:
    [[nodiscard]] static bool isSafeEntryName(const QString &entryName);
    [[nodiscard]] bool extract(const QString &archivePath, const QString &destination,
                               QString *error,
                               const BibleImportProgressCallback &progress = {},
                               const BibleImportCancellation &cancel = {}) const;
};

class GitBibleSourceStager final {
public:
    explicit GitBibleSourceStager(bool allowLocalRepositories = false);
    [[nodiscard]] StagedBibleSource stage(
        const BibleSource &source, const QString &destination,
        const BibleImportProgressCallback &progress = {},
        const BibleImportCancellation &cancel = {}) const;

private:
    bool m_allowLocalRepositories = false;
};

class ZipBibleSourceStager final {
public:
    explicit ZipBibleSourceStager(bool allowLocalFiles = false);
    [[nodiscard]] StagedBibleSource stage(
        const BibleSource &source, const QString &destination,
        const BibleImportProgressCallback &progress = {},
        const BibleImportCancellation &cancel = {}) const;

private:
    bool m_allowLocalFiles = false;
};

} // namespace churchpresenter
