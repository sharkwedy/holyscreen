#pragma once

#include "screens/OutputRole.h"
#include "screens/ScreenManager.h"

#include <QString>
#include <QVector>

namespace churchpresenter {

class OutputWindowManager;

enum class OutputConnectionState {
    Connected,
    Missing,
};

struct OutputDescriptor {
    QString screenId;
    QString screenFingerprint;
    QString displayName;
    bool enabled = true;
    bool connected = false;
    OutputRole role = OutputRole::Audience;
    OutputConnectionState state = OutputConnectionState::Missing;
    QString bibleTranslationId;
    bool mediaEnabled = true;
};

struct EnableOutputResult {
    enum Reason {
        None,
        LimitReached,
        ScreenDisconnected,
    };

    bool accepted = false;
    Reason reason = None;
};

class OutputManager {
public:
    static constexpr qsizetype MaximumOutputs = 5;

    EnableOutputResult enable(const ScreenDescriptor &screen);
    qsizetype enableAllAudienceScreens(const QVector<ScreenDescriptor> &screens);
    bool setBibleTranslation(const QString &screenFingerprint, const QString &translationId);
    bool setRole(const QString &screenFingerprint, OutputRole role);
    bool setMediaEnabled(const QString &screenFingerprint, bool enabled);
    bool setDisplayName(const QString &screenFingerprint, const QString &displayName);
    void applyScreens(const QVector<ScreenDescriptor> &screens);
    void replaceOutputs(const QVector<OutputDescriptor> &outputs);
    void restore(const OutputDescriptor &output);
    const QVector<OutputDescriptor> &activeOutputs() const;

private:
    QVector<OutputDescriptor> m_outputs;
};

} // namespace churchpresenter
