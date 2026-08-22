#include "bible/BibleImportTypes.h"

namespace churchpresenter {

QString bibleSourceKindName(BibleSourceKind kind)
{
    switch (kind) {
    case BibleSourceKind::LocalFolder: return QStringLiteral("local-folder");
    case BibleSourceKind::GitHttps: return QStringLiteral("git-https");
    case BibleSourceKind::ZipUrl: return QStringLiteral("zip-url");
    case BibleSourceKind::HolyScreenJson: return QStringLiteral("holyscreen-json");
    }
    return {};
}

} // namespace churchpresenter
