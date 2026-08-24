#pragma once

#include "app/ProcessMetrics.h"
#include "core/CommandTypes.h"
#include "core/EventTypes.h"

#include <QElapsedTimer>
#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>

#include <random>

namespace churchpresenter {

class ApplicationController;

//! Parâmetros de uma sessão de endurance. Os padrões correspondem ao smoke
//! curto documentado em `docs/ENDURANCE.md`; a validação da 1.0 usa duas horas.
struct EnduranceOptions {
    int durationSeconds = 300;
    QString reportPath;
    QString mediaDirectory;
    quint32 seed = 20260824u;
    int actionsPerMinute = 240;
};

//! Executa uma sessão de endurance dirigindo o aplicativo real pelo barramento
//! de comandos e produz um relatório JSON auditável.
//!
//! O executor nunca inventa conteúdo do operador: ele cria a própria
//! apresentação de texto e só usa mídia do diretório informado, que deve conter
//! arquivos sintéticos. Toda a avaliação de bloqueadores acontece a partir de
//! medições, e não de impressões: perda de saída, atraso do event loop,
//! crescimento de memória, travamento de reprodução, log crítico, laço de
//! automação e rejeição inesperada de comando.
class EnduranceRunner final : public QObject {
    Q_OBJECT

public:
    EnduranceRunner(ApplicationController &controller, EnduranceOptions options,
                    QObject *parent = nullptr);

    void start();

    [[nodiscard]] bool isFinished() const { return m_finished; }
    [[nodiscard]] bool passed() const;
    [[nodiscard]] QJsonObject report() const { return m_report; }

    //! Grava o relatório em `options.reportPath`. Devolve falso e preenche
    //! \a error quando o caminho não pode ser escrito.
    bool writeReport(QString *error = nullptr) const;

signals:
    void completed();

private:
    struct Failure {
        double atSeconds = 0.0;
        QString category;
        QString description;
    };

    struct Action {
        QString name;
        int weight = 1;
    };

    void prepareContent();
    void performAction();
    void takeSample();
    void finish();

    void recordFailure(const QString &category, const QString &description);
    CommandResult dispatch(const QString &type, const QVariantMap &payload = {});
    void observeEvent(const DomainEvent &event);
    [[nodiscard]] QStringList availableActions() const;
    void buildReport();

    ApplicationController &m_controller;
    const EnduranceOptions m_options;

    QTimer m_actionTimer;
    QTimer m_sampleTimer;
    QTimer m_lagTimer;
    QElapsedTimer m_wallClock;
    QElapsedTimer m_lagClock;

    std::mt19937 m_random;

    QString m_startedAt;
    QStringList m_mediaIds;
    int m_slideCount = 0;
    int m_baselineOutputs = 0;
    int m_minimumOutputs = 0;
    bool m_blackout = false;
    int m_roleCursor = 0;
    bool m_finished = false;

    quint64 m_actionCount = 0;
    QHash<QString, quint64> m_actionsByType;
    quint64 m_rejectedCommands = 0;

    ProcessSample m_firstSample;
    ProcessSample m_previousSample;
    qint64 m_previousSampleMs = 0;
    QList<double> m_cpuPercentSamples;
    QList<quint64> m_residentSamples;
    QList<double> m_lagSamples;

    QString m_lastMediaState;
    int m_lastMediaPosition = -1;
    int m_stalledSamples = 0;
    bool m_stallReported = false;

    QHash<QString, quint64> m_eventsByCorrelation;
    bool m_loopReported = false;

    quint64 m_warningBaseline = 0;
    quint64 m_criticalBaseline = 0;
    quint64 m_fatalBaseline = 0;

    QList<Failure> m_failures;
    QStringList m_blockers;
    QJsonObject m_report;
};

} // namespace churchpresenter
